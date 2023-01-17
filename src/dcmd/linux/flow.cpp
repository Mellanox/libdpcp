/*
 * Copyright (c) 2019-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <string>

#include <utils/os.h>
#include "dcmd/dcmd.h"

using namespace dcmd;

flow::flow(ctx_handle handle, struct flow_desc* desc)
{
    struct ibv_flow* ib_flow;
    struct mlx5dv_flow_matcher* matcher = NULL;
    struct mlx5dv_flow_matcher_attr matcher_attr;

    memset(&matcher_attr, 0, sizeof(matcher_attr));
    matcher_attr.type = IBV_FLOW_ATTR_NORMAL;
    matcher_attr.flags = 0;
    matcher_attr.priority = desc->priority;
    // Only outer header for now!!!
    matcher_attr.match_criteria_enable = 1
        << MLX5_CREATE_FLOW_GROUP_IN_MATCH_CRITERIA_ENABLE_OUTER_HEADERS;
    matcher_attr.match_mask = (struct mlx5dv_flow_match_parameters*)desc->match_criteria;
    matcher_attr.comp_mask = MLX5DV_FLOW_MATCHER_MASK_FT_TYPE;
    matcher_attr.ft_type = MLX5_IB_UAPI_FLOW_TABLE_TYPE_NIC_RX;

    matcher = mlx5dv_create_flow_matcher(handle, &matcher_attr);
    if (NULL == matcher) {
        throw DCMD_ENOTSUP;
    }

    size_t num_actions = (desc->flow_id ? (desc->num_dst_obj + 1) : desc->num_dst_obj);
    num_actions += desc->modify_actions ? 1 : 0;
    struct mlx5dv_flow_action_attr actions_attr[num_actions];
    int i = 0;
    int j = 0;

    if (desc->flow_id) {
        actions_attr[i].type = MLX5DV_FLOW_ACTION_TAG;
        actions_attr[i].tag_value = desc->flow_id;
        i++;
    }
    if (desc->modify_actions) {
        actions_attr[i].type = MLX5DV_FLOW_ACTION_IBV_FLOW_ACTION;
        actions_attr[i].action = mlx5dv_create_flow_action_modify_header(
            handle, sizeof(modify_action) * desc->num_of_actions, (uint64_t*)desc->modify_actions,
            MLX5_IB_UAPI_FLOW_TABLE_TYPE_NIC_RX);
        if (!actions_attr[i].action) {
            throw DCMD_ENOTSUP;
        }
        i++;
    }
    for (j = 0; j < (int)desc->num_dst_obj; j++, i++) {
        actions_attr[i].type = MLX5DV_FLOW_ACTION_DEST_DEVX;
        actions_attr[i].obj = desc->dst_obj[j];
    }

    ib_flow = mlx5dv_create_flow(matcher, (struct mlx5dv_flow_match_parameters*)desc->match_value,
                                 num_actions, actions_attr);
    if (NULL == ib_flow) {
        mlx5dv_destroy_flow_matcher(matcher);
        throw DCMD_ENOTSUP;
    }
    m_matcher = matcher;
    m_handle = ib_flow;
}

flow::~flow()
{
    if (m_handle) {
        ibv_destroy_flow(m_handle);
        m_handle = nullptr;
        mlx5dv_destroy_flow_matcher(m_matcher);
        m_matcher = nullptr;
    }
}
