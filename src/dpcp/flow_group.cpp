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

flow_group::flow_group(dcmd::ctx* ctx, const flow_group_attr& attr, const flow_table* table)
    : obj(ctx)
    , m_attr(attr)
    , m_table(table)
    , m_group_id()
    , m_is_initialized()
    , m_matcher(nullptr)
{
}

status flow_group::create()
{
    status ret = DPCP_OK;
    uint32_t flow_table_id = 0;
    uint32_t in[DEVX_ST_SZ_DW(create_flow_group_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(create_flow_group_out)] = {0};
    size_t outlen = sizeof(out);

    if (!m_table) {
        log_error("Flow table is not valid\n");
        return DPCP_ERR_INVALID_PARAM;
    }
    if (m_is_initialized) {
        log_warn("Flow group was already created\n");
        return DPCP_ERR_CREATE;
    }
    if (!m_table->is_kernel_table() && m_table->get_table_id(flow_table_id) != DPCP_OK) {
        log_error("Flow table is not valid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    // Create flow matcher.
    flow_matcher_attr matcher_attr;
    matcher_attr.match_criteria = m_attr.match_criteria;
    matcher_attr.match_criteria_enabled = m_attr.match_criteria_enable;
    m_matcher = new (std::nothrow) flow_matcher(matcher_attr);
    if (!m_matcher) {
        return DPCP_ERR_NO_MEMORY;
    }

    // TODO: keep here, after POC i will use flow_matcher instead if flow_rule
    if (m_table->is_kernel_table()) {
        m_is_initialized = true;
        return DPCP_OK;
    }

    // Configure flow group.
    DEVX_SET(create_flow_group_in, in, opcode, MLX5_CMD_OP_CREATE_FLOW_GROUP);
    DEVX_SET(create_flow_group_in, in, table_id, flow_table_id);
    DEVX_SET(create_flow_group_in, in, start_flow_index, m_attr.start_flow_index);
    DEVX_SET(create_flow_group_in, in, end_flow_index, m_attr.end_flow_index);
    DEVX_SET(create_flow_group_in, in, match_criteria_enable, m_attr.match_criteria_enable);

    // Set match cratiria to flow group.
    void* match_params = DEVX_ADDR_OF(create_flow_group_in, in, match_criteria);
    m_matcher->apply(match_params, m_attr.match_criteria);

    // Create flow group HW object.
    ret = obj::create(in, sizeof(in), out, outlen);
    if (ret != DPCP_OK) {
        return ret;
    }
    m_group_id = DEVX_GET(create_flow_group_out, out, group_id);
    m_is_initialized = true;

    log_trace("Flow group created: match_criteria_enable=0x%x\n", m_attr.match_criteria_enable);
    log_trace("                    start_flow_index=0x%x\n", m_attr.start_flow_index);
    log_trace("                    end_flow_index=0x%x\n", m_attr.end_flow_index);
    log_trace("                    table_id=0x%x\n", flow_table_id);
    log_trace("                    group_id=0x%x\n", m_group_id);

    return ret;
}

status flow_group::get_group_id(uint32_t& group_id) const
{
    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }
    if (m_table->is_kernel_table()) {
        log_info("Flow group on root table do not support get_group_id()\n");
        return DPCP_ERR_NO_SUPPORT;
    }

    group_id = m_group_id;
    return DPCP_OK;
}

status flow_group::get_table_id(uint32_t& table_id) const
{
    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }
    if (m_table->is_kernel_table()) {
        log_info("Flow group on root table do not support get_table_id()\n");
        return DPCP_ERR_NO_SUPPORT;
    }

    uint32_t t_id = 0;
    status ret = m_table->get_table_id(t_id);
    if (ret != DPCP_OK) {
        log_error("Flow table is not valid, should not be here\n");
        return DPCP_ERR_QUERY;
    }
    
    table_id = t_id;
    return DPCP_OK;
}

status flow_group::get_match_criteria(match_params_ex& match) const
{
    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }

    match = m_attr.match_criteria;

    return DPCP_OK;
}

status flow_group::add_flow_rule(const flow_rule_attr_ex& attr, flow_rule_ex*& rule)
{
    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }

    flow_rule_ex* fr = new (std::nothrow) flow_rule_ex(get_ctx(), attr, m_table, this, m_matcher);
    if (!fr) {
        log_error("Flow rule allocation failed\n");
        return DPCP_ERR_NO_MEMORY;
    }

    auto ret = m_rules.insert(fr);
    if (!ret.second) {
        delete fr;
        log_error("Flow rule placement failed\n");
        return DPCP_ERR_NO_MEMORY;
    }
    rule = fr;

    return DPCP_OK;
}

status flow_group::remove_flow_rule(flow_rule_ex*& rule)
{
    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }

    if (m_rules.erase(rule) != 1) {
        log_error("Flow rule %p do not exist in this group\n", rule);
        return DPCP_ERR_INVALID_PARAM;
    }

    delete rule;
    rule = nullptr;
    return DPCP_OK;
}

flow_group::~flow_group()
{
    for (auto& rule : m_rules) {
        delete rule;
    }

    if (m_is_initialized && !m_table->is_kernel_table()) {
        obj::destroy();
    }
}

} // namespace dpcp

