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

#include <memory>

#include "common/def.h"
#include "common/log.h"
#include "common/sys.h"
#include "common/base.h"

#include "dpcp_base.h"

using namespace dpcp;

class dpcp_flow_rule_ex : public dpcp_base {};

/**
 * @test dpcp_flow_rule_ex.ti_01_add_flow_rule
 * @brief
 *    Check add_flow_group
 * @details
 */
TEST_F(dpcp_flow_rule_ex, ti_01_add_flow_rule)
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

    // Set flow table attributes.
    flow_table_attr ft_attr_fwd;
    ft_attr_fwd.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft_attr_fwd.flags = 0;
    ft_attr_fwd.level = 2;
    ft_attr_fwd.log_size = 10;
    ft_attr_fwd.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft_attr_fwd.type = flow_table_type::FT_RX;

    // Create flow table SW object;
    std::shared_ptr<flow_table> ft_fwd_obj;
    adapter_obj->create_flow_table(ft_attr_fwd, ft_fwd_obj);
    ASSERT_EQ(DPCP_OK, ret);

    // Create flow table HW object;
    ret = ft_fwd_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    flow_action_generator& action_gen = adapter_obj->get_flow_action_generator();
    std::vector<forwardable_obj*> dests;
    dests.push_back(ft_fwd_obj.get());
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_fwd(dests));

    flow_rule_attr_ex fr_attr;
    fr_attr.priority = 3;
    fr_attr.flow_index = 0;

    uint64_t dmac_val = 0x0cc47a515dbc;
    memcpy(fr_attr.match_value.match_lyr2.dst_mac,  &dmac_val, sizeof(fr_attr.match_value.match_lyr2.dst_mac));
    uint64_t smac_val = 0x0cc47a515dbd;
    memcpy(fr_attr.match_value.match_lyr2.src_mac, &smac_val, sizeof(fr_attr.match_value.match_lyr2.src_mac));
    fr_attr.match_value.match_lyr2.ethertype = 0x800;

    fr_attr.match_value.match_lyr3.dst_ip = 0x0ad1ff8a;
    fr_attr.match_value.match_lyr3.src_ip = 0x0ad1ff8b;
    fr_attr.match_value.match_lyr3.ip_protocol = 0x11;

    fr_attr.match_value.match_lyr4.type = match_params_lyr_4_type::UDP;
    fr_attr.match_value.match_lyr4.dst_port = 0xc350;
    fr_attr.match_value.match_lyr4.src_port = 0xc351;

    fr_attr.actions.push_back(fa_fwd);
    std::weak_ptr<flow_rule_ex> fr_obj;
    ret = fg_obj.lock()->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj.lock().get(), nullptr);

    ret = fr_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_rule_ex.ti_02_add_flow_rule_reformat
 * @brief
 *    Check add_flow_group with reformat insert headert
 * @details
 */
