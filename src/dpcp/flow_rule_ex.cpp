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

flow_rule_ex::flow_rule_ex    (dcmd::ctx* ctx, const flow_rule_attr_ex& attr,
    const flow_table* table, const flow_group* group, const flow_matcher* matcher)
    : obj(ctx)
    , m_match_value(attr.match_value)
    , m_priority(attr.priority)
    , m_is_initialized()
    , m_table(table)
    , m_group(group)
    , m_flow_index(attr.flow_index)
    , m_is_valid_actions(true)
    , m_matcher(matcher)
    , m_flow(nullptr)
{
    // Insert all actions to map with key that represent the object type.
    // In that way we can ensure that we have only one flow_action from each type.
    // also will provide a fast access to specific flow action type if needed.
    for (auto action : attr.actions) {
        // typeid() will identify the flow action type, type_index() is for the std::hash function.
        // It do not support typeid() as key, so the type_index make wrapped that the hash function can use.
        m_actions.insert({ std::type_index(typeid(*action.get())), action });
    }

    if (m_actions.size() != attr.actions.size()) {
        log_error("Flow action placement failure, could be caused by multiple actions from the same type\n");
        m_is_valid_actions = false;
    }
    // TODO: more validation tests acording to HCA.caps.
}

status flow_rule_ex::alloc_in_buff(size_t& in_len, void*& in)
{
    // Get destination list size.
    size_t dest_list_size = 0;
    auto action_fwd = m_actions.find(std::type_index(typeid(flow_action_fwd)));
    if (action_fwd != m_actions.end()) {
        dest_list_size = dynamic_cast<flow_action_fwd*>(action_fwd->second.get())->get_dest_num();
    }

    // Allocate in buffer.
    in_len = DEVX_ST_SZ_BYTES(set_fte_in) + DEVX_ST_SZ_BYTES(dest_format_struct) * dest_list_size;
    in = new (std::nothrow) uint8_t[in_len];
    if (!in) {
        log_error("Flow rule in buf memory allocation failed\n");
        return DPCP_ERR_NO_MEMORY;
    }
    memset(in, 0, in_len);

    return DPCP_OK;
}

void flow_rule_ex::free_in_buff(void*& in)
{
    delete[] (uint8_t*)in;
    in = nullptr;
}

status flow_rule_ex::config_flow_rule(void* in)
{
    flow_table_type ft_type = flow_table_type::FT_END;
    uint32_t ft_id = 0;
    status ret = DPCP_OK;

    DEVX_SET(set_fte_in, in, opcode, MLX5_CMD_OP_SET_FLOW_TABLE_ENTRY);
    DEVX_SET(set_fte_in, in, flow_index, m_flow_index);

    // Set flow table type.
    ret = m_table->get_table_type(ft_type);
    if (ret != DPCP_OK) {
        log_error("Flow rule failed to get flow table type\n");
        free_in_buff(in);
        return ret;
    }
    DEVX_SET(set_fte_in, in, table_type, ft_type);

    // Set flow table id.
    ret = m_table->get_table_id(ft_id);
    if (ret != DPCP_OK) {
        log_error("Flow rule failed to get flow table id\n");
        free_in_buff(in);
        return ret;
    }
    DEVX_SET(set_fte_in, in, table_id, ft_id);

    // Set flow group id
    void* in_flow_context = DEVX_ADDR_OF(set_fte_in, in, flow_context);
    uint32_t fg_id = 0;
    ret = m_group->get_group_id(fg_id);
    if (ret != DPCP_OK) {
        log_error("Flow rule failed to get flow group id\n");
        free_in_buff(in);
        return ret;
    }
    DEVX_SET(flow_context, in_flow_context, group_id, fg_id);

    return DPCP_OK;
}

// TODO: unify for GA
struct prm_match_params {
    size_t buf_sz;
    uint32_t buf[DEVX_ST_SZ_DW(fte_match_param)];
};

