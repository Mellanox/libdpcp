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

#include <algorithm>
#include <type_traits>

#include "dpcp/internal.h"
#include "utils/os.h"

namespace dpcp {

////////////////////////////////////////////////////////////////////////
// flow_table implementation.                                         //
////////////////////////////////////////////////////////////////////////

flow_table::flow_table(dcmd::ctx* ctx, flow_table_type type)
    : forwardable_obj(ctx)
    , m_type(type)
    , m_is_initialized(false)
{
}

status flow_table::get_flow_table_status() const
{
    if (!m_is_initialized) {
        log_error("Flow table HW object was not created\n");
        return DPCP_ERR_NOT_APPLIED;
    }

    return DPCP_OK;
}

status flow_table::get_table_type(flow_table_type& table_type) const
{
    status ret = get_flow_table_status();
    if (ret != DPCP_OK) {
        log_error("Failed to get Flow Table type, bad status %d\n", ret);
        return ret;
    }

    table_type = m_type;
    return DPCP_OK;
}

status flow_table::remove_flow_group(std::weak_ptr<flow_group>& group)
{
    std::shared_ptr<flow_group> shrd_group = group.lock();

    status ret = get_flow_table_status();
    if (ret != DPCP_OK) {
        log_error("Failed to remove Flow Group %p, bad status %d\n", shrd_group.get(), ret);
        return ret;
    }

    if (m_groups.erase(group.lock()) != 1) {
        log_error("Flow Group %p do not exist in this Flow Table\n", shrd_group.get());
        return DPCP_ERR_INVALID_PARAM;
    }

    return DPCP_OK;
}

template <class FG>
status flow_table::create_flow_group(const flow_group_attr& attr, std::weak_ptr<flow_group>& group)
{
    static_assert(std::is_base_of<flow_group, FG>::value, "FG must inherit from flow_group");

    status ret = get_flow_table_status();
    if (ret != DPCP_OK) {
        log_error("Failed to create Flow Group, bad status %d\n", ret);
        return ret;
    }

    std::weak_ptr<flow_table> weak_from_this = shared_from_this();
    std::shared_ptr<flow_group> fg(new (std::nothrow) FG(get_ctx(), attr, weak_from_this));
    if (!fg) {
        log_error("Flow Group allocation failed\n");
        return DPCP_ERR_NO_MEMORY;
    }

    auto iter = m_groups.insert(fg);
    if (!iter.second) {
        log_error("Flow Group placement failed\n");
        return DPCP_ERR_NO_MEMORY;
    }

    group = fg;
    return DPCP_OK;
}

int flow_table::get_fwd_type() const
{
    return MLX5_FLOW_DESTINATION_TYPE_FLOW_TABLE;
}

////////////////////////////////////////////////////////////////////////
// flow_table_prm implementation.                                     //
////////////////////////////////////////////////////////////////////////

// Constructor should be used only for DEVX flow tables.
flow_table_prm::flow_table_prm(dcmd::ctx* ctx, const flow_table_attr& attr)
    : flow_table(ctx, attr.type)
    , m_table_id(0)
    , m_attr(attr)
{
}

status flow_table_prm::create()
{
    status ret = DPCP_OK;
    uint32_t in[DEVX_ST_SZ_DW(create_flow_table_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(create_flow_table_out)] = {0};
    size_t outlen = sizeof(out);

    if (m_is_initialized) {
        log_warn("Flow Table was already created\n");
        return DPCP_OK;
    }

    // Set flow table operation mode.
    DEVX_SET(create_flow_table_in, in, op_mod, m_attr.op_mod);
    switch (m_attr.op_mod) {
    // Regular flow table mode.
    case flow_table_op_mod::FT_OP_MOD_NORMAL:
        ret = set_miss_action(in);
        break;
    // Unsupported flow table mode.
    default:
        log_error("Flow table operation mode %d is not supported\n", m_attr.op_mod);
        ret = DPCP_ERR_NO_SUPPORT;
    }
    if (ret != DPCP_OK) {
        return ret;
    }

    // Set flow table configurations.
    DEVX_SET(create_flow_table_in, in, opcode, MLX5_CMD_OP_CREATE_FLOW_TABLE);
    DEVX_SET(create_flow_table_in, in, table_type, m_attr.type);
    DEVX_SET(create_flow_table_in, in, flow_table_context.level, m_attr.level);
    DEVX_SET(create_flow_table_in, in, flow_table_context.log_size, m_attr.log_size);

    // Set flow table flags.
    DEVX_SET(create_flow_table_in, in, flow_table_context.decap_en,
             !!(m_attr.flags & flow_table_flags::FT_EN_DECAP));
    DEVX_SET(create_flow_table_in, in, flow_table_context.reformat_en,
             !!(m_attr.flags & flow_table_flags::FT_EN_REFORMAT));

    // Create flow table HW object.
    ret = obj::create(in, sizeof(in), out, outlen);
    if (ret != DPCP_OK) {
        log_error("Failed to create Flow table HW object, ret %d\n", ret);
        return ret;
    }
    m_table_id = DEVX_GET(create_flow_table_out, out, table_id);

    log_trace("Flow table created: flags=0x%zux\n", m_attr.flags);
    log_trace("                    def_miss_action=0x%x\n", m_attr.def_miss_action);
    log_trace("                    level=0x%x\n", m_attr.level);
    log_trace("                    log_size=0x%x\n", m_attr.log_size);
    log_trace("                    op_mod=0x%x\n", m_attr.op_mod);
    log_trace("                    table_type=0x%x\n", m_attr.type);
    log_trace("                    table_id=0x%x\n", m_table_id);

    m_is_initialized = true;
    return DPCP_OK;
}

status flow_table_prm::set_miss_action(void* in)
{
    uint32_t miss_table_id = 0;
    uint8_t miss_table_level = 0;

    switch (m_attr.def_miss_action) {
    // Will Set default miss behavior according to table type.
    case flow_table_miss_action::FT_MISS_ACTION_DEF:
        DEVX_SET(create_flow_table_in, in, flow_table_context.table_miss_action,
                 flow_table_miss_action::FT_MISS_ACTION_DEF);
        break;

    // Will forward the packet to the next flow table if did not match on any of the rules.
    case flow_table_miss_action::FT_MISS_ACTION_FWD: {
        std::shared_ptr<flow_table_prm> prm_table_miss =
            std::dynamic_pointer_cast<flow_table_prm>(m_attr.table_miss);
        if (!prm_table_miss || prm_table_miss->get_table_id(miss_table_id) != DPCP_OK ||
            prm_table_miss->get_table_level(miss_table_level) != DPCP_OK) {
            log_error("Flow table, miss flow table is not initialized\n");
            return DPCP_ERR_INVALID_PARAM;
        }
        if (miss_table_level <= m_attr.level) {
            log_error("Flow table, miss table level should be higher, miss_table_level=%d, "
                      "table_level=%d\n",
                      miss_table_level, m_attr.level);
            return DPCP_ERR_INVALID_PARAM;
        }
        DEVX_SET(create_flow_table_in, in, flow_table_context.table_miss_action,
                 flow_table_miss_action::FT_MISS_ACTION_FWD);
        DEVX_SET(create_flow_table_in, in, flow_table_context.table_miss_id, miss_table_id);
        break;
    }

    // Unsupported miss action.
    default:
        log_error("Flow table miss action %d is not supported\n", m_attr.def_miss_action);
        return DPCP_ERR_NO_SUPPORT;
    }

    return DPCP_OK;
}

status flow_table_prm::get_table_id(uint32_t& table_id) const
{
    status ret = get_flow_table_status();
    if (ret != DPCP_OK) {
        log_error("Failed to get Flow Table id, bad status %d\n", ret);
        return ret;
    }

    table_id = m_table_id;
    return DPCP_OK;
}

status flow_table_prm::query(flow_table_attr& attr)
{
    status ret = DPCP_OK;
    uint32_t table_miss = 0;
    uint32_t in[DEVX_ST_SZ_DW(query_flow_table_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(query_flow_table_out)] = {0};
    size_t outlen = sizeof(out);

    ret = get_flow_table_status();
    if (ret != DPCP_OK) {
        log_error("Failed to query Flow Table, bad status %d\n", ret);
        return ret;
    }

    // Set flow table in query buffer.
    DEVX_SET(query_flow_table_in, in, opcode, MLX5_CMD_OP_QUERY_FLOW_TABLE);
    DEVX_SET(query_flow_table_in, in, table_id, m_table_id);
    DEVX_SET(query_flow_table_in, in, table_type, m_attr.type);

    // Query flow table.
    ret = obj::query(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        log_error("Failed to query Flow Table HW object, table id=0x%x, ret=%d\n", m_table_id, ret);
        return ret;
    }

    // Set out put flow table attributes
    attr.def_miss_action = static_cast<flow_table_miss_action>(
        DEVX_GET(query_flow_table_out, out, flow_table_context.table_miss_action));
    if (DEVX_GET(query_flow_table_out, out, flow_table_context.decap_en)) {
        attr.flags |= flow_table_flags::FT_EN_DECAP;
    }
    if (DEVX_GET(query_flow_table_out, out, flow_table_context.reformat_en)) {
        attr.flags |= flow_table_flags::FT_EN_REFORMAT;
    }
    attr.level = DEVX_GET(query_flow_table_out, out, flow_table_context.level);
    attr.log_size = DEVX_GET(query_flow_table_out, out, flow_table_context.log_size);
    attr.op_mod = m_attr.op_mod;
    attr.type = m_attr.type;
    flow_table_prm* table = dynamic_cast<flow_table_prm*>(m_attr.table_miss.get());
    if (table) {
        ret = table->get_table_id(table_miss);
        if (ret != DPCP_OK) {
            log_error("Failed to get Flow Table id, ret=%d\n", ret);
            return ret;
        }
        if (DEVX_GET(query_flow_table_out, out, flow_table_context.table_miss_id) == table_miss) {
            attr.table_miss = m_attr.table_miss;
        }
    }

    log_trace("Flow table attr: flags=0x%zux\n", attr.flags);
    log_trace("                 def_miss_action=0x%x\n", attr.def_miss_action);
    log_trace("                 level=0x%x\n", attr.level);
    log_trace("                 log_size=0x%x\n", attr.log_size);
    log_trace("                 op_mod=0x%x\n", attr.op_mod);
    log_trace("                 table_miss=0x%x\n", table_miss);
    log_trace("                 table_type=0x%x\n", attr.type);

    return DPCP_OK;
}

status flow_table_prm::get_table_level(uint8_t& table_level) const
{
    status ret = get_flow_table_status();
    if (ret != DPCP_OK) {
        log_error("Failed to get Flow Table level, bad status %d\n", ret);
        return ret;
    }

    table_level = m_attr.level;
    return DPCP_OK;
}

status flow_table_prm::add_flow_group(const flow_group_attr& attr, std::weak_ptr<flow_group>& group)
{
    return create_flow_group<flow_group_prm>(attr, group);
}

////////////////////////////////////////////////////////////////////////
// flow_table_kernel implementation.                                  //
////////////////////////////////////////////////////////////////////////

flow_table_kernel::flow_table_kernel(dcmd::ctx* ctx, flow_table_type type)
    : flow_table(ctx, type)
{
}

status flow_table_kernel::create()
{
    m_is_initialized = true;
    return DPCP_OK;
}

status flow_table_kernel::query(flow_table_attr& attr)
{
    status ret = get_flow_table_status();
    if (ret != DPCP_OK) {
        log_error("Failed to query Flow Table, bad status %d\n", ret);
        return ret;
    }

    attr.type = m_type;
    attr.level = DEFAULT_LEVEL;
    attr.flags = 0;
    attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    attr.log_size = DEFAULT_LOG_SIZE;

    return DPCP_OK;
}

status flow_table_kernel::get_table_level(uint8_t& table_level) const
{
    status ret = get_flow_table_status();
    if (ret != DPCP_OK) {
        log_error("Failed to get Flow Table level, bad status %d\n", ret);
        return ret;
    }

    table_level = DEFAULT_LEVEL;
    return DPCP_OK;
}

status flow_table_kernel::add_flow_group(const flow_group_attr& attr,
                                         std::weak_ptr<flow_group>& group)
{
    return create_flow_group<flow_group_kernel>(attr, group);
}

} // namespace dpcp
