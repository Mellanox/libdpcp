/*
 Copyright (C) Mellanox Technologies, Ltd. 2019-2020. ALL RIGHTS RESERVED.

 This software product is a proprietary product of Mellanox Technologies, Ltd.
 (the "Company") and all right, title, and interest in and to the software
 product, including all associated intellectual property rights, are and shall
 remain exclusively with the Company. All rights in or to the software product
 are licensed, not sold. All rights not licensed are reserved.

 This software product is governed by the End User License Agreement provided
 with the software product.
*/

#include "dpcp/internal.h"
#include "utils/os.h"

namespace dpcp {

////////////////////////////////////////////////////////////////////////
// flow_action_modify implemitation.                                  //
////////////////////////////////////////////////////////////////////////

/**
 * @brief: Help function to apply flow_action_modify from type set.
 */
void flow_action_modify::apply_modify_set_action(void* in, flow_action_modify_type_attr& attr)
{
    DEVX_SET(set_action_in, in, action_type, MLX5_ACTION_TYPE_SET);
    DEVX_SET(set_action_in, in, field, attr.set.field);
    DEVX_SET(set_action_in, in, offset, attr.set.offset.to_ulong());
    DEVX_SET(set_action_in, in, length, attr.set.length.to_ulong());
    DEVX_SET(set_action_in, in, data, attr.set.data);
    log_trace("Flow action modify, added set action, field 0x%x, offset 0x%lx, length 0x%lx\n",
            attr.set.field, attr.set.offset.to_ulong(), attr.set.length.to_ulong());
}

flow_action_modify::flow_action_modify(dcmd::ctx* ctx, flow_action_modify_attr& attr)
    : flow_action(ctx)
    , m_attr(attr)
    , m_is_valid(false)
{
}

status flow_action_modify::create_prm_modify()
{
    uint32_t out[DEVX_ST_SZ_DW(alloc_modify_header_context_out)] = {0};
    size_t out_len = sizeof(out);

    // TODO: check num of actions error with capabilities
    // Allocate modify header in buff, depended of num_of_actions.
    size_t in_len = DEVX_ST_SZ_BYTES(alloc_modify_header_context_in) +
            DEVX_UN_SZ_BYTES(set_add_copy_action_in_auto) * m_attr.actions.size();
    void *in = new (std::nothrow) uint8_t[in_len];
    if (!in) {
        log_error("Flow action modify allocation failed\n");
        return DPCP_ERR_NO_MEMORY;
    }
    memset(in, 0, in_len);

    // Set modify header configurations
    DEVX_SET(alloc_modify_header_context_in, in, opcode, MLX5_CMD_OP_ALLOC_MODIFY_HEADER_CONTEXT);
    DEVX_SET(alloc_modify_header_context_in, in, table_type, m_attr.table_type);
    DEVX_SET(alloc_modify_header_context_in, in, num_of_actions, m_attr.actions.size());

    // Apply modify header actions by type.
    void* curr_action = DEVX_ADDR_OF(alloc_modify_header_context_in, in, actions);
    for (auto& action_attr : m_attr.actions) {
        switch(action_attr.type) {
            case flow_action_modify_type::SET:
                apply_modify_set_action(curr_action, action_attr);
                break;
            default:
                log_error("Flow action modify not supported type 0x%x\n", action_attr.type);
                delete[] (uint8_t*)in;
                return DPCP_ERR_NO_SUPPORT;
        }
        curr_action = (uint8_t*)curr_action + DEVX_UN_SZ_BYTES(set_add_copy_action_in_auto);
    }

    // Create HW object
    status ret = obj::create(in, in_len, out, out_len);
    if (ret != DPCP_OK) {
        delete[] (uint8_t*)in;
        log_error("flow action modify HW object create failed\n");
        return ret;
    }
    m_modify_id = DEVX_GET(alloc_modify_header_context_out, out, modify_header_id);

    log_trace("flow_action_modify created: id=0x%x\n", m_modify_id);
    log_trace("                            table_type=0x%x\n", m_attr.table_type);
    log_trace("                            num_of_actions=0x%lx\n", m_attr.actions.size());

    delete[] (uint8_t*)in;
    m_is_valid = true;

    return DPCP_OK;
}

status flow_action_modify::apply(void* in)
{
    status ret = DPCP_OK;

    if (!m_is_valid) {
        ret = create_prm_modify();
        if (ret != DPCP_OK) {
            return ret;
        }
    }

    void* in_flow_context = DEVX_ADDR_OF(set_fte_in, in, flow_context);
    uint32_t action_enabled = DEVX_GET(flow_context, in_flow_context, action);
    action_enabled |= MLX5_FLOW_CONTEXT_ACTION_MOD_HDR;
    DEVX_SET(flow_context, in_flow_context, action, action_enabled);
    DEVX_SET(flow_context, in_flow_context, modify_header_id, m_modify_id);

    log_trace("Flow action modify id 0x%x was applied\n", m_modify_id);

    return DPCP_OK;
}

status flow_action_modify::get_num_actions(size_t& num)
{
    num = m_attr.actions.size();
    return DPCP_OK;
}

status flow_action_modify::apply_root(dcmd::modify_action* modify_actions)
{
    for (size_t i = 0; i < m_attr.actions.size(); ++i) {
        switch (m_attr.actions[i].type) {
            case flow_action_modify_type::SET:
                modify_actions[i].action_type = MLX5_ACTION_TYPE_SET;
                modify_actions[i].field = m_attr.actions[i].set.field;
                modify_actions[i].length = m_attr.actions[i].set.length.to_ulong();
                modify_actions[i].offset = m_attr.actions[i].set.offset.to_ulong();
                modify_actions[i].data1 = m_attr.actions[i].set.data;
                modify_actions[i].data0 = htobe32(modify_actions[i].data0);
                modify_actions[i].data1 = htobe32(modify_actions[i].data1);
                log_trace("Flow action modify was applied to root, type %d,field %d,length %lu,offset %lu,data %u\n",
                    m_attr.actions[i].set.type, m_attr.actions[i].set.field, m_attr.actions[i].set.length.to_ulong(),
                    m_attr.actions[i].set.offset.to_ulong(), m_attr.actions[i].set.data);
                break;
            default:
                return DPCP_ERR_NO_SUPPORT;
        }
    }

    return DPCP_OK;
}

status flow_action_modify::get_id(uint32_t& id)
{
    if (!m_is_valid) {
        return DPCP_ERR_NOT_APPLIED;
    }

    id = m_modify_id;
    return DPCP_OK;
}

flow_action_modify::~flow_action_modify()
{
    // Object will be destroyed in obj destructor.
}

////////////////////////////////////////////////////////////////////////
// flow_action_reformat implemitation.                                //
////////////////////////////////////////////////////////////////////////

// TODO: reformat_data_size check < HCA_CAP.max_reformat_insert_size from hca_cap
/**
 * @brief: Help function to allocate flow_action_reformat from type insert.
 */
status flow_action_reformat::alloc_reformat_insert_action(void*& in, size_t& in_len,
        flow_action_reformat_attr& attr)
{
    if (!attr.insert.data) {
        log_error("Flow action reformat insert, no data provided\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    // Allocate in buffer
    in_len = DEVX_ST_SZ_BYTES(alloc_packet_reformat_context_in) + attr.insert.data_len.to_ulong();
    in_len += sizeof(uint32_t) - (in_len % sizeof(uint32_t));
    in = new (std::nothrow) uint8_t[in_len];
    if (!in) {
        log_error("Flow action reformat insert, in buffer allocation failed\n");
        return DPCP_ERR_NO_MEMORY;
    }
    memset(in, 0, in_len);

    // Set reformat insert configurations
    void *pck_reformat_ctx_in = DEVX_ADDR_OF(alloc_packet_reformat_context_in, in, packet_reformat_context);
    void *reformat = DEVX_ADDR_OF(packet_reformat_context_in, pck_reformat_ctx_in, reformat_data);
    DEVX_SET(alloc_packet_reformat_context_in,
            in, opcode, MLX5_CMD_OP_ALLOC_PACKET_REFORMAT_CONTEXT);
    DEVX_SET(packet_reformat_context_in,
            pck_reformat_ctx_in, reformat_data_size, attr.insert.data_len.to_ulong());
    DEVX_SET(packet_reformat_context_in,
            pck_reformat_ctx_in, reformat_param_0, attr.insert.start_hdr);
    DEVX_SET(packet_reformat_context_in,
            pck_reformat_ctx_in, reformat_type, MLX5_REFORMAT_TYPE_INSERT_HDR);
    DEVX_SET(packet_reformat_context_in,
            pck_reformat_ctx_in, reformat_param_1, attr.insert.offset);
    memcpy(reformat, attr.insert.data, attr.insert.data_len.to_ulong());

    log_trace("Flow action reformat insert allocated, data_size 0x%lx, start_hdr 0x%x, offset 0x%x\n",
            attr.insert.data_len.to_ulong(), attr.insert.start_hdr, attr.insert.offset);

    return DPCP_OK;
}

flow_action_reformat::flow_action_reformat(dcmd::ctx* ctx, flow_action_reformat_attr& attr)
: flow_action(ctx)
, m_attr(attr)
, m_is_valid(false)
, m_reformat_id(0)
{
    uint32_t out[DEVX_ST_SZ_DW(alloc_packet_reformat_context_out)] = {0};
    size_t out_len = DEVX_ST_SZ_BYTES(alloc_packet_reformat_context_out);
    void* in = nullptr;
    size_t in_len = 0;
    status ret = DPCP_OK;

    // Allocate reformat action by type.
    switch (attr.type) {
        case flow_action_reformat_type::INSERT_HDR:
            ret = alloc_reformat_insert_action(in, in_len, attr);
            break;
        default:
            log_error("Flow action reformat, not supported type %d\n", attr.type);
            return;
    }
    if (ret != DPCP_OK) {
        log_error("Flow action reformat from type 0x%x faile with error %d\n", attr.type, ret);
        return;
    }

    // Create flow group HW object.
    ret = obj::create(in, in_len, out, out_len);
    if (ret != DPCP_OK) {
        log_error("Flow action reformat HW object create failed\n");
        delete[] (uint8_t*)in;
        return;
    }
    m_reformat_id = DEVX_GET(alloc_packet_reformat_context_out, out, packet_reformat_id);

    log_trace("flow_action_reformat created: id=0x%x\n", m_reformat_id);
    log_trace("                              type=0x%x\n", attr.type);

    // reformat creation was successful.
    m_is_valid = true;
    delete[] (uint8_t*)in;
}

flow_action_reformat::~flow_action_reformat()
{
    // Object will be destroyed in obj destructor.
}

status flow_action_reformat::apply(void* in)
{
    if (!m_is_valid) {
        return DPCP_ERR_NOT_APPLIED;
    }

    void* in_flow_context = DEVX_ADDR_OF(set_fte_in, in, flow_context);
    uint32_t action_enabled = DEVX_GET(flow_context, in_flow_context, action);
    action_enabled |= MLX5_FLOW_CONTEXT_ACTION_PACKET_REFORMAT;
    DEVX_SET(flow_context, in_flow_context, action, action_enabled);
    DEVX_SET(flow_context, in_flow_context, packet_reformat_id, m_reformat_id);

    log_trace("Flow action reformat 0x%x was applied\n", m_reformat_id);
    return DPCP_OK;
}

status flow_action_reformat::get_id(uint32_t& id)
{
    if (!m_is_valid) {
        return DPCP_ERR_NOT_APPLIED;
    }

    id = m_reformat_id;
    return DPCP_OK;
}

////////////////////////////////////////////////////////////////////////
// flow_action_tag implemitation.                                     //
////////////////////////////////////////////////////////////////////////

flow_action_tag::flow_action_tag(dcmd::ctx* ctx, uint32_t id)
: flow_action(ctx)
, m_tag_id(id)
{
}

status flow_action_tag::apply(void* in)
{
    void* in_flow_context = DEVX_ADDR_OF(set_fte_in, in, flow_context);
    DEVX_SET(flow_context, in_flow_context, flow_tag, m_tag_id);

    log_trace("Flow action tag 0x%x was applied\n", m_tag_id);
    return DPCP_OK;
}

uint32_t flow_action_tag::get_tag_id()
{
    return m_tag_id;
}

////////////////////////////////////////////////////////////////////////
// flow_action_fwd implemitation.                                     //
////////////////////////////////////////////////////////////////////////

status flow_action_fwd::get_dst_attr(obj* dest, uint32_t& type, uint32_t& id)
{
    // Check if destination is Tir.
    if (dynamic_cast<tir*>(dest)) {
        id = dynamic_cast<tir*>(dest)->get_tirn();
        type = MLX5_FLOW_DESTINATION_TYPE_TIR;
    }
    // Check if destination is Flow Table.
    else if (dynamic_cast<flow_table*>(dest)) {
        status ret = dynamic_cast<flow_table*>(dest)->get_table_id(id);
        if (ret != DPCP_OK) {
            log_error("Flow action forward, destination flow table is not valid\n");
            return DPCP_ERR_INVALID_PARAM;
        }
        type = MLX5_FLOW_DESTINATION_TYPE_FLOW_TABLE;
    }
    // We should not be here, unknown destination.
    else {
        log_error("Flow action forward, not supported destination type\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    return DPCP_OK;
}

status flow_action_fwd::apply(void* in)
{
    void *in_flow_context = DEVX_ADDR_OF(set_fte_in, in, flow_context);
    void *in_dests = DEVX_ADDR_OF(flow_context, in_flow_context, destination);
    uint8_t* curr_dest = reinterpret_cast<uint8_t*>(in_dests);

    // Set all destination by type.
    for (obj* dest : m_dests) {
        uint32_t id = 0;
        uint32_t type = 0;

        status ret = get_dst_attr(dest, type, id);
        if (ret != DPCP_OK) {
            log_error("Flow action forward, failed to apply destination\n");
            return ret;
        }

        // Set destination.
        DEVX_SET(dest_format_struct, curr_dest, destination_id, id);
        DEVX_SET(dest_format_struct, curr_dest, destination_type, type);
        curr_dest += DEVX_ST_SZ_BYTES(dest_format_struct);

        log_trace("Flow action forward, added destination, type 0x%x, id 0x%x\n", type, id);
    }

    // Enable forward action.
    uint32_t action_enabled = DEVX_GET(flow_context, in_flow_context, action);
    action_enabled |= MLX5_FLOW_CONTEXT_ACTION_FWD_DEST;
    DEVX_SET(flow_context, in_flow_context, action, action_enabled);

    // Set number of destination.
    DEVX_SET(flow_context, in_flow_context, destination_list_size, m_dests.size());

    log_trace("Flow action forward was applied\n");
    return DPCP_OK;
}

size_t flow_action_fwd::get_dest_num()
{
    return m_dests.size();
}

const std::vector<obj*>& flow_action_fwd::get_dest_objs()
{
    return m_dests;
}

flow_action_fwd::flow_action_fwd(dcmd::ctx* ctx, std::vector<obj*> dests)
: flow_action(ctx)
, m_dests(dests)
{
}

////////////////////////////////////////////////////////////////////////
// flow_action_generator implemitation.                               //
////////////////////////////////////////////////////////////////////////

flow_action_generator::flow_action_generator(dcmd::ctx* ctx)
: m_ctx(ctx)
{
}

std::shared_ptr<flow_action> flow_action_generator::create_flow_action_fwd(std::vector<obj*> dests)
{
    return std::shared_ptr<flow_action>(new (std::nothrow) flow_action_fwd(m_ctx, dests));
}

std::shared_ptr<flow_action> flow_action_generator::create_flow_action_tag(uint32_t id)
{
    return std::shared_ptr<flow_action>(new (std::nothrow) flow_action_tag(m_ctx, id));
}

std::shared_ptr<flow_action> flow_action_generator::create_flow_action_reformat(flow_action_reformat_attr& attr)
{
    return std::shared_ptr<flow_action>(new (std::nothrow) flow_action_reformat(m_ctx, attr));
}

std::shared_ptr<flow_action> flow_action_generator::create_flow_action_modify(flow_action_modify_attr& attr)
{
    return std::shared_ptr<flow_action>(new (std::nothrow) flow_action_modify(m_ctx, attr));
}

} // namespace dpcp

