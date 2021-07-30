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

class dpcp_td : public dpcp_base {
protected:
    //	SetUp() {}
};

/**
 * @test dpcp_td.ti_01_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_td, ti_01_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    td td1(ad->get_ctx());
    uint32_t id = td1.get_td_id();
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
TEST_F(dpcp_td, ti_02_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    td td1(ad->get_ctx());

    status ret = td1.create();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t td_id = td1.get_td_id();
    EXPECT_LE((uint32_t)0, td_id);

    log_trace("td_id: %u\n", td_id);

    delete ad;
}
