/*
 * Copyright (c) 2020-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