TEST_F(dpcp_flow_rule_ex, ti_02_add_flow_rule_reformat)
{
    status ret = DPCP_OK;

    // Get adapter.
    std::unique_ptr<adapter> adapter_obj(OpenAdapter());
    ASSERT_NE(nullptr, adapter_obj);

    adapter_hca_capabilities caps;
    ret = adapter_obj->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    // Check if reformat insert header is supported, otherwise skip test.
    if (!caps.flow_table_caps.receive.is_flow_action_reformat_from_type_insert_supported) {
        return;
    }

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

    // Set flow table attributes.
    flow_table_attr ft_attr_fwd;
    ft_attr_fwd.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft_attr_fwd.flags = 0;
    ft_attr_fwd.level = 2;
    ft_attr_fwd.log_size = 10;
    ft_attr_fwd.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft_attr_fwd.type = flow_table_type::FT_RX;

    // Create flow table SW object;
    std::shared_ptr<flow_table> ft_fwd_obj;
    adapter_obj->create_flow_table(ft_attr_fwd, ft_fwd_obj);

    // Create flow table HW object;
    ret = ft_fwd_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    flow_action_generator& action_gen = adapter_obj->get_flow_action_generator();
    std::vector<forwardable_obj*> dests;
    dests.push_back(ft_fwd_obj.get());
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_fwd(dests));

    flow_action_reformat_attr insert_hdr_attr {};
    insert_hdr_attr.insert.type = flow_action_reformat_type::INSERT_HDR;
    insert_hdr_attr.insert.start_hdr = dpcp::flow_action_reformat_anchor::MAC_START;
    insert_hdr_attr.insert.offset = 22;
    insert_hdr_attr.insert.data_len = 28;
    insert_hdr_attr.insert.data = new uint8_t[28];

    std::shared_ptr<flow_action> fa_insert(action_gen.create_reformat(insert_hdr_attr));
    delete[] (uint8_t*)insert_hdr_attr.insert.data;

    flow_rule_attr_ex fr_attr;
    fr_attr.priority = 3;
    fr_attr.flow_index = 0;

    uint64_t dmac_val = 0x0cc47a515dbc;
    memcpy(fr_attr.match_value.match_lyr2.dst_mac,  &dmac_val, sizeof(fr_attr.match_value.match_lyr2.dst_mac));
    uint64_t smac_val = 0x0cc47a515dbd;
    memcpy(fr_attr.match_value.match_lyr2.src_mac, &smac_val, sizeof(fr_attr.match_value.match_lyr2.src_mac));
    fr_attr.match_value.match_lyr2.ethertype = 0x800;

    fr_attr.match_value.match_lyr3.dst_ip = 0x0ad1ff8a;
    fr_attr.match_value.match_lyr3.src_ip = 0x0ad1ff8b;
    fr_attr.match_value.match_lyr3.ip_protocol = 0x11;

    fr_attr.match_value.match_lyr4.type = match_params_lyr_4_type::UDP;
    fr_attr.match_value.match_lyr4.dst_port = 0xc350;
    fr_attr.match_value.match_lyr4.src_port = 0xc351;

    fr_attr.actions.push_back(fa_fwd);
    fr_attr.actions.push_back(fa_insert);
    std::weak_ptr<flow_rule_ex> fr_obj;
    ret = fg_obj.lock()->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj.lock().get(), nullptr);

    ret = fr_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);
}


/**
 * @test dpcp_flow_rule_ex.ti_03_add_flow_rule_modify_set
 * @brief
 *    Check add_flow_group with modify set
 * @details
 */
