/*
Copyright (C) Mellanox Technologies, Ltd. 2020. ALL RIGHTS RESERVED.

This software product is a proprietary product of Mellanox Technologies, Ltd.
(the "Company") and all right, title, and interest in and to the software
product, including all associated intellectual property rights, are and shall
remain exclusively with the Company. All rights in or to the software product
are licensed, not sold. All rights not licensed are reserved.

This software product is governed by the End User License Agreement provided
with the software product.
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
    flow_table ft_obj(adapter_obj->get_ctx(), ft_attr);

    // Create flow table HW object;
    ret = ft_obj.create();
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
    flow_group* fg_obj = nullptr;
    ret = ft_obj.add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj, nullptr);

    // Create flow group HW object;
    ret = fg_obj->create();
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
    flow_table ft_fwd_obj(adapter_obj->get_ctx(), ft_attr_fwd);

    // Create flow table HW object;
    ret = ft_fwd_obj.create();
    ASSERT_EQ(DPCP_OK, ret);

    flow_action_generator action_gen(adapter_obj->get_ctx());
    std::vector<obj*> dests;
    dests.push_back(&ft_fwd_obj);
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_flow_action_fwd(dests));

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
    flow_rule_ex* fr_obj = nullptr;
    ret = fg_obj->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj, nullptr);

    ret = fr_obj->create();
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
    flow_table ft_obj(adapter_obj->get_ctx(), ft_attr);

    // Create flow table HW object;
    ret = ft_obj.create();
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
    flow_group* fg_obj = nullptr;
    ret = ft_obj.add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj, nullptr);

    // Create flow group HW object;
    ret = fg_obj->create();
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
    flow_table ft_fwd_obj(adapter_obj->get_ctx(), ft_attr_fwd);

    // Create flow table HW object;
    ret = ft_fwd_obj.create();
    ASSERT_EQ(DPCP_OK, ret);

    flow_action_generator action_gen(adapter_obj->get_ctx());
    std::vector<obj*> dests;
    dests.push_back(&ft_fwd_obj);
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_flow_action_fwd(dests));

    flow_action_reformat_attr insert_hdr_attr {};
    insert_hdr_attr.insert.type = flow_action_reformat_type::INSERT_HDR;
    insert_hdr_attr.insert.start_hdr = dpcp::flow_action_reformat_anchor::MAC_START;
    insert_hdr_attr.insert.offset = 22;
    insert_hdr_attr.insert.data_len = std::bitset<10>(28);
    insert_hdr_attr.insert.data = new uint8_t[28];

    std::shared_ptr<flow_action> fa_insert(action_gen.create_flow_action_reformat(insert_hdr_attr));
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
    flow_rule_ex* fr_obj = nullptr;
    ret = fg_obj->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj, nullptr);

    ret = fr_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    delete adapter_obj;
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
    flow_table ft_obj(adapter_obj->get_ctx(), ft_attr);

    // Create flow table HW object;
    ret = ft_obj.create();
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
    flow_group* fg_obj = nullptr;
    ret = ft_obj.add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj, nullptr);

    // Create flow group HW object;
    ret = fg_obj->create();
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
    flow_table ft_fwd_obj(adapter_obj->get_ctx(), ft_attr_fwd);

    // Create flow table HW object;
    ret = ft_fwd_obj.create();
    ASSERT_EQ(DPCP_OK, ret);

    flow_action_generator action_gen(adapter_obj->get_ctx());
    std::vector<obj*> dests;
    dests.push_back(&ft_fwd_obj);
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_flow_action_fwd(dests));

    flow_action_modify_type_attr set_attr {};
    set_attr.set.type = flow_action_modify_type::SET;
    set_attr.set.data = 0x800;
    set_attr.set.field = flow_action_modify_field::OUT_ETHERTYPE;
    set_attr.set.length = 0x10;
    set_attr.set.offset = 0;

    flow_action_modify_attr modify_hdr_attr;
    modify_hdr_attr.table_type = flow_table_type::FT_RX;
    modify_hdr_attr.actions.push_back(set_attr);

    std::shared_ptr<flow_action> fa_modify(action_gen.create_flow_action_modify(modify_hdr_attr));

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
    flow_rule_ex* fr_obj = nullptr;
    ret = fg_obj->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj, nullptr);

    ret = fr_obj->create();
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
    flow_group* fg_obj = nullptr;
    ret = root_table->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj, nullptr);

    ret = fg_obj->create();
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
    flow_table ft_fwd_obj(adapter_obj->get_ctx(), ft_attr_fwd);

    // Create flow table HW object;
    ret = ft_fwd_obj.create();
    ASSERT_EQ(DPCP_OK, ret);

    flow_action_generator action_gen(adapter_obj->get_ctx());
    std::vector<obj*> dests;
    dests.push_back(&ft_fwd_obj);
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_flow_action_fwd(dests));

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
    flow_rule_ex* fr_obj = nullptr;
    ret = fg_obj->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj, nullptr);

    ret = fr_obj->create();
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
    flow_group* fg_obj = nullptr;
    ret = root_table->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj, nullptr);

    ret = fg_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Create tir object to forward to
    ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = adapter_obj->get_td();
    ASSERT_NE(0, tdn);

    striding_rq* srq_obj = open_str_rq(adapter_obj, m_rqp);
    ASSERT_NE(nullptr, srq_obj);

    uint32_t rqn = 0;
    ret = srq_obj->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0, rqn);
    log_trace("tdn: 0x%x rqn: 0x%x\n", tdn, rqn);

    tir tir_obj(adapter_obj->get_ctx());

    ret = tir_obj.create(tdn, rqn);
    ASSERT_EQ(DPCP_OK, ret);



    flow_action_generator action_gen(adapter_obj->get_ctx());
    std::vector<obj*> dests;
    dests.push_back(&tir_obj);
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_flow_action_fwd(dests));

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
    flow_rule_ex* fr_obj = nullptr;
    ret = fg_obj->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj, nullptr);

    ret = fr_obj->create();
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
    flow_group* fg_obj = nullptr;
    ret = root_table->add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj, nullptr);

    ret = fg_obj->create();
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
    flow_table ft_fwd_obj(adapter_obj->get_ctx(), ft_attr_fwd);

    // Create flow table HW object;
    ret = ft_fwd_obj.create();
    ASSERT_EQ(DPCP_OK, ret);

    flow_action_generator action_gen(adapter_obj->get_ctx());
    std::vector<obj*> dests;
    dests.push_back(&ft_fwd_obj);
    std::shared_ptr<flow_action> fa_fwd(action_gen.create_flow_action_fwd(dests));

    flow_action_modify_type_attr fas {};
    fas.set.data = 0x800;
    fas.set.field = flow_action_modify_field::OUT_ETHERTYPE;
    fas.set.length = std::bitset<5>(0x10);
    fas.set.offset = std::bitset<5>(0x0);
    fas.set.type = flow_action_modify_type::SET;

    flow_action_modify_attr fam;
    fam.table_type = flow_table_type::FT_RX;
    fam.actions.push_back(fas);
    std::shared_ptr<flow_action> fa_modify(action_gen.create_flow_action_modify(fam));

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

    flow_rule_ex* fr_obj = nullptr;
    ret = fg_obj->add_flow_rule(fr_attr, fr_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fr_obj, nullptr);

    ret = fr_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    delete adapter_obj;
}

