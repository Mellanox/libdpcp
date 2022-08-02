/*
 * Copyright © 2019-2022 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Nvidia Corporation and its affiliates
 * (the "Company") and all right, title, and interest in and to the software
 * product, including all associated intellectual property rights, are and
 * shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 */

#include <type_traits>

#include "dpcp/internal.h"
#include "utils/os.h"

namespace dpcp {

////////////////////////////////////////////////////////////////////////
// flow_group implementation.                                         //
////////////////////////////////////////////////////////////////////////

flow_group::flow_group(dcmd::ctx* ctx, const flow_group_attr& attr,
                       std::weak_ptr<const flow_table> table)
    : obj(ctx)
    , m_attr(attr)
    , m_table(table)
    , m_is_initialized(false)
{
}

status flow_group::create()
{
    if (!m_table.lock()) {
        log_error("Flow table is not valid\n");
        return DPCP_ERR_INVALID_PARAM;
    }
    if (m_is_initialized) {
        log_warn("Flow group was already created\n");
        return DPCP_ERR_CREATE;
    }

    // Create flow matcher.
    flow_matcher_attr matcher_attr;
    matcher_attr.match_criteria = m_attr.match_criteria;
    matcher_attr.match_criteria_enabled = m_attr.match_criteria_enable;
    m_matcher = std::make_shared<flow_matcher>(matcher_attr);
    if (!m_matcher) {
        log_error("Flow matcher allocation failed.\n");
        return DPCP_ERR_NO_MEMORY;
    }

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

status flow_group::remove_flow_rule(std::weak_ptr<flow_rule_ex>& rule)
{
    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }

    if (m_rules.erase(rule.lock()) != 1) {
        log_error("Flow rule %p do not exist in this group\n", rule.lock().get());
        return DPCP_ERR_INVALID_PARAM;
    }

    return DPCP_OK;
}

template <class FR>
status flow_group::create_flow_rule_ex(const flow_rule_attr_ex& attr,
                                       std::weak_ptr<flow_rule_ex>& rule)
{
    static_assert(std::is_base_of<flow_rule_ex, FR>::value, "FR must inherit from flow_rule_ex");

    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }

    std::weak_ptr<flow_group> weak_from_this = shared_from_this();
    std::shared_ptr<flow_rule_ex> fr(new (std::nothrow)
                                         FR(get_ctx(), attr, m_table, weak_from_this, m_matcher));
    if (!fr) {
        log_error("Flow rule allocation failed\n");
        return DPCP_ERR_NO_MEMORY;
    }

    auto ret = m_rules.insert(fr);
    if (!ret.second) {
        log_error("Flow rule placement failed\n");
        return DPCP_ERR_NO_MEMORY;
    }
    rule = fr;

    return DPCP_OK;
}

////////////////////////////////////////////////////////////////////////
// flow_group_prm implementation.                                     //
////////////////////////////////////////////////////////////////////////

flow_group_prm::flow_group_prm(dcmd::ctx* ctx, const flow_group_attr& attr,
                               std::weak_ptr<const flow_table> table)
    : flow_group(ctx, attr, table)
    , m_group_id()
{
}

status flow_group_prm::create()
{
    status ret = DPCP_OK;
    uint32_t flow_table_id = 0;
    uint32_t in[DEVX_ST_SZ_DW(create_flow_group_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(create_flow_group_out)] = {0};
    size_t outlen = sizeof(out);

    // First call create of flow_group parent class
    if (flow_group::create() != DPCP_OK) {
        return DPCP_ERR_NO_MEMORY;
    }

    std::shared_ptr<const flow_table_prm> prm_table =
        std::dynamic_pointer_cast<const flow_table_prm>(m_table.lock());
    if (!prm_table || prm_table->get_table_id(flow_table_id) != DPCP_OK) {
        log_error("Flow table is not valid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    // Configure flow group.
    DEVX_SET(create_flow_group_in, in, opcode, MLX5_CMD_OP_CREATE_FLOW_GROUP);
    DEVX_SET(create_flow_group_in, in, table_id, flow_table_id);
    DEVX_SET(create_flow_group_in, in, start_flow_index, m_attr.start_flow_index);
    DEVX_SET(create_flow_group_in, in, end_flow_index, m_attr.end_flow_index);
    DEVX_SET(create_flow_group_in, in, match_criteria_enable, m_attr.match_criteria_enable);

    // Set match criteria to flow group.
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

status flow_group_prm::get_group_id(uint32_t& group_id) const
{
    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }

    group_id = m_group_id;
    return DPCP_OK;
}

status flow_group_prm::get_table_id(uint32_t& table_id) const
{
    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }

    uint32_t t_id = 0;
    status ret =
        std::dynamic_pointer_cast<const flow_table_prm>(m_table.lock())->get_table_id(t_id);
    if (ret != DPCP_OK) {
        log_error("Flow table is not valid, should not be here\n");
        return DPCP_ERR_QUERY;
    }

    table_id = t_id;
    return DPCP_OK;
}

status flow_group_prm::add_flow_rule(const flow_rule_attr_ex& attr,
                                     std::weak_ptr<flow_rule_ex>& rule)
{
    return create_flow_rule_ex<flow_rule_ex_prm>(attr, rule);
}

////////////////////////////////////////////////////////////////////////
// flow_group_kernel implementation.                                  //
////////////////////////////////////////////////////////////////////////

flow_group_kernel::flow_group_kernel(dcmd::ctx* ctx, const flow_group_attr& attr,
                                     std::weak_ptr<const flow_table> table)
    : flow_group(ctx, attr, table)
{
}

status flow_group_kernel::create()
{
    status ret = flow_group::create();
    if (ret != DPCP_OK) {
        log_error("failed to create base flow group object\n");
        return ret;
    }

    m_is_initialized = true;
    return DPCP_OK;
}

status flow_group_kernel::add_flow_rule(const flow_rule_attr_ex& attr,
                                        std::weak_ptr<flow_rule_ex>& rule)
{
    return create_flow_rule_ex<flow_rule_ex_kernel>(attr, rule);
}

} // namespace dpcp
