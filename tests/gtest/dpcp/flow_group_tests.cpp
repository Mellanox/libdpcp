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

    uint32_t fg_id = 0;
    ret = fg_obj->get_group_id(fg_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_id, 0);

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

    // Create flow table SW object.
    flow_table ft_obj(adapter_obj->get_ctx(), ft_attr);

    // Create flow table HW object.
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

    // Create flow group SW object.
    flow_group* fg_obj = nullptr;
    ret = ft_obj.add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj, nullptr);

    // Create flow group HW object.
    ret = fg_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Remove flow group.
    ret = ft_obj.remove_flow_group(fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(fg_obj, nullptr);

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

    // Create flow table SW object.
    flow_table ft_obj(adapter_obj->get_ctx(), ft_attr);

    // Create flow table HW object.
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

    // Create flow group SW object.
    flow_group* fg_obj = nullptr;
    ret = ft_obj.add_flow_group(fg_attr, fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(fg_obj, nullptr);

    // Create flow group HW object.
    ret = fg_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Remove flow group.
    flow_group* fg_obj_tmp = fg_obj;
    ret = ft_obj.remove_flow_group(fg_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(fg_obj, nullptr);

    // Remove flow group second time.
    ret = ft_obj.remove_flow_group(fg_obj_tmp);
    ASSERT_NE(DPCP_OK, ret);

    delete adapter_obj;
}

