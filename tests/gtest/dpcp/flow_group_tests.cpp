/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <memory>

#include "common/def.h"
#include "common/log.h"
#include "common/sys.h"
#include "common/base.h"

#include "dpcp_base.h"

using namespace dpcp;

class dpcp_flow_group : public dpcp_base {};

/**
 * @test dpcp_flow_group.ti_01_add_flow_group
 * @brief
 *    Check add_flow_group
 * @details
 */
TEST_F(dpcp_flow_group, ti_01_add_flow_group)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr ft_attr;
    ft_attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft_attr.flags = 0;
    ft_attr.level = 1;
    ft_attr.log_size = 10;
    ft_attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft_attr.type = flow_table_type::FT_RX;

    // Create flow table SW object;
    std::shared_ptr<flow_table> ft_obj;
    adapter_obj->create_flow_table(ft_attr, ft_obj);

    // Create flow table HW object;
    ret = ft_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Set flow group attributes.
    flow_group_attr fg_attr;
    fg_attr.end_flow_index = 1;
    fg_attr.start_flow_index = 0;
    fg_attr.match_criteria_enable = flow_group_match_criteria_enable::FG_MATCH_OUTER_HDR;

    uint64_t dmac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.dst_mac,  &dmac, sizeof(fg_attr.match_criteria.match_lyr2.dst_mac));
    uint64_t smac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.src_mac, &smac, sizeof(fg_attr.match_criteria.match_lyr2.src_mac));
    fg_attr.match_criteria.match_lyr2.ethertype = 0xFFFF;

    fg_attr.match_criteria.match_lyr3.dst_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.src_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.ip_protocol = 0xFF;

    fg_attr.match_criteria.match_lyr4.type = match_params_lyr_4_type::UDP;
    fg_attr.match_criteria.match_lyr4.dst_port = 0xFFFF;
    fg_attr.match_criteria.match_lyr4.src_port = 0xFFFF;

    // Create flow group SW object;
    std::weak_ptr<flow_group> fg_obj;
    ret = ft_obj->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj.lock().get(), nullptr);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_group.ti_02_create_flow_group
 * @brief
 *    Check create()
 * @details
 */
TEST_F(dpcp_flow_group, ti_02_create_flow_group)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr ft_attr;
    ft_attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft_attr.flags = 0;
    ft_attr.level = 1;
    ft_attr.log_size = 10;
    ft_attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft_attr.type = flow_table_type::FT_RX;

    // Create flow table SW object;
    std::shared_ptr<flow_table> ft_obj;
    adapter_obj->create_flow_table(ft_attr, ft_obj);


    // Create flow table HW object;
    ret = ft_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Set flow group attributes.
    flow_group_attr fg_attr;
    fg_attr.end_flow_index = 1;
    fg_attr.start_flow_index = 0;
    fg_attr.match_criteria_enable = flow_group_match_criteria_enable::FG_MATCH_OUTER_HDR;

    uint64_t dmac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.dst_mac,  &dmac, sizeof(fg_attr.match_criteria.match_lyr2.dst_mac));
    uint64_t smac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.src_mac, &smac, sizeof(fg_attr.match_criteria.match_lyr2.src_mac));
    fg_attr.match_criteria.match_lyr2.ethertype = 0xFFFF;

    fg_attr.match_criteria.match_lyr3.dst_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.src_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.ip_protocol = 0xFF;

    fg_attr.match_criteria.match_lyr4.type = match_params_lyr_4_type::UDP;
    fg_attr.match_criteria.match_lyr4.dst_port = 0xFFFF;
    fg_attr.match_criteria.match_lyr4.src_port = 0xFFFF;

    // Create flow group SW object;
    std::weak_ptr<flow_group> fg_obj;
    ret = ft_obj->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj.lock().get(), nullptr);

    // Create flow group HW object;
    ret = fg_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_group.ti_03_get_group_id
 * @brief
 *    Check get_group_id()
 * @details
 */
TEST_F(dpcp_flow_group, ti_03_get_group_id)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr ft_attr;
    ft_attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft_attr.flags = 0;
    ft_attr.level = 1;
    ft_attr.log_size = 10;
    ft_attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft_attr.type = flow_table_type::FT_RX;

    // Create flow table SW object;
    std::shared_ptr<flow_table> ft_obj;
    adapter_obj->create_flow_table(ft_attr, ft_obj);

    // Create flow table HW object;
    ret = ft_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Set flow group attributes.
    flow_group_attr fg_attr;
    fg_attr.end_flow_index = 1;
    fg_attr.start_flow_index = 0;
    fg_attr.match_criteria_enable = flow_group_match_criteria_enable::FG_MATCH_OUTER_HDR;

    uint64_t dmac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.dst_mac,  &dmac, sizeof(fg_attr.match_criteria.match_lyr2.dst_mac));
    uint64_t smac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.src_mac, &smac, sizeof(fg_attr.match_criteria.match_lyr2.src_mac));
    fg_attr.match_criteria.match_lyr2.ethertype = 0xFFFF;

    fg_attr.match_criteria.match_lyr3.dst_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.src_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.ip_protocol = 0xFF;

    fg_attr.match_criteria.match_lyr4.type = match_params_lyr_4_type::UDP;
    fg_attr.match_criteria.match_lyr4.dst_port = 0xFFFF;
    fg_attr.match_criteria.match_lyr4.src_port = 0xFFFF;

    // Create flow group SW object;
    std::weak_ptr<flow_group> fg_obj;
    ret = ft_obj->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj.lock().get(), nullptr);

    // Create flow group HW object;
    ret = fg_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t fg_id = 0;
    ret = fg_obj.lock()->get_id(fg_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_id, 0U);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_group.ti_04_remove_flow_group_01
 * @brief
 *    Check remove_flow_group()
 * @details
 */
