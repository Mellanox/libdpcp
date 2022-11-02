/*
 * Copyright (c) 2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
// flow_rule_ex                                                       //
////////////////////////////////////////////////////////////////////////

bool flow_rule_ex::verify_flow_actions(const std::vector<std::shared_ptr<flow_action>>& actions)
{
    if (actions.empty()) {
        log_error("No Flow Actions were added to Flow Rule\n");
        return false;
    }

    // Insert all actions to map with key that represent the object type.
    // In that way we can ensure that we have only one flow_action from each type.
    // also will provide a fast access to specific flow action type if needed.
    for (auto action : actions) {
        // typeid() will identify the flow action type, type_index() is for the std::hash function.
        // It do not support typeid() as key, so the type_index make wrapper that the hash function
        // can use.
        auto& action_ref = *action.get();
        m_actions.insert({std::type_index(typeid(action_ref)), action});
    }

    if (m_actions.size() != actions.size()) {
        log_error("Flow Action placement failure, could be caused by multiple actions from the "
                  "same type\n");
        return false;
    }

    // Flow rule must have flow action forward.
    auto action_iter = m_actions.find(std::type_index(typeid(flow_action_fwd)));
    if (action_iter == m_actions.end()) {
        log_error("Flow Rule must have Flow Action forward to destination\n");
        return false;
    }

    return true;
}

flow_rule_ex::flow_rule_ex(dcmd::ctx* ctx, const flow_rule_attr_ex& attr,
                           std::weak_ptr<const flow_table> table,
                           std::weak_ptr<const flow_group> group,
                           std::shared_ptr<const flow_matcher> matcher)
    : obj(ctx)
    , m_match_value(attr.match_value)
    , m_is_initialized()
    , m_table(table)
    , m_group(group)
    , m_is_valid_actions(false)
    , m_matcher(matcher)
{
    m_is_valid_actions = verify_flow_actions(attr.actions);
}

status flow_rule_ex::get_match_value(match_params_ex& match_val)
{
    match_val = m_match_value;
    return DPCP_OK;
}

////////////////////////////////////////////////////////////////////////
// flow_rule_ex_prm                                                   //
////////////////////////////////////////////////////////////////////////

flow_rule_ex_prm::flow_rule_ex_prm(dcmd::ctx* ctx, const flow_rule_attr_ex& attr,
                                   std::weak_ptr<const flow_table> table,
                                   std::weak_ptr<const flow_group> group,
                                   std::shared_ptr<const flow_matcher> matcher)
    : flow_rule_ex(ctx, attr, table, group, matcher)
    , m_flow_index(attr.flow_index)
{
}

status flow_rule_ex_prm::alloc_in_buff(size_t& in_len, std::unique_ptr<uint8_t[]>& in_mem_guard)
{
    // Get destination list size.
    size_t dest_list_size = 0;
    auto action_fwd = m_actions.find(std::type_index(typeid(flow_action_fwd)));
    if (action_fwd != m_actions.end()) {
        dest_list_size =
            std::dynamic_pointer_cast<flow_action_fwd>(action_fwd->second)->get_dest_num();
    }

    // Allocate in buffer.
    in_len = DEVX_ST_SZ_BYTES(set_fte_in) + DEVX_ST_SZ_BYTES(dest_format_struct) * dest_list_size;
    in_mem_guard.reset(new (std::nothrow) uint8_t[in_len]);
    if (!in_mem_guard) {
        log_error("Flow rule in buf memory allocation failed\n");
        return DPCP_ERR_NO_MEMORY;
    }
    memset(in_mem_guard.get(), 0, in_len);

    return DPCP_OK;
}

status flow_rule_ex_prm::config_flow_rule(void* in)
{
    flow_table_type ft_type = flow_table_type::FT_END;
    uint32_t ft_id = 0;
    status ret = DPCP_OK;
    std::shared_ptr<const flow_table_prm> prm_table =
        std::dynamic_pointer_cast<const flow_table_prm>(m_table.lock());
    std::shared_ptr<const flow_group_prm> prm_group =
        std::dynamic_pointer_cast<const flow_group_prm>(m_group.lock());

    DEVX_SET(set_fte_in, in, opcode, MLX5_CMD_OP_SET_FLOW_TABLE_ENTRY);
    DEVX_SET(set_fte_in, in, flow_index, m_flow_index);

    // Set flow table type.
    ret = prm_table->get_table_type(ft_type);
    if (ret != DPCP_OK) {
        log_error("Flow rule failed to get flow table type\n");
        return ret;
    }
    DEVX_SET(set_fte_in, in, table_type, ft_type);

    // Set flow table id.
    ret = prm_table->get_table_id(ft_id);
    if (ret != DPCP_OK) {
        log_error("Flow rule failed to get flow table id\n");
        return ret;
    }
    DEVX_SET(set_fte_in, in, table_id, ft_id);

    // Set flow group id
    void* in_flow_context = DEVX_ADDR_OF(set_fte_in, in, flow_context);
    uint32_t fg_id = 0;
    ret = prm_group->get_group_id(fg_id);
    if (ret != DPCP_OK) {
        log_error("Flow rule failed to get flow group id\n");
        return ret;
    }
    DEVX_SET(flow_context, in_flow_context, group_id, fg_id);

    return DPCP_OK;
}

status flow_rule_ex_prm::create()
{
    status ret = DPCP_OK;

    if (!m_is_valid_actions) {
        log_error("Flow Actions are not valid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    // Prepare PRM buffers.
    uint32_t out[DEVX_ST_SZ_DW(set_fte_out)] {0};
    size_t outlen = sizeof(out);
    size_t in_len = 0;
    std::unique_ptr<uint8_t[]> in_mem_guard;
    ret = alloc_in_buff(in_len, in_mem_guard);
    if (ret != DPCP_OK) {
        log_error("Flow Rule buffer allocation failed, ret %d\n", ret);
        return ret;
    }
    void* in = in_mem_guard.get();

    // configure flow rule attributes.
    ret = config_flow_rule(in);
    if (ret != DPCP_OK) {
        log_error("Flow Rule set configuration failed, ret %d\n", ret);
        return ret;
    }

    // Set match values
    void* match_params = DEVX_ADDR_OF(set_fte_in, in, flow_context.match_value);
    ret = m_matcher->apply(match_params, m_match_value);
    if (ret != DPCP_OK) {
        log_error("Flow Rule failed to apply match parameters\n");
        return ret;
    }

    // Apply flow actions.
    for (auto action : m_actions) {
        ret = action.second->apply(in);
        if (ret != DPCP_OK) {
            log_error("Flow rule failed to apply actions\n");
            return ret;
        }
    }

    // Create flow rule HW object.
    ret = obj::create(in, in_len, out, outlen);
    if (ret != DPCP_OK) {
        log_error("Flow rule failed to create HW object\n");
        return ret;
    }

    uint32_t flow_rule_id = 0;
    obj::get_id(flow_rule_id);
    log_trace("Flow rule created: id=0x%x\n", flow_rule_id);

    m_is_initialized = true;
    return ret;
}

////////////////////////////////////////////////////////////////////////
// flow_rule_ex_kernel                                                //
////////////////////////////////////////////////////////////////////////

flow_rule_ex_kernel::flow_rule_ex_kernel(dcmd::ctx* ctx, const flow_rule_attr_ex& attr,
                                         std::weak_ptr<const flow_table> table,
                                         std::weak_ptr<const flow_group> group,
                                         std::shared_ptr<const flow_matcher> matcher)
    : flow_rule_ex(ctx, attr, table, group, matcher)
    , m_priority(attr.priority)
    , m_flow(nullptr)
{
}

status flow_rule_ex_kernel::set_match_params(dcmd::flow_desc& flow_desc, prm_match_params& criteria,
                                             prm_match_params& values)
{
    status ret = DPCP_OK;

    // Get match criteria.
    memset(&criteria, 0, sizeof(criteria));
    criteria.buf_sz = sizeof(criteria.buf);
    void* prm_mc = (uint32_t*)&criteria.buf;
    match_params_ex match_criteria;
    ret = m_group.lock()->get_match_criteria(match_criteria);
    if (ret != DPCP_OK) {
        log_error("Flow Rule failed to get match criteria, ret %d\n", ret);
        return ret;
    }
    ret = m_matcher->apply(prm_mc, match_criteria);
    if (ret != DPCP_OK) {
        log_error("Flow Rule failed to apply match criteria, ret %d\n", ret);
        return ret;
    }

    // Get match values.
    memset(&values, 0, sizeof(values));
    values.buf_sz = sizeof(values.buf);
    uint32_t* prm_mv = (uint32_t*)&values.buf;
    ret = m_matcher->apply(prm_mv, m_match_value);
    if (ret != DPCP_OK) {
        log_error("Flow Rule failed to apply match values, ret %d\n", ret);
        return ret;
    }

    // Set match params.
    flow_desc.match_criteria = (dcmd::flow_match_parameters*)&criteria;
    flow_desc.match_value = (dcmd::flow_match_parameters*)&values;
    return DPCP_OK;
}

status flow_rule_ex_kernel::create()
{
    status ret = DPCP_OK;
    struct dcmd::flow_desc dcmd_flow;
    prm_match_params mask;
    prm_match_params values;

    if (!m_is_valid_actions) {
        log_error("Flow Actions are not valid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    // Prepare dcmd flow description.
    dcmd_flow.priority = m_priority;

    // Set Flow Rule match params
    ret = set_match_params(dcmd_flow, mask, values);
    if (ret != DPCP_OK) {
        log_error("Flow Rule failed to set match params on root, ret %d\n", ret);
        return ret;
    }

    // Set Flow Rule actions.
    for (auto action : m_actions) {
        ret = action.second->apply(dcmd_flow);
        if (ret != DPCP_OK) {
            log_error("Flow Rule failed to apply Flow Action, ret %d\n", ret);
            return ret;
        }
    }

    // Create dcmd flow object.
    m_flow = get_ctx()->create_flow(&dcmd_flow);
    return m_flow ? DPCP_OK : DPCP_ERR_CREATE;
}

flow_rule_ex_kernel::~flow_rule_ex_kernel()
{
    if (m_flow) {
        delete m_flow;
        m_flow = nullptr;
    }
}

} // namespace dpcp