TEST_F(dpcp_flow_rule_ex, ti_03_add_flow_rule_modify)
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
    ret = adapter_obj->create_flow_table(ft_attr, ft_obj);
    ASSERT_EQ(DPCP_OK, ret);

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

    // Set flow table attributes.
    flow_table_attr ft_attr_fwd;
    ft_attr_fwd.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft_attr_fwd.flags = 0;
    ft_attr_fwd.level = 2;
    ft_attr_fwd.log_size = 10;
    ft_attr_fwd.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft_attr_fwd.type = flow_table_type::FT_RX;

    // Create flow table SW object;
    std::shared_ptr<flow_table> ft_fwd_obj;
    ret = adapter_obj->create_flow_table(ft_attr_fwd, ft_fwd_obj);
    ASSERT_EQ(DPCP_OK, ret);

    // Create flow table HW object;
    ret = ft_fwd_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    flow_action_generator& action_gen = adapter_obj->get_flow_action_generator();
    std::vector<forwardable_obj*> dests;
    dests.push_back(ft_fwd_obj.get());
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_fwd(dests));

    flow_action_modify_type_attr set_attr {};
    set_attr.set.type = flow_action_modify_type::SET;
    set_attr.set.data = 0x800;
    set_attr.set.field = flow_action_modify_field::OUT_ETHERTYPE;
    set_attr.set.length = 0x10;
    set_attr.set.offset = 0;

    flow_action_modify_attr modify_hdr_attr;
    modify_hdr_attr.table_type = flow_table_type::FT_RX;
    modify_hdr_attr.actions.push_back(set_attr);

    std::shared_ptr<flow_action> fa_modify(action_gen.create_modify(modify_hdr_attr));

    flow_rule_attr_ex fr_attr;
    fr_attr.priority = 3;
    fr_attr.flow_index = 0;

    uint64_t dmac_val = 0x0cc47a515dbc;
    memcpy(fr_attr.match_value.match_lyr2.dst_mac,  &dmac_val, sizeof(fr_attr.match_value.match_lyr2.dst_mac));
    uint64_t smac_val = 0x0cc47a515dbd;
    memcpy(fr_attr.match_value.match_lyr2.src_mac, &smac_val, sizeof(fr_attr.match_value.match_lyr2.src_mac));
    fr_attr.match_value.match_lyr2.ethertype = 0x800;

    fr_attr.match_value.match_lyr3.dst_ip = 0x0ad1ff8a;
    fr_attr.match_value.match_lyr3.src_ip = 0x0ad1ff8b;
    fr_attr.match_value.match_lyr3.ip_protocol = 0x11;

    fr_attr.match_value.match_lyr4.type = match_params_lyr_4_type::UDP;
    fr_attr.match_value.match_lyr4.dst_port = 0xc350;
    fr_attr.match_value.match_lyr4.src_port = 0xc351;

    fr_attr.actions.push_back(fa_fwd);
    fr_attr.actions.push_back(fa_modify);
    std::weak_ptr<flow_rule_ex> fr_obj;
    ret = fg_obj.lock()->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj.lock().get(), nullptr);

    ret = fr_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_rule_ex.ti_04_add_flow_rule_kernel_fwd_table
 * @brief
 *    Check add_flow_rule kernel forward to flow table.
 * @details
 */
TEST_F(dpcp_flow_rule_ex, ti_04_add_flow_rule_kernel_fwd_table)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    std::shared_ptr<flow_table> root_table(adapter_obj->get_root_table(flow_table_type::FT_RX));
    ASSERT_NE(root_table.get(), nullptr);

    // Set flow group attributes.
    flow_group_attr fg_attr;
    fg_attr.end_flow_index = 1000;
    fg_attr.start_flow_index = 0;
    fg_attr.match_criteria_enable = flow_group_match_criteria_enable::FG_MATCH_OUTER_HDR;

#if defined(__linux__)
    uint64_t dmac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.dst_mac,  &dmac, sizeof(fg_attr.match_criteria.match_lyr2.dst_mac));
    uint64_t smac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.src_mac, &smac, sizeof(fg_attr.match_criteria.match_lyr2.src_mac));
#endif
    fg_attr.match_criteria.match_lyr2.ethertype = 0xFFFF;

    fg_attr.match_criteria.match_lyr3.dst_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.src_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.ip_protocol = 0xFF;

    fg_attr.match_criteria.match_lyr4.type = match_params_lyr_4_type::UDP;
    fg_attr.match_criteria.match_lyr4.dst_port = 0xFFFF;
#if defined(__linux__)
    fg_attr.match_criteria.match_lyr4.src_port = 0xFFFF;
#endif

    // Create flow group SW object;
    std::weak_ptr<flow_group> fg_obj;
    ret = root_table->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj.lock().get(), nullptr);

    ret = fg_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Set flow table attributes.
    flow_table_attr ft_attr_fwd;
    ft_attr_fwd.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft_attr_fwd.flags = 0;
    ft_attr_fwd.level = 100;
    ft_attr_fwd.log_size = 10;
    ft_attr_fwd.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft_attr_fwd.type = flow_table_type::FT_RX;

    // Create flow table SW object;
    std::shared_ptr<flow_table> ft_fwd_obj;
    adapter_obj->create_flow_table(ft_attr_fwd, ft_fwd_obj);
    ASSERT_EQ(DPCP_OK, ret);

    // Create flow table HW object;
    ret = ft_fwd_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    flow_action_generator& action_gen = adapter_obj->get_flow_action_generator();
    std::vector<forwardable_obj*> dests;
    dests.push_back(ft_fwd_obj.get());
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_fwd(dests));

    flow_rule_attr_ex fr_attr;
    fr_attr.priority = 3;
    fr_attr.flow_index = 0;

