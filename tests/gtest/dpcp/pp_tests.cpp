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

class dpcp_pp : /*public obj,*/ public dpcp_base {
protected:
    //	SetUp() {}
};

qos_packet_pacing s_pp_attr = {1200000, 1, 1200};

/**
 * @test dpcp_pp.ti_01_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_pp, ti_01_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(ret, DPCP_OK);

    packet_pacing pp1(ad->get_ctx(), s_pp_attr);
    uint32_t id = pp1.get_index();
    ASSERT_EQ(0, id);

    delete ad;
}

/**
 * @test dpcp_pp.ti_02_create
 * @brief
 *    Check packet_pacing::create method
 * @details
 *
 */
TEST_F(dpcp_pp, ti_02_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(ret, DPCP_OK);

    packet_pacing pp1(ad->get_ctx(), s_pp_attr);
    uint32_t idx = pp1.get_index();
    ASSERT_EQ(0, idx);

    ret = pp1.create();
    ASSERT_EQ(DPCP_OK, ret);

    idx = pp1.get_index();
    EXPECT_LE((uint32_t)0, idx);

    log_trace("pp_idx: %u\n", idx);

    delete ad;
}
