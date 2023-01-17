/*
 * Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dpcp/internal.h"
#include "utils/os.h"

namespace dpcp {

////////////////////////////////////////////////////////////////////////
// flow_action_modify implementation.                                 //
////////////////////////////////////////////////////////////////////////

/**
 * @brief: Helper function to apply flow_action_modify from type set.
 */
void flow_action_modify::apply_modify_set_action(void* in, flow_action_modify_type_attr& attr)
{
    DEVX_SET(set_action_in, in, action_type, MLX5_ACTION_TYPE_SET);
    DEVX_SET(set_action_in, in, field, attr.set.field);
    DEVX_SET(set_action_in, in, offset, attr.set.offset);
    DEVX_SET(set_action_in, in, length, attr.set.length);
    DEVX_SET(set_action_in, in, data, attr.set.data);

    log_trace("Flow action modify, added set action, field 0x%x, offset 0x%x, length 0x%x\n",
              attr.set.field, attr.set.offset, attr.set.length);
}
/**
 * @brief: Helper function to apply flow_action_modify from type copy.
 */
void flow_action_modify::apply_modify_copy_action(void* in, flow_action_modify_type_attr& attr)
{
    DEVX_SET(copy_action_in, in, action_type, MLX5_ACTION_TYPE_COPY);
    DEVX_SET(copy_action_in, in, src_field, attr.copy.src_field);
    DEVX_SET(copy_action_in, in, src_offset, attr.copy.src_offset);
    DEVX_SET(copy_action_in, in, length, attr.copy.length);
    DEVX_SET(copy_action_in, in, dst_field, attr.copy.dst_field);
    DEVX_SET(copy_action_in, in, dst_offset, attr.copy.dst_offset);

    log_trace("Flow action modify, added copy action, src_field 0x%x, src_offset 0x%x,"
              " length 0x%x, dst_field 0x%x, dst_offset 0x%x,\n",
              attr.copy.src_field, attr.copy.src_offset, attr.set.length, attr.copy.dst_field,
              attr.copy.dst_offset);
}

flow_action_modify::flow_action_modify(dcmd::ctx* ctx, flow_action_modify_attr& attr)
    : flow_action(ctx)
    , m_attr(attr)
    , m_is_valid(false)
    , m_modify_id(0)
{
}

status flow_action_modify::prepare_prm_modify_buff()
{
    // Allocate in buffer
    m_inlen = DEVX_ST_SZ_BYTES(alloc_modify_header_context_in) +
        DEVX_UN_SZ_BYTES(set_add_copy_action_in_auto) * m_attr.actions.size();
    m_in.reset(new (std::nothrow) uint8_t[m_inlen]);
    if (!m_in) {
        log_error("Flow Action modify in buffer allocation failed\n");
        return DPCP_ERR_NO_MEMORY;
    }
    void* in = m_in.get();
    memset(in, 0, m_inlen);

    // Set modify header configurations
    DEVX_SET(alloc_modify_header_context_in, in, opcode, MLX5_CMD_OP_ALLOC_MODIFY_HEADER_CONTEXT);
    DEVX_SET(alloc_modify_header_context_in, in, table_type, m_attr.table_type);
    DEVX_SET(alloc_modify_header_context_in, in, num_of_actions, m_attr.actions.size());

    // Apply modify header actions by type.
    void* curr_action = DEVX_ADDR_OF(alloc_modify_header_context_in, in, actions);
    for (auto& action_attr : m_attr.actions) {
        switch (action_attr.type) {
        case flow_action_modify_type::SET:
            apply_modify_set_action(curr_action, action_attr);
            break;
        case flow_action_modify_type::COPY:
            apply_modify_copy_action(curr_action, action_attr);
            break;
        default:
            log_error("Flow Action modify unknown type 0x%x\n", action_attr.type);
            return DPCP_ERR_NO_SUPPORT;
        }
        curr_action = (uint8_t*)curr_action + DEVX_UN_SZ_BYTES(set_add_copy_action_in_auto);
    }

    return DPCP_OK;
}

