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

static sq_attr s_sqattr;
static adapter* s_ad;
static pp_sq* s_ppsq;
static uint32_t s_sq_num;
static uint32_t s_wqe_sz;
static tis* s_tis;
static uint32_t s_tis_n;
static cq_data s_cqd = {};

class dpcp_sq : /*public obj,*/ public dpcp_base {
protected:
    void SetUp() override
    {
        if (errno) {
            log_trace("dpcp_sq::SetUp errno= %d\n", errno);
            errno = EOK;
        }
    }
};

/**
 * @test dpcp_sq.ti_01_create_with_pp
 * @brief
 *    Check pp_sq::create method
 * @details
 *
 */
TEST_F(dpcp_sq, ti_01_create_with_pp)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    s_ad = ad;

    ret = (status)create_cq(ad, &s_cqd);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0, s_cqd.cqn);

    ret = ad->create_tis(tis_flags::TIS_NONE, s_tis);
    ASSERT_EQ(DPCP_OK, ret);
    s_tis_n = 0;
    ret = s_tis->get_tisn(s_tis_n);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0, s_tis_n);

    qos_attributes qos_attr;
    qos_attr.qos_type = QOS_TYPE::QOS_PACKET_PACING;
    qos_attr.qos_attr.packet_pacing_attr.burst_sz = 1;
    qos_attr.qos_attr.packet_pacing_attr.packet_sz = 1200;
    qos_attr.qos_attr.packet_pacing_attr.sustained_rate = 1200000;
    s_sqattr.qos_attrs_sz = 1;
    s_sqattr.qos_attrs = &qos_attr;
    s_sqattr.wqe_sz = 64;
    s_sqattr.wqe_num = 32768;
    s_sqattr.user_index = 0;
    s_sqattr.cqn = s_cqd.cqn;
    s_sqattr.tis_num = s_tis_n;

    pp_sq* ppsq = nullptr;
    ret = ad->create_pp_sq(s_sqattr, ppsq);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, ppsq);
    s_ppsq = ppsq;
    void* buf = nullptr;
    ret = ppsq->get_wq_buf(buf);
}
/**
 * @test dpcp_sq.ti_02_get_wq_buf
 * @brief
 *    Test for get_wq_buf
 * @details
 */
TEST_F(dpcp_sq, ti_02_get_wq_buf)
{
    void* buf = nullptr;
    status ret = s_ppsq->get_wq_buf(buf);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, buf);
}
/**
 * @test dpcp_sq.ti_03_get_dbrec
 * @brief
 *    Test for get_dbrec
 * @details
 */
TEST_F(dpcp_sq, ti_03_get_dbrec)
{
    uint32_t* dbrec = nullptr;
    status ret = s_ppsq->get_dbrec(dbrec);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, dbrec);
}
/**
 * @test dpcp_rq.ti_04_get_uar_page
 * @brief
 *    Test of get_uar_page
 * @details
 */
TEST_F(dpcp_sq, ti_04_get_uar_page)
{
    volatile void* uar_page = nullptr;
    status ret = s_ppsq->get_uar_page(uar_page);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, (void*)uar_page);
}
/**
 * @test dpcp_sq.ti_05_get_wqe_num
 * @brief
 *    Check get_wqe_num()
 * @details
 */
