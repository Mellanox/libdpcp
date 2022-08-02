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

class dpcp_pd : /*public obj,*/ public dpcp_base {
protected:
    //	SetUp() {}
};

/**
 * @test dpcp_pd.ti_01_Constructor
 * @brief
 *    Check pd_ibv constructor
 * @details
 */
TEST_F(dpcp_pd, ti_01_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    pd_ibv pd1(ad->get_ctx());
    uint32_t id = pd1.get_pd_id();
    ASSERT_EQ(0, id);

    delete ad;
}

/**
 * @test dpcp_pd.ti_02_Constructor_external
 * @brief
 *    Check pd_ibv constructor
 * @details
 */
TEST_F(dpcp_pd, ti_02_Constructor_external)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    ibv_context* ibv_ctx = (ibv_context*)ad->get_ibv_context();
    ASSERT_NE(nullptr, ibv_ctx);

    struct ibv_pd* pd = ibv_alloc_pd(ibv_ctx);
    ASSERT_NE(nullptr, pd);

    pd_ibv pd1(ad->get_ctx(), pd);
    uint32_t id = pd1.get_pd_id();
    ASSERT_EQ(0, id);

    ASSERT_EQ(pd1.get_ibv_pd(), pd);

    ibv_dealloc_pd(pd);
    delete ad;
}


/**
 * @test dpcp_pd.ti_03_create
 * @brief
 *    Check pd_ibv ::create method
 * @details
 *
 */
TEST_F(dpcp_pd, ti_03_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    pd_ibv pd1(ad->get_ctx());

    status ret = pd1.create();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t pd_id = pd1.get_pd_id();
    EXPECT_LE((uint32_t)0, pd_id);

    log_trace("pd_id: %u\n", pd_id);

    delete ad;
}

/**
 * @test dpcp_pd.ti_04_create_external
 * @brief
 *    Check pd_ibv ::create method
 * @details
 *
 */
TEST_F(dpcp_pd, ti_04_create_external)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    ibv_context* ibv_ctx = (ibv_context*)ad->get_ibv_context();
    ASSERT_NE(nullptr, ibv_ctx);

    struct ibv_pd* pd = ibv_alloc_pd(ibv_ctx);
    ASSERT_NE(nullptr, pd);

    pd_ibv pd1(ad->get_ctx(), pd);
    status ret = pd1.create();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t pd_id = pd1.get_pd_id();
    EXPECT_LE((uint32_t)0, pd_id);
    ASSERT_EQ(pd1.get_ibv_pd(), pd);

    ibv_dealloc_pd(pd);
    delete ad;
}

/**
 * @test dpcp_pd.ti_05_Constructor
 * @brief
 *    Check pd_devx constructor
 * @details
 */
TEST_F(dpcp_pd, ti_05_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    pd_devx pd1(ad->get_ctx());
    uint32_t id = pd1.get_pd_id();
    ASSERT_EQ(0, id);

    delete ad;
}

/**
 * @test dpcp_pd.ti_06_create
 * @brief
 *    Check pd_devx ::create method
 * @details
 *
 */
TEST_F(dpcp_pd, ti_06_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    pd_devx pd1(ad->get_ctx());

    status ret = pd1.create();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t pd_id = pd1.get_pd_id();
    EXPECT_LE((uint32_t)0, pd_id);

    log_trace("pd_id: %u\n", pd_id);

    delete ad;
}
