/*
 * Copyright © 2020-2022 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Nvidia Corporation and its affiliates
 * (the "Company") and all right, title, and interest in and to the software
 * product, including all associated intellectual property rights, are and
 * shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 */

#include "common/def.h"
#include "common/log.h"
#include "common/sys.h"
#include "common/base.h"

#include "dpcp_base.h"

using namespace dpcp;

class dpcp_obj : /*public obj,*/ public dpcp_base {
protected:
    //	SetUp() {}
};

/**
 * @test dpcp_obj.ti1_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_obj, ti1_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    obj o(ad->get_ctx());
    uint32_t id;
    status ret = o.get_id(id);
    ASSERT_EQ(ret, DPCP_ERR_INVALID_ID);

    uintptr_t handle;
    ret = o.get_handle(handle);
    delete ad;
}

/**
 * @test dpcp_obj.ti2_create_bad_par
 * @brief
 *    Check obj.create with wrong parameters
 * @details
 *
 */
TEST_F(dpcp_obj, ti2_create_bad_par)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    obj o(ad->get_ctx());

    uint32_t in[DEVX_ST_SZ_DW(alloc_pd_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(alloc_pd_out)] = {};
    size_t outlen = sizeof(out);

    DEVX_SET(alloc_pd_in, in, opcode, MLX5_CMD_OP_ALLOC_PD);
    status ret = o.create(nullptr, sizeof(in), out, outlen);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    ret = o.create(in, 0, out, outlen);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    ret = o.create(in, sizeof(in), nullptr, outlen);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    outlen = 10;
    ret = o.create(in, sizeof(in), out, outlen);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    delete ad;
}

/**
 * @test dpcp_obj.ti2_create_good_par
 * @brief
 *    Check obj.create with good parameters
 * @details
 *
 */
TEST_F(dpcp_obj, ti3_create_good_par)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    obj o(ad->get_ctx());

    uint32_t in[DEVX_ST_SZ_DW(alloc_pd_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(alloc_pd_out)] = {};
    size_t outlen = sizeof(out);

    DEVX_SET(alloc_pd_in, in, opcode, MLX5_CMD_OP_ALLOC_PD);
    status ret = o.create(in, sizeof(in), out, outlen);
    log_trace("errno: %d outlen: %zd out: %8x %8x %8x %8x\n", errno, outlen, out[0], out[1], out[2],
              out[3]);
    ASSERT_EQ(DPCP_OK, ret);

    int pd = DEVX_GET(alloc_pd_out, out, pd);
    EXPECT_LE(0, pd);

    uint32_t id;
    ret = o.get_id(id);
    log_trace("pd: %d id: 0x%x\n", pd, id);
    ASSERT_EQ(DPCP_OK, ret);

    uintptr_t handle;
    ret = o.get_handle(handle);
    ASSERT_EQ(DPCP_OK, ret);

    delete ad;
}

/**
 * @test dpcp_obj.ti4_modify_bad_par
 * @brief
 *    Check obj.modify with wrong parameters
 * @details
 *
 */
TEST_F(dpcp_obj, ti4_modify_bad_par)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    obj o(ad->get_ctx());

    uint32_t in[DEVX_ST_SZ_DW(alloc_pd_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(alloc_pd_out)] = {};
    size_t outlen = sizeof(out);

    DEVX_SET(alloc_pd_in, in, opcode, MLX5_CMD_OP_ALLOC_PD);
    status ret = o.modify(nullptr, sizeof(in), out, outlen);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    ret = o.modify(in, 0, out, outlen);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    ret = o.modify(in, sizeof(in), nullptr, outlen);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    outlen = 10;
    ret = o.modify(in, sizeof(in), out, outlen);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    delete ad;
}

/**
 * @test dpcp_obj.ti6_query_bad_par
 * @brief
 *    Check obj.query with wrong parameters
 * @details
 *
 */
TEST_F(dpcp_obj, ti6_query_bad_par)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    obj o(ad->get_ctx());

    uint32_t in[DEVX_ST_SZ_DW(alloc_pd_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(alloc_pd_out)] = {};
    size_t outlen = sizeof(out);

    DEVX_SET(alloc_pd_in, in, opcode, MLX5_CMD_OP_ALLOC_PD);
    status ret = o.query(nullptr, sizeof(in), out, outlen);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    ret = o.query(in, 0, out, outlen);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    ret = o.query(in, sizeof(in), nullptr, outlen);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    outlen = 10;
    ret = o.query(in, sizeof(in), out, outlen);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    delete ad;
}
