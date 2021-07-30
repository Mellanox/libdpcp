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
 *    Check constructor
 * @details
 */
TEST_F(dpcp_pd, ti_01_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    pd pd1(ad->get_ctx());
    uint32_t id = pd1.get_pd_id();
    ASSERT_EQ(0, id);

    delete ad;
}

/**
 * @test dpcp_pd.ti_02_create
 * @brief
 *    Check pd::create method
 * @details
 *
 */
TEST_F(dpcp_pd, ti_02_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    pd pd1(ad->get_ctx());

    status ret = pd1.create();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t pd_id = pd1.get_pd_id();
    EXPECT_LE((uint32_t)0, pd_id);

    log_trace("pd_id: %u\n", pd_id);

    delete ad;
}
