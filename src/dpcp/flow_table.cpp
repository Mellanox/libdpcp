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

#include <algorithm>

#include "dpcp/internal.h"
#include "utils/os.h"

namespace dpcp {

// Constructor should be used only for DEVX flow tables.
flow_table::flow_table(dcmd::ctx* ctx, const flow_table_attr& attr)
    : obj(ctx)
    , m_attr(attr)
    , m_table_id()
    , m_is_initialized(false)
    , m_is_kernel_table(false)
{
}

// Constructor should be used only for Kernel flow tables.
flow_table::flow_table(dcmd::ctx* ctx, flow_table_type type)
    : obj(ctx)
    , m_attr()
    , m_table_id()
    , m_is_initialized(true)
    , m_is_kernel_table(true)
{
    m_attr.type = type;
}

status flow_table::set_miss_action(void* in)
{
    uint32_t miss_table_id = 0;
    uint8_t miss_table_level = 0;

    switch (m_attr.def_miss_action) {
        // Will drop packet if did not match on any of the rules.
        case flow_table_miss_action::FT_MISS_ACTION_DEF:
            DEVX_SET(create_flow_table_in, in,flow_table_context.table_miss_action,
                flow_table_miss_action::FT_MISS_ACTION_DEF);
            break;

        // Will forward the packet to the next flow table if did not match on any of the rules.
        case flow_table_miss_action::FT_MISS_ACTION_FWD:
            if (!m_attr.table_miss
                    || m_attr.table_miss->get_table_id(miss_table_id) != DPCP_OK
                    || m_attr.table_miss->get_table_level(miss_table_level) != DPCP_OK) {
                log_error("Flow table, miss flow table is not initialized\n");
                return DPCP_ERR_INVALID_PARAM;
            }
            if (miss_table_level <= m_attr.level) {
                log_error("Flow table, miss table level should be higher, miss_table_level=%d, table_level=%d\n",
                        miss_table_level, m_attr.level);
                return DPCP_ERR_INVALID_PARAM;
            }
            DEVX_SET(create_flow_table_in, in, flow_table_context.table_miss_action,
                flow_table_miss_action::FT_MISS_ACTION_FWD);
            DEVX_SET(create_flow_table_in, in,flow_table_context.table_miss_id, miss_table_id);
            break;

        // Unsupported miss action.
        default:
            log_error("Flow table miss action %d is not supported\n", m_attr.def_miss_action);
            return DPCP_ERR_NO_SUPPORT;
    }

    return DPCP_OK;
}

status flow_table::create()
{
    status ret = DPCP_OK;
    uint32_t in[DEVX_ST_SZ_DW(create_flow_table_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(create_flow_table_out)] = {0};
    size_t outlen = sizeof(out);

    if(m_is_kernel_table) {
        return DPCP_OK;
    }
    if (m_is_initialized) {
        log_warn("Flow table was already created\n");
        return DPCP_ERR_CREATE;
    }
    if (m_attr.level == 0) {
        log_error("Flow table level was set to 0\n");
        return DPCP_ERR_INVALID_PARAM;
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
        return ret;
    }
    m_table_id = DEVX_GET(create_flow_table_out, out, table_id);
    m_is_initialized = true;

    log_trace("Flow table created: flags=0x%x\n", m_attr.flags);
    log_trace("                    def_miss_action=0x%x\n", m_attr.def_miss_action);
    log_trace("                    level=0x%x\n", m_attr.level);
    log_trace("                    log_size=0x%x\n", m_attr.log_size);
    log_trace("                    op_mod=0x%x\n", m_attr.op_mod);
    log_trace("                    table_type=0x%x\n", m_attr.type);
    log_trace("                    table_id=0x%x\n", m_table_id);

    return ret;
}

status flow_table::get_table_id(uint32_t& table_id) const
{
    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }
    if(m_is_kernel_table) {
        log_warn("Root flow table do not support get_table_id()\n");
        return DPCP_ERR_NO_SUPPORT;
    }

    table_id = m_table_id;
    return DPCP_OK;
}

status flow_table::get_table_type(flow_table_type& table_type) const
{
    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }

    table_type = m_attr.type;
    return DPCP_OK;
}

flow_table::~flow_table()
{
    std::for_each(m_groups.begin(), m_groups.end(), [](flow_group* fg) {delete fg;} );

    if (!m_is_kernel_table && m_is_initialized) {
        obj::destroy();
    }
}

status flow_table::query(flow_table_attr& attr)
{
    status ret = DPCP_OK;
    uint32_t table_miss = 0;
    uint32_t in[DEVX_ST_SZ_DW(query_flow_table_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(query_flow_table_out)] = {0};
    size_t outlen = sizeof(out);

    if (!m_is_initialized) {
        log_error("Flow Table is invalid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    // Set flow table in query buffer.
    DEVX_SET(query_flow_table_in, in, opcode, MLX5_CMD_OP_QUERY_FLOW_TABLE);
    DEVX_SET(query_flow_table_in, in, table_id, m_table_id);
    DEVX_SET(query_flow_table_in, in, table_type, m_attr.type);

    // Query flow table.
    ret = obj::query(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        log_error("Flow Table query() table id=0x%x ret=%d\n", m_table_id, ret);
        return ret;
    }

    // Set out put flow table attributes
    attr.def_miss_action =
        static_cast<flow_table_miss_action>(DEVX_GET(query_flow_table_out, out, flow_table_context.table_miss_action));
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
    if (m_attr.table_miss) {
        m_attr.table_miss->get_table_id(table_miss);
        if (DEVX_GET(query_flow_table_out, out, flow_table_context.table_miss_id) == table_miss) {
            attr.table_miss = m_attr.table_miss;
        }
    }

    log_trace("Flow table attr: flags=0x%x\n", attr.flags);
    log_trace("                 def_miss_action=0x%x\n", attr.def_miss_action);
    log_trace("                 level=0x%x\n", attr.level);
    log_trace("                 log_size=0x%x\n", attr.log_size);
    log_trace("                 op_mod=0x%x\n", attr.op_mod);
    log_trace("                 table_miss=0x%x\n", table_miss);
    log_trace("                 table_type=0x%x\n", attr.type);

    return DPCP_OK;
}

bool flow_table::is_kernel_table() const
{
    return m_is_kernel_table;
}

status flow_table::get_table_level(uint8_t& table_level) const
{
    if (!m_is_initialized && !m_is_kernel_table) {
        return DPCP_ERR_NOT_APPLIED;
    }

    table_level = m_attr.level;
    return DPCP_OK;
}

status flow_table::add_flow_group(const flow_group_attr& attr, flow_group*& group)
{
    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }

    flow_group* fg = new (std::nothrow) flow_group(get_ctx(), attr, this);
    if (!fg) {
        log_error("Flow group allocation failed\n");
        return DPCP_ERR_NO_MEMORY;
    }

    auto ret = m_groups.insert(fg);
    if (!ret.second) {
        delete fg;
        log_error("Flow group placement failed\n");
        return DPCP_ERR_NO_MEMORY;
    }
    group = fg;

    return DPCP_OK;
}

status flow_table::remove_flow_group(flow_group*& group)
{
    if (!m_is_initialized) {
        return DPCP_ERR_NOT_APPLIED;
    }

    if (m_groups.erase(group) != 1) {
        log_error("Flow group %p do not exist in this table\n", group);
        return DPCP_ERR_INVALID_PARAM;
    }

    group = nullptr;
    return DPCP_OK;
}

} // namespace dpcp