#if defined(__linux__)
    uint64_t dmac_val = 0x0cc47a515dbc;
    memcpy(fr_attr.match_value.match_lyr2.dst_mac,  &dmac_val, sizeof(fr_attr.match_value.match_lyr2.dst_mac));
    uint64_t smac_val = 0x0cc47a515dbd;
    memcpy(fr_attr.match_value.match_lyr2.src_mac, &smac_val, sizeof(fr_attr.match_value.match_lyr2.src_mac));
#endif
    fr_attr.match_value.match_lyr2.ethertype = 0x800;

    fr_attr.match_value.match_lyr3.dst_ip = 0x0ad1ff8a;
    fr_attr.match_value.match_lyr3.src_ip = 0x0ad1ff8b;
    fr_attr.match_value.match_lyr3.ip_protocol = 0x11;

    fr_attr.match_value.match_lyr4.type = match_params_lyr_4_type::UDP;
    fr_attr.match_value.match_lyr4.dst_port = 0xc350;
#if defined(__linux__)
    fr_attr.match_value.match_lyr4.src_port = 0xc351;
#endif

    fr_attr.actions.push_back(fa_fwd);
    std::weak_ptr<flow_rule_ex> fr_obj;
    ret = fg_obj.lock()->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj.lock().get(), nullptr);

    ret = fr_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_rule_ex.ti_05_add_flow_rule_kernel_fwd_tir
 * @brief
 *    Check add_flow_rule kernel forward to tir
 * @details
 */
TEST_F(dpcp_flow_rule_ex, ti_05_add_flow_rule_kernel_fwd_tir)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    std::shared_ptr<flow_table> root_table(adapter_obj->get_root_table(flow_table_type::FT_RX));
    ASSERT_NE(root_table.get(), nullptr);

    // Set flow group attributes.
    flow_group_attr fg_attr;
    fg_attr.end_flow_index = 1;
    fg_attr.start_flow_index = 0;
    fg_attr.match_criteria_enable = flow_group_match_criteria_enable::FG_MATCH_OUTER_HDR;

#if defined(__linux__)
    uint64_t dmac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.dst_mac,  &dmac, sizeof(fg_attr.match_criteria.match_lyr2.dst_mac));
    uint64_t smac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.src_mac, &smac, sizeof(fg_attr.match_criteria.match_lyr2.src_mac));
#endif
    fg_attr.match_criteria.match_lyr2.ethertype = 0xFFFF;

    fg_attr.match_criteria.match_lyr3.dst_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.src_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.ip_protocol = 0xFF;

    fg_attr.match_criteria.match_lyr4.type = match_params_lyr_4_type::UDP;
    fg_attr.match_criteria.match_lyr4.dst_port = 0xFFFF;
#if defined(__linux__)
    fg_attr.match_criteria.match_lyr4.src_port = 0xFFFF;
