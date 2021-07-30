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

static rq_attr s_rq_attr;
static adapter* s_ad;
static striding_rq* s_srq;
static uint32_t s_rq_num;
static uint32_t s_wqe_sz;

class dpcp_rq : /*public obj,*/ public dpcp_base {
protected:
    void SetUp() override
    {
        if (errno) {
            log_trace("dpcp_rq::SetUp errno= %d\n", errno);
            errno = EOK;
        }
    }
};

/**
 * @test dpcp_rq.ti_01_create
 * @brief
 *    Check pd::create method
 * @details
 *
 */
TEST_F(dpcp_rq, ti_01_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    s_ad = ad;

    cq_data cqd = {};
    ret = (status)create_cq(ad, &cqd);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0, cqd.cqn);

    s_rq_attr = {2048, 16384, 0, cqd.cqn};

    s_rq_num = 4;
    s_wqe_sz = s_rq_attr.buf_stride_num * s_rq_attr.buf_stride_sz / 16; // in DS (16B)

    striding_rq* srq = nullptr;
    ret = ad->create_striding_rq(s_rq_attr, s_rq_num, s_wqe_sz, srq);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, srq);
    s_srq = srq;
    void* buf = nullptr;
    ret = s_srq->get_wq_buf(buf);
}
/**
 * @test dpcp_rq.ti_02_get_wq_buf
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_rq, ti_02_get_wq_buf)
{
    void* buf = nullptr;
    status ret = s_srq->get_wq_buf(buf);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, buf);
}
/**
 * @test dpcp_rq.ti_03_get_dbrec
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_rq, ti_03_get_dbrec)
{
    uint32_t* dbrec = nullptr;
    status ret = s_srq->get_dbrec(dbrec);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, dbrec);
}
/**
 * @test dpcp_rq.ti_04_get_uar_page
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_rq, ti_04_get_uar_page)
{
    volatile void* uar_page = nullptr;
    status ret = s_srq->get_uar_page(uar_page);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, (void*)uar_page);
}
/**
 * @test dpcp_rq.ti_05_get_wqe_num
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_rq, ti_05_get_wqe_num)
{
    uint32_t wqe_num;
    status ret = s_srq->get_wqe_num(wqe_num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(wqe_num, s_rq_num);
}
/**
 * @test dpcp_rq.ti_06_get_wq_stride_sz
 * @brief
 *    Check get_wq_stride_sz
 * @details
 */
TEST_F(dpcp_rq, ti_06_get_wq_stride_sz)
{
    uint32_t wq_stride_sz;
    status ret = s_srq->get_wq_stride_sz(wq_stride_sz);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(wq_stride_sz, s_wqe_sz * 16);
}
/**
 * @test dpcp_rq.ti_07_get_wq_buf_sz
 * @brief
 *    Check get_wq_buf_sz
 * @details
 */
TEST_F(dpcp_rq, ti_07_get_wq_buf_sz)
{
    size_t wq_buf_sz = s_srq->get_wq_buf_sz();
    log_trace("wq_buf_sz: %zd\n", wq_buf_sz);
}
/**
 * @test dpcp_rq.ti_08_modify_state
 * @brief
 *    Check striding_rq::modify_state method
 * @details
 *
 */
TEST_F(dpcp_rq, ti_08_modify_state)
{
    status ret = s_srq->modify_state(RQ_RDY);
    ASSERT_EQ(DPCP_OK, ret);

    delete s_srq;
    delete s_ad;
}