TEST_F(dpcp_flow_group, ti_04_remove_flow_group_01)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr ft_attr;
    ft_attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft_attr.flags = 0;
    ft_attr.level = 1;
    ft_attr.log_size = 10;
    ft_attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft_attr.type = flow_table_type::FT_RX;

    // Create flow table SW object;
    std::shared_ptr<flow_table> ft_obj;
    adapter_obj->create_flow_table(ft_attr, ft_obj);

    // Create flow table HW object.
    ret = ft_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Set flow group attributes.
    flow_group_attr fg_attr;
    fg_attr.end_flow_index = 1;
    fg_attr.start_flow_index = 0;
    fg_attr.match_criteria_enable = flow_group_match_criteria_enable::FG_MATCH_OUTER_HDR;

    uint64_t dmac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.dst_mac,  &dmac, sizeof(fg_attr.match_criteria.match_lyr2.dst_mac));
    uint64_t smac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.src_mac, &smac, sizeof(fg_attr.match_criteria.match_lyr2.src_mac));
    fg_attr.match_criteria.match_lyr2.ethertype = 0xFFFF;

    fg_attr.match_criteria.match_lyr3.dst_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.src_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.ip_protocol = 0xFF;

    fg_attr.match_criteria.match_lyr4.type = match_params_lyr_4_type::UDP;
    fg_attr.match_criteria.match_lyr4.dst_port = 0xFFFF;
    fg_attr.match_criteria.match_lyr4.src_port = 0xFFFF;

    // Create flow group SW object.
    std::weak_ptr<flow_group> fg_obj;
    ret = ft_obj->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj.lock().get(), nullptr);

    // Create flow group HW object.
    ret = fg_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Remove flow group.
    ret = ft_obj->remove_flow_group(fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(fg_obj.lock().get(), nullptr);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_group.ti_05_remove_flow_group_02
 * @brief
 *    Check remove_flow_group() twise on the same group
 * @details
 */
TEST_F(dpcp_flow_group, ti_05_remove_flow_group_02)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr ft_attr;
    ft_attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft_attr.flags = 0;
    ft_attr.level = 1;
    ft_attr.log_size = 10;
    ft_attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft_attr.type = flow_table_type::FT_RX;

    // Create flow table SW object;
    std::shared_ptr<flow_table> ft_obj;
    adapter_obj->create_flow_table(ft_attr, ft_obj);

    // Create flow table HW object.
    ret = ft_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Set flow group attributes.
    flow_group_attr fg_attr;
    fg_attr.end_flow_index = 1;
    fg_attr.start_flow_index = 0;
    fg_attr.match_criteria_enable = flow_group_match_criteria_enable::FG_MATCH_OUTER_HDR;

    uint64_t dmac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.dst_mac,  &dmac, sizeof(fg_attr.match_criteria.match_lyr2.dst_mac));
    uint64_t smac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.src_mac, &smac, sizeof(fg_attr.match_criteria.match_lyr2.src_mac));
    fg_attr.match_criteria.match_lyr2.ethertype = 0xFFFF;

    fg_attr.match_criteria.match_lyr3.dst_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.src_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.ip_protocol = 0xFF;

    fg_attr.match_criteria.match_lyr4.type = match_params_lyr_4_type::UDP;
    fg_attr.match_criteria.match_lyr4.dst_port = 0xFFFF;
    fg_attr.match_criteria.match_lyr4.src_port = 0xFFFF;

    // Create flow group SW object.
    std::weak_ptr<flow_group> fg_obj;
    ret = ft_obj->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj.lock().get(), nullptr);

    // Create flow group HW object.
    ret = fg_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Remove flow group.
    std::weak_ptr<flow_group> fg_obj_tmp = fg_obj;
    ret = ft_obj->remove_flow_group(fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(fg_obj.lock().get(), nullptr);

    // Remove flow group second time.
    ret = ft_obj->remove_flow_group(fg_obj_tmp);
    ASSERT_NE(DPCP_OK, ret);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_group.ti_06_create_flow_group_with_meta_register_c_0
 * @brief
 *    Check add_flow_group
 * @details
 */
TEST_F(dpcp_flow_group, ti_06_create_flow_group_with_meta_register_c_0)
{
    status ret = DPCP_OK;

    // Get adapter:
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes:
    flow_table_attr ft_attr;
    ft_attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft_attr.flags = 0;
    ft_attr.level = 1;
    ft_attr.log_size = 10;
    ft_attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft_attr.type = flow_table_type::FT_RX;

    // Create flow table SW object:
    std::shared_ptr<flow_table> ft_obj;
    adapter_obj->create_flow_table(ft_attr, ft_obj);

    // Create flow table HW object:
    ret = ft_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Set flow group attributes:
    flow_group_attr fg_attr;
    fg_attr.end_flow_index = 1;
    fg_attr.start_flow_index = 0;
    fg_attr.match_criteria_enable = flow_group_match_criteria_enable::FG_MATCH_METADATA_REG_C_0;
    fg_attr.match_criteria.match_metadata_reg_c_0 = 0xFFFFFFFF;

    // Create flow group SW object:
    std::weak_ptr<flow_group> fg_obj;
    ret = ft_obj->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj.lock().get(), nullptr);

    // Create flow group HW object;
    ret = fg_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    delete adapter_obj;
}
