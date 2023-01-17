/*
 * Copyright (c) 2020-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include "stdafx.h"
#include "dcmd/dcmd.h"
#include "flow.h"

using namespace dcmd;

flow::flow(ctx_handle handle, struct flow_desc* desc)
{
    if (!desc->num_dst_obj) {
        throw DCMD_ENOTSUP;
    }
    size_t extra_dest_num = desc->num_dst_obj - 1; // the first is within devx_fs_rule_add_in
    uint32_t in_len = (uint32_t)(DEVX_ST_SZ_BYTES(devx_fs_rule_add_in) +
                                 DEVX_ST_SZ_BYTES(dest_format_struct) * extra_dest_num);
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
    void* prm_mc = DEVX_ADDR_OF(devx_fs_rule_add_in, in, match_criteria);
    memcpy(prm_mc, desc->match_criteria->match_buf, desc->match_criteria->match_sz);
    // TODO: WinOF2 doesn't accept ip_version so far, delete when enabled
    void* hdr = DEVX_ADDR_OF(fte_match_param, prm_mc, outer_headers);
    DEVX_SET(fte_match_set_lyr_2_4, hdr, ip_version, 0x0);

    // value
    void* prm_val = DEVX_ADDR_OF(devx_fs_rule_add_in, in, match_value);
    memcpy(prm_val, desc->match_value->match_buf, desc->match_value->match_sz);
    // TODO: WinOF2 doesn't accept ip_version so far, delete when enabled
    hdr = DEVX_ADDR_OF(fte_match_param, prm_val, outer_headers);
    DEVX_SET(fte_match_set_lyr_2_4, hdr, ip_version, 0x0);

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

    // modify action
    if (desc->num_of_actions != 0) {
        m_modify_obj.reset(new obj(handle, &desc->modify_acttions_obj_desc));
        uint32_t modify_obj_id = DEVX_GET(alloc_modify_header_context_out,
                                          desc->modify_acttions_obj_desc.out, modify_header_id);
        DEVX_SET(devx_fs_rule_add_in, in, modify_header_id, modify_obj_id);
        DEVX_SET(devx_fs_rule_add_in, in, has_modify_header_id, 1);
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
