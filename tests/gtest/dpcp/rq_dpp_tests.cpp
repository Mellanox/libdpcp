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

static rq_attr s_dpp_rq_attr;
static adapter* s_ad;
static dpp_rq* s_dpp_rq;
static uint32_t s_dpp_rq_num;
static direct_mkey* s_mk;
static uint32_t s_mkey;
static dpcp_dpp_protocol s_dpp_protocol = dpcp::DPCP_DPP_NOT_INITIALIZED;

class dpcp_dpp_rq : /*public obj,*/ public dpcp_base {
protected:
    void SetUp() override
    {
        if (errno) {
            log_trace("dpcp_dpp_rq::SetUp errno= %d\n", errno);
            errno = EOK;
        }
    }
};

/**
 * @test dpcp_dpp_rq.ti_01_create
 * @brief
 *    Check pd::create method
 * @details
 *
 */
TEST_F(dpcp_dpp_rq, ti_01_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(ad, nullptr);

    status ret = ad->open();
    ASSERT_EQ(ret, DPCP_OK);

    s_ad = ad;

    cq_data cqd = {};
    ret = (status)create_cq(s_ad, &cqd);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(cqd.cqn, 0);

    s_dpp_rq_attr = {1500, 10000, 0, cqd.cqn};

    // create buffer and obtain mkey for it
    int32_t buf_length = s_dpp_rq_attr.buf_stride_sz * s_dpp_rq_attr.buf_stride_num;
    void* buf = new (std::nothrow) uint8_t[buf_length];
    ASSERT_NE(buf, nullptr);

    direct_mkey* mk = nullptr;
    ret = ad->create_direct_mkey(buf, buf_length, MKEY_ZERO_BASED, mk);
    ASSERT_EQ(ret, DPCP_OK);

    s_mk = mk;

    void* addr;
    ret = mk->get_address(addr);
    ASSERT_NE(addr, nullptr);
    ASSERT_EQ(buf, addr);

    uint32_t mk_id;
    ret = mk->get_id(mk_id);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(mk_id, 0);

    int32_t new_num;
    ret = mk->get_mkey_num(new_num);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_EQ(new_num, 1);

    // Create
    s_mkey = mk_id;
    dpp_rq* drq = nullptr;

    s_dpp_protocol = dpcp::DPCP_DPP_2110;
    ret = ad->create_dpp_rq(s_dpp_rq_attr, s_dpp_protocol, s_mkey, drq);
    if (ret != DPCP_OK) { // TODO!! Should be replaced with HW capabilities check
        delete s_mk;
        delete s_ad;
        return;
    }

    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(drq, nullptr);

    delete drq;

    s_dpp_protocol = dpcp::DPCP_DPP_2110_EXT;
    ret = ad->create_dpp_rq(s_dpp_rq_attr, s_dpp_protocol, s_mkey, drq);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(drq, nullptr);

    s_dpp_rq = drq;
}

/**
 * @test dpcp_rq.ti_02_get_dpp_wire_protocol
 * @brief
 *    Check dpp wire protocol
 * @details
 */
TEST_F(dpcp_dpp_rq, ti_02_get_dpp_protocol)
{
    if (s_dpp_rq == nullptr) { // TODO!! Should be removed after adding HW capabilities check
        return;
    }

    dpcp_dpp_protocol protocol;
    status ret = s_dpp_rq->get_dpp_protocol(protocol);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_EQ(protocol, s_dpp_protocol);
}

/**
 * @test dpcp_rq.ti_03_get_mkey
 * @brief
 *    Check dpp wire protocol
 * @details
 */
TEST_F(dpcp_dpp_rq, ti_03_get_mkey)
{
    if (s_dpp_rq == nullptr) { // TODO!! Should be removed after adding HW capabilities check
        return;
    }

    uint32_t mkey;
    status ret = s_dpp_rq->get_mkey(mkey);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_EQ(mkey, s_mkey);
}

/**
 * @test dpcp_rq.ti_04_modify_state
 * @brief
 *    Check striding_rq::modify_state method
 * @details
 *
 */
TEST_F(dpcp_dpp_rq, ti_04_modify_state)
{
    if (s_dpp_rq == nullptr) { // TODO!! Should be removed after adding HW capabilities check
        return;
    }

    status ret = s_dpp_rq->modify_state(RQ_RDY);
    ASSERT_EQ(ret, DPCP_OK);

    delete s_dpp_rq;
    delete s_mk;
    delete s_ad;
}