#endif

    // Create flow group SW object;
    std::weak_ptr<flow_group> fg_obj;
    ret = root_table->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj.lock().get(), nullptr);

    ret = fg_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Create tir object to forward to
    ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = adapter_obj->get_td();
    ASSERT_NE(0U, tdn);

    striding_rq* srq_obj = open_str_rq(adapter_obj, m_rqp);
    ASSERT_NE(nullptr, srq_obj);

    uint32_t rqn = 0;
    ret = srq_obj->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rqn);
    log_trace("tdn: 0x%x rqn: 0x%x\n", tdn, rqn);

    tir tir_obj(adapter_obj->get_ctx());
    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = TIR_ATTR_INLINE_RQN | TIR_ATTR_TRANSPORT_DOMAIN;
    tir_attr.inline_rqn = rqn;
    tir_attr.transport_domain = tdn;
    ret = tir_obj.create(tir_attr);
    ASSERT_EQ(DPCP_OK, ret);



    flow_action_generator& action_gen = adapter_obj->get_flow_action_generator();
    std::vector<forwardable_obj*> dests;
    dests.push_back(&tir_obj);
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_fwd(dests));

    flow_rule_attr_ex fr_attr;
    fr_attr.priority = 3;
    fr_attr.flow_index = 0;

#if defined(__linux__)
    uint64_t dmac_val = 0x0cc47a515dbc;
    memcpy(fr_attr.match_value.match_lyr2.dst_mac,  &dmac_val, sizeof(fr_attr.match_value.match_lyr2.dst_mac));
    uint64_t smac_val = 0x0cc47a515dbd;
    memcpy(fr_attr.match_value.match_lyr2.src_mac, &smac_val, sizeof(fr_attr.match_value.match_lyr2.src_mac));
#endif
    fr_attr.match_value.match_lyr2.ethertype = 0x800;

    fr_attr.match_value.match_lyr3.dst_ip = 0x0ad1ff8a;
    fr_attr.match_value.match_lyr3.src_ip = 0x0ad1ff8b;
    fr_attr.match_value.match_lyr3.ip_protocol = 0x11;

    fr_attr.match_value.match_lyr4.type = match_params_lyr_4_type::UDP;
    fr_attr.match_value.match_lyr4.dst_port = 0xc350;
#if defined(__linux__)
    fr_attr.match_value.match_lyr4.src_port = 0xc351;
#endif

    fr_attr.actions.push_back(fa_fwd);
    std::weak_ptr<flow_rule_ex> fr_obj;
    ret = fg_obj.lock()->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj.lock().get(), nullptr);

    ret = fr_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    delete srq_obj;
    delete adapter_obj;
}

// TODO: cleanup this test for GA.
/**
 * @test dpcp_flow_rule_ex.ti_05_add_flow_rule_kernel_fwd_table_and_modify_header
 * @brief
 *    Check add_flow_rule kernel forward to flow table.
 * @details
 */
TEST_F(dpcp_flow_rule_ex, ti_05_add_flow_rule_kernel_fwd_table_and_modify_header)
{
    // Open adapter:
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    status ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    // Create receive queue:
    uint32_t tdn = adapter_obj->get_td();
    ASSERT_NE(0U, tdn);

    striding_rq* srq_obj = open_str_rq(adapter_obj, m_rqp);
    ASSERT_NE(nullptr, srq_obj);

    uint32_t rqn = 0;
    ret = srq_obj->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rqn);
    log_trace("tdn: 0x%x rqn: 0x%x\n", tdn, rqn);

    tir tir_obj(adapter_obj->get_ctx());

    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = TIR_ATTR_INLINE_RQN | TIR_ATTR_TRANSPORT_DOMAIN;
    tir_attr.inline_rqn = rqn;
    tir_attr.transport_domain = tdn;
    ret = tir_obj.create(tir_attr);
    ASSERT_EQ(DPCP_OK, ret);

    // Create steering components:
    std::shared_ptr<flow_table> root_table(adapter_obj->get_root_table(flow_table_type::FT_RX));
    ASSERT_NE(root_table.get(), nullptr);

    // Set flow group attributes.
    flow_group_attr fg_attr;
    fg_attr.end_flow_index = 1;
    fg_attr.start_flow_index = 0;
    fg_attr.match_criteria_enable = flow_group_match_criteria_enable::FG_MATCH_OUTER_HDR;

