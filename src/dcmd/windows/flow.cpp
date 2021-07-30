/*
Copyright (C) Mellanox Technologies, Ltd. 2001-2020. ALL RIGHTS RESERVED.

This software product is a proprietary product of Mellanox Technologies, Ltd.
(the "Company") and all right, title, and interest in and to the software
product, including all associated intellectual property rights, are and shall
remain exclusively with the Company.  All rights in or to the software product
are licensed, not sold.  All rights not licensed are reserved.

This software product is governed by the End User License Agreement provided
with the software product.
*/

#include "stdafx.h"
#include "flow.h"

using namespace dcmd;

flow::flow(ctx_handle handle, struct flow_desc* desc)
{
    if (!desc->num_dst_tir) {
        throw DCMD_ENOTSUP;
    }
    size_t extra_dest_num = desc->num_dst_tir - 1; // the first is within devx_fs_rule_add_in
    uint32_t in_len = (uint32_t)(MLX5_ST_SZ_BYTES(devx_fs_rule_add_in) +
                                 MLX5_ST_SZ_BYTES(dest_format_struct) * extra_dest_num);
    auto in = new (std::nothrow) uint8_t[in_len];
    memset(in, 0, in_len);
    // priority
    DEVX_SET(devx_fs_rule_add_in, in, prio, desc->priority);
    // FlowTag Id
    DEVX_SET(devx_fs_rule_add_in, in, flow_tag, desc->flow_id);
    // Only outer header for now!!!
    DEVX_SET(devx_fs_rule_add_in, in, match_criteria_enable,
             1 << MLX5_CREATE_FLOW_GROUP_IN_MATCH_CRITERIA_ENABLE_OUTER_HEADERS);
    // mask
    memcpy(DEVX_ADDR_OF(devx_fs_rule_add_in, in, match_criteria), desc->match_criteria->match_buf,
           desc->match_criteria->match_sz);
    // value
    memcpy(DEVX_ADDR_OF(devx_fs_rule_add_in, in, match_value), desc->match_value->match_buf,
           desc->match_value->match_sz);
    // emdedded dest
    void* p_dst = DEVX_ADDR_OF(devx_fs_rule_add_in, in, dest);
    memcpy(p_dst, desc->dst_formats, MLX5_ST_SZ_BYTES(dest_format_struct));
    uint32_t dst_id = DEVX_GET(dest_format_struct, p_dst, destination_id);
    // extra dest
    log_trace("apply FR MCmask_sz %zd MCval_sz %zd prio %d, flow_tag 0x%x num_dst %zd dst_tir_n[0] "
              "0x%x\n",
              desc->match_criteria->match_sz, desc->match_value->match_sz, desc->priority,
              desc->flow_id, extra_dest_num + 1, dst_id);
    if (extra_dest_num) {
        DEVX_SET(devx_fs_rule_add_in, in, extra_dests_count, extra_dest_num);
        auto dests =
            (mlx5_ifc_dest_format_struct_bits*)(in + MLX5_ST_SZ_BYTES(devx_fs_rule_add_in));
        memcpy(dests, desc->dst_formats + 1, MLX5_ST_SZ_BYTES(dest_format_struct) * extra_dest_num);
    }
    m_handle = devx_fs_rule_add(handle, in, in_len);
    if (IS_ERR(m_handle)) {
        log_error("flow rule with prio %d wasn't added!\n", desc->priority);
        m_handle = nullptr;
        delete[] in;
        throw DCMD_ENOTSUP;
    }
    delete[] in;
}

flow::~flow()
{
    if (m_handle) {
        devx_fs_rule_del(m_handle);
        m_handle = nullptr;
    }
}
