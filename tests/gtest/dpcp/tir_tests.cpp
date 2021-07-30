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

class dpcp_tir : public dpcp_base {
protected:
    //	SetUp() {}
};

/**
 * @test dpcp_td.ti_01_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_tir, ti_01_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    tir tir1(ad->get_ctx());
    uint32_t id = 0;
    status ret = tir1.get_id(id);
    log_trace("ret: %d id: 0x%x\n", ret, id);
    ASSERT_EQ(DPCP_ERR_INVALID_ID, ret);
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
TEST_F(dpcp_tir, ti_02_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = ad->get_td();
    ASSERT_NE(0, tdn);

    striding_rq* srq = open_str_rq(ad, m_rqp);
    ASSERT_NE(nullptr, srq);

    uint32_t rqn = 0;
    ret = srq->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0, rqn);
    log_trace("tdn: 0x%x rqn: 0x%x\n", tdn, rqn);

    tir tr1(ad->get_ctx());

    ret = tr1.create(tdn, rqn);
    ASSERT_EQ(DPCP_OK, ret);

    delete srq;
    delete ad;
}