// Quick and dirty for POC.
// the GA solution will be to use the flow matcher to set the match value and masks
// then to call dcmd::flow_table with the flow_matcher output.
// for now we use the legacy flow_rule as is.
// TODO: use flow_matcher for GA
status flow_rule_ex::create_root_flow_rule()
{
    status ret = DPCP_OK;
    match_params_ex mask_ex;
    m_group->get_match_criteria(mask_ex);
    match_params mask;
    memcpy(mask.dst_mac, mask_ex.match_lyr2.dst_mac, sizeof(mask.dst_mac));
    memcpy(mask.src_mac, mask_ex.match_lyr2.src_mac, sizeof(mask.src_mac));
    mask.ethertype = mask_ex.match_lyr2.ethertype;
    mask.vlan_id = mask_ex.match_lyr2.first_vlan_id; // 12 bits
    mask.dst_ip = mask_ex.match_lyr3.dst_ip;
    mask.src_ip = mask_ex.match_lyr3.src_ip;
    mask.dst_port = mask_ex.match_lyr4.dst_port;
    mask.src_port = mask_ex.match_lyr4.src_port;;
    mask.protocol = mask_ex.match_lyr3.ip_protocol;
    mask.ip_version = 0; // 4 bits
    flow_rule* fr = new (std::nothrow) flow_rule(obj::get_ctx(), m_priority, mask);
    m_flow = fr;

    match_params value;
    memcpy(value.dst_mac, m_match_value.match_lyr2.dst_mac, sizeof(value.dst_mac));
    memcpy(value.src_mac, m_match_value.match_lyr2.src_mac, sizeof(value.src_mac));
    value.ethertype = m_match_value.match_lyr2.ethertype;
    value.vlan_id = m_match_value.match_lyr2.first_vlan_id; // 12 bits
    value.dst_ip = m_match_value.match_lyr3.dst_ip;
    value.src_ip = m_match_value.match_lyr3.src_ip;
    value.dst_port = m_match_value.match_lyr4.dst_port;
    value.src_port = m_match_value.match_lyr4.src_port;;
    value.protocol = m_match_value.match_lyr3.ip_protocol;
    value.ip_version = 0; // 4 bits

    fr->set_match_value(value);

    if (m_actions.find(std::type_index(typeid(flow_action_tag))) != m_actions.end()) {
        fr->set_flow_id(dynamic_cast<flow_action_tag*>(m_actions[std::type_index(typeid(flow_action_tag))].get())->get_tag_id());
    }
    if (m_actions.find(std::type_index(typeid(flow_action_fwd))) != m_actions.end()) {
        auto dests = dynamic_cast<flow_action_fwd*>(m_actions[std::type_index(typeid(flow_action_fwd))].get())->get_dest_objs();
        for (auto dest : dests) {
            if (dynamic_cast<flow_table*>(dest)) {
                fr->add_dest_table(dynamic_cast<flow_table*>(dest));
            } else if (dynamic_cast<tir*>(dest)) {
                fr->add_dest_tir(dynamic_cast<tir*>(dest));
            }
        }
    }
    if (m_actions.find(std::type_index(typeid(flow_action_modify))) != m_actions.end()) {
        flow_action_modify* modify_action = dynamic_cast<flow_action_modify*>(m_actions[std::type_index(typeid(flow_action_modify))].get());

        size_t num_of_actions = 0;
        ret = modify_action->get_num_actions(num_of_actions);
        if (ret != DPCP_OK) {
            log_error("failed to get number of actions\n");
            return ret;
        }

        dcmd::modify_action* modify_actions = new dcmd::modify_action[num_of_actions];
        ret = modify_action->apply_root(modify_actions);
        if (ret != DPCP_OK) {
            delete[] modify_actions;
            log_error("failed to apply modify actions on root flow rule\n");
            return ret;
        }
        fr->set_modify_header(modify_actions, num_of_actions);
        delete[] modify_actions;
    }

    ret = fr->apply_settings();
    if (ret != DPCP_OK) {
        log_error("failed to apply flow rule on root table\n");
        return ret;
    }

    return DPCP_OK;
}

status flow_rule_ex::create()
{
    status ret = DPCP_OK;
    
    if (m_actions.empty() || !m_is_valid_actions) {
        log_error("Flow rule actions added are not valid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (m_table->is_kernel_table()) {
        ret = create_root_flow_rule();
        return ret;
    }

    // Prepare PRM buffers.
    uint32_t out[DEVX_ST_SZ_DW(set_fte_out)] {0};
    size_t outlen = sizeof(out);
    void* in = nullptr;
    size_t in_len = 0;
    ret = alloc_in_buff(in_len, in);
    if (ret != DPCP_OK) {
        return ret;
    }

    // configure flow rule attributes.
    ret = config_flow_rule(in);
    if (ret != DPCP_OK) {
        free_in_buff(in);
        return ret;
    }    

    // Set match values
    void* match_params = DEVX_ADDR_OF(set_fte_in, in, flow_context.match_value);
    ret = m_matcher->apply(match_params, m_match_value);
    if (ret != DPCP_OK) {
        log_error("Flow rule failed to apply match parameters\n");
        free_in_buff(in);
        return ret;
    }

    // Apply flow actions.
    for (auto action : m_actions) {
        ret = action.second->apply(in);
        if (ret != DPCP_OK) {
            log_error("Flow rule failed to apply actions\n");
            free_in_buff(in);
            return ret;
        }
    }

    // Create flow rule HW object.
    ret = obj::create(in, in_len, out, outlen);
    if (ret != DPCP_OK) {
        log_error("Flow rule failed to create HW object\n");
        free_in_buff(in);
        return ret;
    }

    uint32_t flow_rule_id = 0;
    obj::get_id(flow_rule_id);
    log_trace("Flow rule created: id=0x%x\n", flow_rule_id);

    m_is_initialized = true;
    free_in_buff(in);
    return ret;
}

flow_rule_ex::~flow_rule_ex()
{
    // TODO: fix after doing GA root table.
    if (nullptr != m_flow) {
        delete m_flow;
        m_flow = nullptr;
    }
}

} // namespace dpcp

