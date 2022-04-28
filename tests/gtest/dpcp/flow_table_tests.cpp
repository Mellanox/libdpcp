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

class dpcp_flow_table : public dpcp_base {};

/**
 * @test dpcp_flow_table.ti_01_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_flow_table, ti_01_Constructor)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr attr;
    attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    attr.flags = 0;
    attr.level = 1;
    attr.log_size = 10;
    attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    attr.type = flow_table_type::FT_RX;

    // Create flow table SW object;
    flow_table ft_obj(adapter_obj->get_ctx(), attr);
    uintptr_t handle = 0;
    ret = ft_obj.get_handle(handle);
    ASSERT_EQ(DPCP_ERR_CREATE, ret);
    ASSERT_EQ(0, handle);

    uint32_t id = 0;
    ret = ft_obj.get_id(id);
    ASSERT_EQ(DPCP_ERR_INVALID_ID, ret);
    ASSERT_EQ(0, id);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_table.ti_02_create
 * @brief
 *    Check flow_table::create method
 * @details
 */
TEST_F(dpcp_flow_table, ti_02_create)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr attr;
    attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    attr.level = 1;
    attr.log_size = 10;
    attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    attr.type = flow_table_type::FT_RX;

    // Create flow table SW object.
    flow_table ft_obj(adapter_obj->get_ctx(), attr);

    // Create flow_table HW object.
    ret = ft_obj.create();
    ASSERT_EQ(DPCP_OK, ret);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_table.ti_04_create_twice
 * @brief
 *    flow table can not be created more than once
 * @details
 */
TEST_F(dpcp_flow_table, ti_04_create_twice)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr attr;
    attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    attr.level = 1;
    attr.log_size = 10;
    attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    attr.type = flow_table_type::FT_RX;

    // Create flow table SW object.
    flow_table ft_obj(adapter_obj->get_ctx(), attr);

    // Create flow_table HW object.
    ret = ft_obj.create();
    ASSERT_EQ(DPCP_OK, ret);

    // Create flow_table HW object second time.
    ret = ft_obj.create();
    ASSERT_EQ(DPCP_ERR_CREATE, ret);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_table.ti_05_create_invalid_01
 * @brief
 *    Check flow table creation with invalid parameters
 * @details
 */
TEST_F(dpcp_flow_table, ti_05_create_invalid_01)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr attr;
    attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    attr.level = 0;
    attr.log_size = 10;
    attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    attr.type = flow_table_type::FT_RX;

    // Create flow table SW object.
    flow_table ft_obj(adapter_obj->get_ctx(), attr);

    // Create flow_table HW object.
    ret = ft_obj.create();
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_table.ti_06_create_invalid_02
 * @brief
 *    Check flow table creation with invalid parameters
 * @details
 */
TEST_F(dpcp_flow_table, ti_06_create_invalid_02)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr attr;
    attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_FWD;
    attr.level = 1;
    attr.log_size = 10;
    attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    attr.type = flow_table_type::FT_RX;

    // Create flow table SW object.
    flow_table ft_obj(adapter_obj->get_ctx(), attr);

    // Create flow_table HW object.
    ret = ft_obj.create();
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_table.ti_07_query
 * @brief
 *    Check dpcp_flow_table::query method
 * @details
 *
 */
TEST_F(dpcp_flow_table, ti_07_query)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr attr_in;
    attr_in.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    attr_in.level = 1;
    attr_in.log_size = 10;
    attr_in.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    attr_in.type = flow_table_type::FT_RX;

    // Create flow table SW object.
    flow_table ft_obj(adapter_obj->get_ctx(), attr_in);

    // Create flow_table HW object.
    ret = ft_obj.create();
    ASSERT_EQ(DPCP_OK, ret);

    flow_table_attr attr_out;
    ret = ft_obj.query(attr_out);
    ASSERT_EQ(DPCP_OK, ret);

    ASSERT_EQ(attr_in.def_miss_action, attr_out.def_miss_action);
    ASSERT_EQ(attr_in.flags, attr_out.flags);
    ASSERT_EQ(attr_in.level, attr_out.level);
    ASSERT_EQ(attr_in.log_size, attr_out.log_size);
    ASSERT_EQ(attr_in.op_mod, attr_out.op_mod);
    ASSERT_EQ(attr_in.type, attr_out.type);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_table.ti_08_miss_action_fwd01
 * @brief
 *    Check flow table miss action forward.
 * @details
 */
TEST_F(dpcp_flow_table, ti_08_miss_action_fwd01)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr attr_miss;
    attr_miss.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    attr_miss.level = 2;
    attr_miss.log_size = 10;
    attr_miss.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    attr_miss.type = flow_table_type::FT_RX;

    // Create flow table SW object of miss table.
    std::shared_ptr<flow_table> ft_miss_obj(new (std::nothrow) flow_table(adapter_obj->get_ctx(), attr_miss));
    ASSERT_NE(nullptr, ft_miss_obj);

    // Create flow_table HW object of miss table.
    ret = ft_miss_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Set flow table attributes.
    flow_table_attr attr;
    attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_FWD;
    attr.level = 1;
    attr.log_size = 10;
    attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    attr.type = flow_table_type::FT_RX;
    attr.table_miss = ft_miss_obj;

    // Create flow table SW object.
    flow_table ft_obj(adapter_obj->get_ctx(), attr);
    ret = ft_obj.create();
    ASSERT_EQ(DPCP_OK, ret);

    delete adapter_obj;
}

/**
 * @test dpcp_flow_table.ti_08_miss_action_fwd02
 * @brief
 *    Check flow table miss action forward fail.
 * @details
 */
TEST_F(dpcp_flow_table, ti_08_miss_action_fwd02)
{
    status ret = DPCP_OK;

    // Get adapter.
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Set flow table attributes.
    flow_table_attr attr_miss;
    attr_miss.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    attr_miss.level = 1;
    attr_miss.log_size = 10;
    attr_miss.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    attr_miss.type = flow_table_type::FT_RX;

    // Create flow table SW object of miss table.
    std::shared_ptr<flow_table> ft_miss_obj(new (std::nothrow) flow_table(adapter_obj->get_ctx(), attr_miss));
    ASSERT_NE(nullptr, ft_miss_obj);

    // Create flow_table HW object of miss table.
    ret = ft_miss_obj->create();
    ASSERT_EQ(DPCP_OK, ret);

    // Set flow table attributes.
    flow_table_attr attr;
    attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_FWD;
    attr.level = 2;
    attr.log_size = 10;
    attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    attr.type = flow_table_type::FT_RX;
    attr.table_miss = ft_miss_obj;

    // Create flow table SW object, should fail because miss table level is less then this table level.
    flow_table ft_obj(adapter_obj->get_ctx(), attr);
    ret = ft_obj.create();
    ASSERT_NE(DPCP_OK, ret);

    delete adapter_obj;
}