TEST_F(dpcp_sq, ti_05_get_wqe_num)
{
    uint32_t wqe_num;
    status ret = s_ppsq->get_wqe_num(wqe_num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(wqe_num, s_sqattr.wqe_num);
}
/**
 * @test dpcp_sq.ti_06_get_wq_stride_sz
 * @brief
 *    Check get_wq_stride_sz
 * @details
 */
TEST_F(dpcp_sq, ti_06_get_wq_stride_sz)
{
    uint32_t wq_stride_sz;
    status ret = s_ppsq->get_wqe_sz(wq_stride_sz);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(wq_stride_sz, s_sqattr.wqe_sz);
}
/**
 * @test dpcp_sq.ti_07_get_wq_buf_sz
 * @brief
 *    Check get_wq_buf_sz
 * @details
 */
TEST_F(dpcp_sq, ti_07_get_wq_buf_sz)
{
    size_t wq_buf_sz = s_ppsq->get_wq_buf_sz();
    log_trace("wq_buf_sz: %zd\n", wq_buf_sz);
}
/**
 * @test dpcp_sq.ti_08_modify_state
 * @brief
 *    Check pp_sq::modify_state method
 * @details
 *
 */
TEST_F(dpcp_sq, ti_08_modify_state)
{
    status ret = s_ppsq->modify_state(SQ_RDY);
    ASSERT_EQ(DPCP_OK, ret);
}
/**
 * @test dpcp_sq.ti_09_modify_pp
 * @brief
 *    Check pp_sq::modify method
 * @details
 *
 */
TEST_F(dpcp_sq, ti_09_modify_pp)
{
    qos_attributes qos_attr;
    qos_attr.qos_type = QOS_TYPE::QOS_PACKET_PACING;
    qos_attr.qos_attr.packet_pacing_attr.burst_sz = 1;
    qos_attr.qos_attr.packet_pacing_attr.packet_sz = 1400;
    qos_attr.qos_attr.packet_pacing_attr.sustained_rate = 2400000;
    s_sqattr.qos_attrs_sz = 1;
    s_sqattr.qos_attrs = &qos_attr;

    status ret = s_ppsq->modify(s_sqattr);
    ASSERT_EQ(DPCP_OK, ret);

    delete s_ppsq;
}
/**
 * @test dpcp_sq.ti_01_create_without_pp
 * @brief
 *    Check pp_sq::create method
 * @details
 *
 */
TEST_F(dpcp_sq, ti_10_create_without_pp)
{
    qos_attributes qos_attr;
    qos_attr.qos_type = QOS_TYPE::QOS_PACKET_PACING;
    qos_attr.qos_attr.packet_pacing_attr.burst_sz = 0;
    qos_attr.qos_attr.packet_pacing_attr.packet_sz = 0;
    qos_attr.qos_attr.packet_pacing_attr.sustained_rate = 0;
    s_sqattr.qos_attrs_sz = 1;
    s_sqattr.qos_attrs = &qos_attr;
    s_sqattr.wqe_sz = 64;
    s_sqattr.wqe_num = 32768;
    s_sqattr.user_index = 0;
    s_sqattr.cqn = s_cqd.cqn;
    s_sqattr.tis_num = s_tis_n;

    pp_sq* ppsq = nullptr;
    status ret = s_ad->create_pp_sq(s_sqattr, ppsq);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, ppsq);

    delete ppsq;
}

/**
 * @test dpcp_sq.ti_11_create_modify_pp
 * @brief
 *    Check pp_sq::create method
 * @details
 *
 */
TEST_F(dpcp_sq, ti_11_create_modify_pp)
{
    qos_attributes qos_attr;
    qos_attr.qos_type = QOS_TYPE::QOS_PACKET_PACING;
    qos_attr.qos_attr.packet_pacing_attr.burst_sz = 1;
    qos_attr.qos_attr.packet_pacing_attr.packet_sz = 1000;
    qos_attr.qos_attr.packet_pacing_attr.sustained_rate = 1000000;
    s_sqattr.qos_attrs_sz = 1;
    s_sqattr.qos_attrs = &qos_attr;
    s_sqattr.wqe_sz = 64;
    s_sqattr.wqe_num = 32768;
    s_sqattr.user_index = 0;
    s_sqattr.cqn = s_cqd.cqn;
    s_sqattr.tis_num = s_tis_n;

    pp_sq* ppsq = nullptr;
    status ret = s_ad->create_pp_sq(s_sqattr, ppsq);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, ppsq);

    ret = ppsq->modify_state(SQ_RDY);
    ASSERT_EQ(DPCP_OK, ret);

    qos_attributes qos_attr2;
    qos_attr2.qos_type = QOS_TYPE::QOS_PACKET_PACING;
    qos_attr2.qos_attr.packet_pacing_attr.burst_sz = 0;
    qos_attr2.qos_attr.packet_pacing_attr.packet_sz = 0;
    qos_attr2.qos_attr.packet_pacing_attr.sustained_rate = 0;
    s_sqattr.qos_attrs_sz = 1;
    s_sqattr.qos_attrs = &qos_attr2;

    ret = ppsq->modify(s_sqattr);
    ASSERT_EQ(DPCP_OK, ret);

    qos_attributes qos_attr3;
    qos_attr3.qos_type = QOS_TYPE::QOS_PACKET_PACING;
    qos_attr3.qos_attr.packet_pacing_attr.burst_sz = 1;
    qos_attr3.qos_attr.packet_pacing_attr.packet_sz = 1400;
    qos_attr3.qos_attr.packet_pacing_attr.sustained_rate = 2400000;
    s_sqattr.qos_attrs_sz = 1;
    s_sqattr.qos_attrs = &qos_attr3;

    ret = ppsq->modify(s_sqattr);
    delete ppsq;
    delete s_tis;
    delete s_ad;
}