status flow_action_modify::create_prm_modify()
{
    status ret = prepare_prm_modify_buff();
    if (ret != DPCP_OK) {
        log_error("Failed to prepare modify create buffer, status %d\n", ret);
        return ret;
    }

    // Create HW object
    ret = obj::create(m_in.get(), m_inlen, &m_out, m_outlen);
    if (ret != DPCP_OK) {
        log_error("Flow Action modify HW object create failed\n");
        return ret;
    }
    m_modify_id = DEVX_GET(alloc_modify_header_context_out, &m_out, modify_header_id);

    log_trace("flow_action_modify created: id=0x%x\n", m_modify_id);
    log_trace("                            table_type=0x%x\n", m_attr.table_type);
    log_trace("                            num_of_actions=%zu\n", m_attr.actions.size());

    m_is_valid = true;
    return DPCP_OK;
}

status flow_action_modify::apply(void* in)
{
    status ret = DPCP_OK;

    if (!m_is_valid) {
        ret = create_prm_modify();
        if (ret != DPCP_OK) {
            log_error("Failed to create Flow Action modify HW object, ret %d\n", ret);
            return ret;
        }
    }

    void* in_flow_context = DEVX_ADDR_OF(set_fte_in, in, flow_context);
    uint32_t action_enabled = DEVX_GET(flow_context, in_flow_context, action);
    action_enabled |= MLX5_FLOW_CONTEXT_ACTION_MOD_HDR;
    DEVX_SET(flow_context, in_flow_context, action, action_enabled);
    DEVX_SET(flow_context, in_flow_context, modify_header_id, m_modify_id);

    log_trace("Flow Action modify id 0x%x was applied\n", m_modify_id);
    return DPCP_OK;
}

status flow_action_modify::get_num_actions(size_t& num)
{
    num = m_attr.actions.size();
    return DPCP_OK;
}

status flow_action_modify::prepare_flow_desc_buffs()
{
    std::unique_ptr<dcmd::modify_action, std::default_delete<dcmd::modify_action[]>>
        modify_actions_guard(new (std::nothrow) dcmd::modify_action[m_attr.actions.size()]);
    if (!modify_actions_guard) {
        log_error("Flow Action modify failed to allocate modify action root list\n");
        return DPCP_ERR_NO_MEMORY;
    }

    dcmd::modify_action* modify_actions = modify_actions_guard.get();
    for (size_t i = 0; i < m_attr.actions.size(); ++i) {
        switch (m_attr.actions[i].type) {
        case flow_action_modify_type::SET:
            modify_actions[i].dst.config.action_type = MLX5_ACTION_TYPE_SET;
            modify_actions[i].dst.config.field = m_attr.actions[i].set.field;
            modify_actions[i].dst.config.length = m_attr.actions[i].set.length;
            modify_actions[i].dst.config.offset = m_attr.actions[i].set.offset;
            modify_actions[i].src.data = m_attr.actions[i].set.data;
            modify_actions[i].dst.data = htobe32(modify_actions[i].dst.data);
            modify_actions[i].src.data = htobe32(modify_actions[i].src.data);

            log_trace(
                "Flow Action modify was applied on root, type %d, field %d, length %d, offset "
                "%d, data %u\n",
                m_attr.actions[i].set.type, m_attr.actions[i].set.field,
                m_attr.actions[i].set.length, m_attr.actions[i].set.offset,
                m_attr.actions[i].set.data);
            break;
        case flow_action_modify_type::COPY:
            modify_actions[i].dst.config.action_type = MLX5_ACTION_TYPE_COPY;
            modify_actions[i].dst.config.field = m_attr.actions[i].copy.dst_field;
            modify_actions[i].dst.config.length = m_attr.actions[i].copy.length;
            modify_actions[i].dst.config.offset = m_attr.actions[i].copy.dst_offset;
            modify_actions[i].src.config.field = m_attr.actions[i].copy.src_field;
            modify_actions[i].src.config.offset = m_attr.actions[i].copy.src_offset;
            modify_actions[i].dst.data = htobe32(modify_actions[i].dst.data);
            modify_actions[i].src.data = htobe32(modify_actions[i].src.data);

            log_trace("Flow Action modify was applied on root, type %d, dst_field %d, length %d, "
                      "dst_offset "
                      "%d, src_field %d, src_offset %d, src_data %u, dst_data %u\n",
                      m_attr.actions[i].copy.type, m_attr.actions[i].copy.dst_field,
                      m_attr.actions[i].copy.length, m_attr.actions[i].copy.dst_offset,
                      m_attr.actions[i].copy.src_field, m_attr.actions[i].copy.src_offset,
                      modify_actions[i].src.data, modify_actions[i].dst.data);
            break;
        default:
            log_error("Flow Action modify on root, unknown type %d\n", m_attr.actions[i].type);
            return DPCP_ERR_NO_SUPPORT;
        }
    }

    m_actions_root = std::move(modify_actions_guard);
    return DPCP_OK;
}