#if defined(__linux__)
    uint64_t dmac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.dst_mac,  &dmac, sizeof(fg_attr.match_criteria.match_lyr2.dst_mac));
    uint64_t smac = 0xFFFFFFFFFFFF;
    memcpy(fg_attr.match_criteria.match_lyr2.src_mac, &smac, sizeof(fg_attr.match_criteria.match_lyr2.src_mac));
#endif
    fg_attr.match_criteria.match_lyr2.ethertype = 0xFFFF;

    fg_attr.match_criteria.match_lyr3.dst_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.src_ip = 0xFFFFFFFF;
    fg_attr.match_criteria.match_lyr3.ip_protocol = 0xFF;
    fg_attr.match_criteria.match_lyr3.ip_version = 0xF;

    fg_attr.match_criteria.match_lyr4.type = match_params_lyr_4_type::UDP;
    fg_attr.match_criteria.match_lyr4.dst_port = 0xFFFF;
#if defined(__linux__)
    fg_attr.match_criteria.match_lyr4.src_port = 0xFFFF;
#endif

    // Create flow group SW object;
    std::weak_ptr<flow_group> fg_obj;
    ret = root_table->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj.lock().get(), nullptr);

    ret = fg_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Set flow table attributes.
    flow_table_attr ft_attr_fwd;
    ft_attr_fwd.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft_attr_fwd.flags = 0;
    ft_attr_fwd.level = 10;
    ft_attr_fwd.log_size = 10;
    ft_attr_fwd.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft_attr_fwd.type = flow_table_type::FT_RX;

    // Create flow table SW object;
    std::shared_ptr<flow_table> ft_fwd_obj;
    adapter_obj->create_flow_table(ft_attr_fwd, ft_fwd_obj);
    ASSERT_EQ(DPCP_OK, ret);

    // Create flow table HW object;
    ret = ft_fwd_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    flow_action_generator& action_gen = adapter_obj->get_flow_action_generator();
    std::vector<forwardable_obj*> dests;
    dests.push_back(ft_fwd_obj.get());
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_fwd(dests));

    flow_action_modify_type_attr fas1 {};
    fas1.set.data = 0x800;
    fas1.set.field = flow_action_modify_field::OUT_ETHERTYPE;
    fas1.set.length = 0x10;
    fas1.set.offset = 0x0;
    fas1.set.type = flow_action_modify_type::SET;

    flow_action_modify_type_attr fas2 {};
    fas2.set.data = tir_obj.get_tirn();
    fas2.set.field = flow_action_modify_field::METADATA_REG_C_0;
    fas2.set.length = 0x0; // Use all 32 bits.
    fas2.set.offset = 0x0;
    fas2.set.type = flow_action_modify_type::SET;

    flow_action_modify_attr fam;
    fam.table_type = flow_table_type::FT_RX;
    fam.actions.push_back(fas1);
    fam.actions.push_back(fas2);
    std::shared_ptr<flow_action> fa_modify(action_gen.create_modify(fam));

    flow_rule_attr_ex fr_attr;
    fr_attr.priority = 3;
    fr_attr.flow_index = 0;

#if defined(__linux__)
    uint64_t dmac_val = 0x0cc47a515dbc;
    memcpy(fr_attr.match_value.match_lyr2.dst_mac,  &dmac_val, sizeof(fr_attr.match_value.match_lyr2.dst_mac));
    uint64_t smac_val = 0x0cc47a515dbd;
    memcpy(fr_attr.match_value.match_lyr2.src_mac, &smac_val, sizeof(fr_attr.match_value.match_lyr2.src_mac));
#endif
    fr_attr.match_value.match_lyr2.ethertype = 0x800;

    fr_attr.match_value.match_lyr3.dst_ip = 0x0ad1ff8a;
    fr_attr.match_value.match_lyr3.src_ip = 0x0ad1ff8b;
    fr_attr.match_value.match_lyr3.ip_protocol = 0x11;
    fr_attr.match_value.match_lyr3.ip_version = 0x4;

    fr_attr.match_value.match_lyr4.type = match_params_lyr_4_type::UDP;
    fr_attr.match_value.match_lyr4.dst_port = 0xc350;
#if defined(__linux__)
    fr_attr.match_value.match_lyr4.src_port = 0xc351;
#endif

    fr_attr.actions.push_back(fa_fwd);
    fr_attr.actions.push_back(fa_modify);

    std::weak_ptr<flow_rule_ex> fr_obj;
    ret = fg_obj.lock()->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj.lock().get(), nullptr);

    ret = fr_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    delete srq_obj;
    delete adapter_obj;
}


/**
 * @test dpcp_flow_rule_ex.ti_06_add_flow_rule_modify_set_modify_copy_fwd_table_reparse
 * @brief
 *    Check creating flow table and flow group, then add flow rule with
 *    actions: modify-copy, modify-set, fwd to table and reparse.
 * @details
 */
TEST_F(dpcp_flow_rule_ex, ti_06_add_flow_rule_modify_set_modify_copy_fwd_table_reparse)
{
    status ret = DPCP_OK;

    // Get adapter:
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    /**< Create flow table 2 */

    // Set flow table 2 attributes:
    flow_table_attr ft2_attr;
    ft2_attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft2_attr.flags = 0;
    ft2_attr.level = 2;
    ft2_attr.log_size = 10;
    ft2_attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft2_attr.type = flow_table_type::FT_RX;

    // Create flow table 2 SW object:
    std::shared_ptr<flow_table> ft2_obj;
    ret = adapter_obj->create_flow_table(ft2_attr, ft2_obj);
    ASSERT_EQ(DPCP_OK, ret);

    // Create flow table 2 HW object:
    ret = ft2_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    /**< Create flow table 1 */

    // Check main capabilities:
    adapter_hca_capabilities caps;
    ret = adapter_obj->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    auto caps_supported = \
        caps.is_flow_table_caps_supported &&
        caps.flow_table_caps.receive.is_flow_action_modify_supported &&
        caps.flow_table_caps.receive.is_flow_action_reparse_supported &&
        caps.flow_table_caps.receive.modify_flow_action_caps.copy_fields_support.outer_udp_dport &&
        caps.flow_table_caps.receive.modify_flow_action_caps.copy_fields_support.metadata_reg_c_1 &&
        caps.flow_table_caps.receive.modify_flow_action_caps.set_fields_support.outer_udp_dport;

    if (!caps_supported) {
        log_error("Missing capabilities");
        return;
    }

    // Set flow table 1 attributes.
    flow_table_attr ft1_attr;
    ft1_attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    ft1_attr.flags = 0;
    ft1_attr.level = 1;
    ft1_attr.log_size = 1;
    ft1_attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    ft1_attr.type = flow_table_type::FT_RX;

    // Create flow table 1 SW object;
    std::shared_ptr<flow_table> ft1_obj;
    ret = adapter_obj->create_flow_table(ft1_attr, ft1_obj);
    ASSERT_EQ(DPCP_OK, ret);

    // Create flow table 1 HW object;
    ret = ft1_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    /**< Create flow group for table 1 */

    // Set flow group attributes:
    flow_group_attr fg_attr;
    fg_attr.start_flow_index = 0;
    fg_attr.end_flow_index = 1;
    fg_attr.match_criteria_enable = flow_group_match_criteria_enable::FG_MATCH_OUTER_HDR;

    // Create match criteria:
    memset(&fg_attr.match_criteria.match_lyr2.dst_mac, 0, sizeof(fg_attr.match_criteria.match_lyr2.dst_mac));
    memset(&fg_attr.match_criteria.match_lyr2.src_mac, 0, sizeof(fg_attr.match_criteria.match_lyr2.src_mac));
    fg_attr.match_criteria.match_lyr2.ethertype = 0xFFFF;

    fg_attr.match_criteria.match_lyr3.dst_ip = 0;
    fg_attr.match_criteria.match_lyr3.src_ip = 0;
    fg_attr.match_criteria.match_lyr3.ip_protocol = 0xFF;

    fg_attr.match_criteria.match_lyr4.type = match_params_lyr_4_type::NONE;
    fg_attr.match_criteria.match_lyr4.dst_port = 0;
    fg_attr.match_criteria.match_lyr4.src_port = 0;

    // Create flow group SW object:
    std::weak_ptr<flow_group> fg_obj;
    ret = ft1_obj->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj.lock().get(), nullptr);

    // Create flow group HW object:
    ret = fg_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    /**< Create flow rules for table 1 */

    // Create action generator:
    flow_action_generator& action_gen = adapter_obj->get_flow_action_generator();
    flow_action_modify_attr modify_hdr_attr;
    modify_hdr_attr.table_type = flow_table_type::FT_RX;

    // Create modify copy action - copy UDP destination port to metadata register C1:
    flow_action_modify_type_attr copy_attr{};
    copy_attr.copy.type = flow_action_modify_type::COPY;
    copy_attr.copy.src_field = flow_action_modify_field::OUT_UDP_DPORT;
    copy_attr.copy.src_offset = 0;
    copy_attr.copy.length = 16;
    copy_attr.copy.dst_field = flow_action_modify_field::METADATA_REG_C_1;
    copy_attr.copy.dst_offset = 0;

    modify_hdr_attr.actions.push_back(copy_attr);

    // Create modify set action - set UDP destination port to some special port:
    flow_action_modify_type_attr set_attr{};
    set_attr.set.type = flow_action_modify_type::SET;
    set_attr.set.data = 9;  // Discard UDP port
    set_attr.set.field = flow_action_modify_field::OUT_UDP_DPORT;
    set_attr.set.length = 16;
    set_attr.set.offset = 0;

    modify_hdr_attr.actions.push_back(set_attr);

    std::shared_ptr<flow_action> fa_modify(action_gen.create_modify(modify_hdr_attr));

    // Create forward action - forward to table 2:
    std::vector<forwardable_obj*> dests;
    dests.push_back(ft2_obj.get());
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_fwd(dests));

    // Crete reparse action:
    std::shared_ptr<flow_action> fa_reparse(action_gen.create_reparse());

    // Create flow rule - forward on all IPv4 packets to the next table:
    flow_rule_attr_ex fr_attr;
    fr_attr.priority = 3;
    fr_attr.flow_index = 0;

    memset(&fr_attr.match_value.match_lyr2.dst_mac, 0, sizeof(fr_attr.match_value.match_lyr2.dst_mac));
    memset(&fr_attr.match_value.match_lyr2.src_mac, 0, sizeof(fr_attr.match_value.match_lyr2.src_mac));
    fr_attr.match_value.match_lyr2.ethertype = 0x800;

    fr_attr.match_value.match_lyr3.dst_ip = 0;
    fr_attr.match_value.match_lyr3.src_ip = 0;
    fr_attr.match_value.match_lyr3.ip_protocol = 0x11;

    fr_attr.match_value.match_lyr4.type = match_params_lyr_4_type::NONE;
    fr_attr.match_value.match_lyr4.dst_port = 0;
    fr_attr.match_value.match_lyr4.src_port = 0;

    fr_attr.actions.push_back(fa_modify);
    fr_attr.actions.push_back(fa_fwd);
    fr_attr.actions.push_back(fa_reparse);
    std::weak_ptr<flow_rule_ex> fr_obj;
    ret = fg_obj.lock()->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj.lock().get(), nullptr);

    ret = fr_obj.lock()->create();
    ASSERT_EQ(DPCP_OK, ret);

    delete adapter_obj;
}