status flow_action_modify::apply(dcmd::flow_desc& flow_desc)
{
    status ret = DPCP_OK;

    if (!m_actions_root) {
        ret = prepare_prm_modify_buff();
        if (ret != DPCP_OK) {
            log_error("Flow Action modify failed prepare prm buffer, ret %d\n", ret);
            return ret;
        }
        ret = prepare_flow_desc_buffs();
        if (ret != DPCP_OK) {
            log_error("Flow Action modify failed prepare dv buffer , ret %d\n", ret);
            return ret;
        }
    }

    flow_desc.modify_actions = reinterpret_cast<dcmd::modify_action*>(m_actions_root.get());
    flow_desc.num_of_actions = m_attr.actions.size();
    flow_desc.modify_acttions_obj_desc.in = m_in.get();
    flow_desc.modify_acttions_obj_desc.inlen = m_inlen;
    flow_desc.modify_acttions_obj_desc.out = &m_out;
    flow_desc.modify_acttions_obj_desc.outlen = m_outlen;
    return DPCP_OK;
}

status flow_action_modify::get_id(uint32_t& id)
{
    if (!m_is_valid) {
        log_error("Flow Action modify was not applied\n");
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

/**
 * @brief: Help function to allocate flow_action_reformat from type insert.
 */
status flow_action_reformat::alloc_reformat_insert_action(std::unique_ptr<uint8_t[]>& in_mem_guard,
                                                          size_t& in_len,
                                                          flow_action_reformat_attr& attr)
{
    if (!attr.insert.data) {
        log_error("Flow action reformat insert, no data provided\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    // Allocate in buffer
    in_len = DEVX_ST_SZ_BYTES(alloc_packet_reformat_context_in) + attr.insert.data_len;
    in_len += sizeof(uint32_t) - (in_len % sizeof(uint32_t));
    in_mem_guard.reset(new (std::nothrow) uint8_t[in_len]);
    void* in = in_mem_guard.get();
    if (!in) {
        log_error("Flow action reformat insert, in buffer allocation failed\n");
        return DPCP_ERR_NO_MEMORY;
    }
    memset(in, 0, in_len);

    // Set reformat insert configurations
    void* pck_reformat_ctx_in =
        DEVX_ADDR_OF(alloc_packet_reformat_context_in, in, packet_reformat_context);
    void* reformat = DEVX_ADDR_OF(packet_reformat_context_in, pck_reformat_ctx_in, reformat_data);
    DEVX_SET(alloc_packet_reformat_context_in, in, opcode,
             MLX5_CMD_OP_ALLOC_PACKET_REFORMAT_CONTEXT);
    DEVX_SET(packet_reformat_context_in, pck_reformat_ctx_in, reformat_data_size,
             attr.insert.data_len);
    DEVX_SET(packet_reformat_context_in, pck_reformat_ctx_in, reformat_param_0,
             attr.insert.start_hdr);
    DEVX_SET(packet_reformat_context_in, pck_reformat_ctx_in, reformat_type,
             MLX5_REFORMAT_TYPE_INSERT_HDR);
    DEVX_SET(packet_reformat_context_in, pck_reformat_ctx_in, reformat_param_1, attr.insert.offset);
    memcpy(reformat, attr.insert.data, attr.insert.data_len);

    log_trace(
        "Flow action reformat insert allocated, data_size 0x%x, start_hdr 0x%x, offset 0x%x\n",
        attr.insert.data_len, attr.insert.start_hdr, attr.insert.offset);

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
    std::unique_ptr<uint8_t[]> in_mem_guard;
    size_t in_len = 0;
    status ret = DPCP_OK;

    // Allocate reformat action by type.
    switch (m_attr.type) {
    case flow_action_reformat_type::INSERT_HDR:
        ret = alloc_reformat_insert_action(in_mem_guard, in_len, m_attr);
        break;
    default:
        log_error("Flow action reformat, not supported type %d\n", m_attr.type);
        return;
    }
    if (ret != DPCP_OK) {
        log_error("Flow action reformat from type 0x%x faile with error %d\n", m_attr.type, ret);
        return;
    }

    // Create flow group HW object.
    void* in = in_mem_guard.get();
    ret = obj::create(in, in_len, out, out_len);
    if (ret != DPCP_OK) {
        log_error("Flow action reformat HW object create failed\n");
        return;
    }
    m_reformat_id = DEVX_GET(alloc_packet_reformat_context_out, out, packet_reformat_id);

    log_trace("flow_action_reformat created: id=0x%x\n", m_reformat_id);
    log_trace("                              type=0x%x\n", m_attr.type);

    // reformat creation was successful.
    m_is_valid = true;
}

flow_action_reformat::~flow_action_reformat()
{
    // Object will be destroyed in obj destructor.
}

status flow_action_reformat::apply(void* in)
{
    if (!m_is_valid) {
        log_error("Flow Action reformat was not applied\n");
        return DPCP_ERR_NOT_APPLIED;
    }

    void* in_flow_context = DEVX_ADDR_OF(set_fte_in, in, flow_context);
    uint32_t action_enabled = DEVX_GET(flow_context, in_flow_context, action);
    action_enabled |= MLX5_FLOW_CONTEXT_ACTION_PACKET_REFORMAT;
    DEVX_SET(flow_context, in_flow_context, action, action_enabled);
    DEVX_SET(flow_context, in_flow_context, packet_reformat_id, m_reformat_id);

    log_trace("Flow Action reformat 0x%x was applied\n", m_reformat_id);
    return DPCP_OK;
}

status flow_action_reformat::apply(dcmd::flow_desc& flow_desc)
{
    NOT_IN_USE(flow_desc);
    log_error("Flow Action reformat is not supported on root table\n");
    return DPCP_ERR_NO_SUPPORT;
}

status flow_action_reformat::get_id(uint32_t& id)
{
    if (!m_is_valid) {
        log_error("Flow Action reformat was not applied\n");
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

    log_trace("Flow Action tag 0x%x was applied\n", m_tag_id);
    return DPCP_OK;
}

status flow_action_tag::apply(dcmd::flow_desc& flow_desc)
{
    flow_desc.flow_id = m_tag_id;
    log_trace("Flow Action tag 0x%x was applied on root\n", m_tag_id);
    return DPCP_OK;
}

////////////////////////////////////////////////////////////////////////
// flow_action_fwd implemitation.                                     //
////////////////////////////////////////////////////////////////////////

status flow_action_fwd::apply(void* in)
{
    void* in_flow_context = DEVX_ADDR_OF(set_fte_in, in, flow_context);
    void* in_dests = DEVX_ADDR_OF(flow_context, in_flow_context, destination);
    uint8_t* curr_dest = reinterpret_cast<uint8_t*>(in_dests);

    // Set all destination by type.
    for (forwardable_obj* dest : m_dests) {
        uint32_t id = 0;
        uint32_t type = 0;

        status ret = dest->get_id(id);
        if (ret != DPCP_OK) {
            log_error("Flow Action forward, failed to get destination id\n");
            return ret;
        }
        type = dest->get_fwd_type();

        // Set destination.
        DEVX_SET(dest_format_struct, curr_dest, destination_id, id);
        DEVX_SET(dest_format_struct, curr_dest, destination_type, type);
        curr_dest += DEVX_ST_SZ_BYTES(dest_format_struct);

        log_trace("Flow Action forward, added destination, type 0x%x, id 0x%x\n", type, id);
    }

    // Enable forward action.
    uint32_t action_enabled = DEVX_GET(flow_context, in_flow_context, action);
    action_enabled |= MLX5_FLOW_CONTEXT_ACTION_FWD_DEST;
    DEVX_SET(flow_context, in_flow_context, action, action_enabled);

    // Set number of destination.
    DEVX_SET(flow_context, in_flow_context, destination_list_size, m_dests.size());

    log_trace("Flow Action forward was applied\n");
    return DPCP_OK;
}

status flow_action_fwd::create_root_action_fwd()
{
    status ret = DPCP_OK;
    size_t num_dest_obj = m_dests.size();
    std::vector<dcmd::fwd_dst_desc> dst_desc_vec;

    for (size_t i = 0; i < num_dest_obj; i++) {
        dcmd::fwd_dst_desc dst_desc;
        ret = m_dests[i]->get_fwd_desc(dst_desc);
        if (ret != DPCP_OK) {
            log_error("Flow Action forward, failed to get forward dest description, ret %d\n", ret);
            return ret;
        }
        dst_desc_vec.push_back(dst_desc);
    }

    m_root_action_fwd = obj::get_ctx()->create_action_fwd(dst_desc_vec);
    return m_root_action_fwd ? DPCP_OK : DPCP_ERR_CREATE;
}

status flow_action_fwd::apply(dcmd::flow_desc& flow_desc)
{
    status ret = DPCP_OK;

    if (!m_root_action_fwd) {
        ret = create_root_action_fwd();
        if (ret != DPCP_OK) {
            log_error("Flow Action forward, failed to create root Flow Action Forward obj\n");
            return ret;
        }
    }

    int dcmd_ret = m_root_action_fwd->apply(flow_desc);
    if (dcmd_ret != DCMD_EOK) {
        log_error("Flow Action forward, failed to apply on root\n");
        return DPCP_ERR_NOT_APPLIED;
    }

    return DPCP_OK;
}

size_t flow_action_fwd::get_dest_num()
{
    return m_dests.size();
}

const std::vector<forwardable_obj*>& flow_action_fwd::get_dest_objs()
{
    return m_dests;
}

flow_action_fwd::flow_action_fwd(dcmd::ctx* ctx, std::vector<forwardable_obj*> dests)
    : flow_action(ctx)
    , m_dests(dests)
    , m_root_action_fwd()
{
}

////////////////////////////////////////////////////////////////////////
// flow_action_reparse implementation.                                //
////////////////////////////////////////////////////////////////////////

flow_action_reparse::flow_action_reparse(dcmd::ctx* ctx)
    : flow_action(ctx)
{
}

status flow_action_reparse::apply(void* in)
{
    void* in_flow_context = DEVX_ADDR_OF(set_fte_in, in, flow_context);

    // Enable reparse action:
    uint32_t action_enabled = DEVX_GET(flow_context, in_flow_context, action);
    action_enabled |= MLX5_FLOW_CONTEXT_ACTION_REPARSE;
    DEVX_SET(flow_context, in_flow_context, action, action_enabled);

    log_trace("Flow Action reparse was applied\n");
    return DPCP_OK;
}

status flow_action_reparse::apply(dcmd::flow_desc& flow_desc)
{
    NOT_IN_USE(flow_desc);
    log_error("Flow Action reparse is not supported on root table\n");
    return DPCP_ERR_NO_SUPPORT;
}

////////////////////////////////////////////////////////////////////////
// flow_action_generator implemitation.                               //
////////////////////////////////////////////////////////////////////////

flow_action_generator::flow_action_generator(dcmd::ctx* ctx, const adapter_hca_capabilities* caps)
    : m_ctx(ctx)
    , m_caps(caps)
{
}

// TODO: Need to add acpabilities check for all Flow Actions, i added the adapter_hca_capabilities
// To the flow_action_generator, but we need to think if to do it here or inside flow_rule_ex
// because we have some complex capabilities check like reformat + modify. Also the caps should be
// per Flow Table type.

std::shared_ptr<flow_action> flow_action_generator::create_fwd(std::vector<forwardable_obj*> dests)
{
    return std::shared_ptr<flow_action>(new (std::nothrow) flow_action_fwd(m_ctx, dests));
}

std::shared_ptr<flow_action> flow_action_generator::create_tag(uint32_t id)
{
    return std::shared_ptr<flow_action>(new (std::nothrow) flow_action_tag(m_ctx, id));
}

std::shared_ptr<flow_action> flow_action_generator::create_reformat(flow_action_reformat_attr& attr)
{
    return std::shared_ptr<flow_action>(new (std::nothrow) flow_action_reformat(m_ctx, attr));
}

std::shared_ptr<flow_action> flow_action_generator::create_modify(flow_action_modify_attr& attr)
{
    return std::shared_ptr<flow_action>(new (std::nothrow) flow_action_modify(m_ctx, attr));
}

std::shared_ptr<flow_action> flow_action_generator::create_reparse()
{
    return std::shared_ptr<flow_action>(new (std::nothrow) flow_action_reparse(m_ctx));
}

} // namespace dpcp
