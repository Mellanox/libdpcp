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

static rq_attr s_ibq_rq_attr;
static adapter* s_ad;
static ibq_rq* s_ibq_rq;
static uint32_t s_ibq_rq_num;
static direct_mkey* s_mk;
static uint32_t s_mkey;
static dpcp_ibq_protocol s_ibq_protocol = dpcp::DPCP_IBQ_NOT_INITIALIZED;

class dpcp_ibq_rq : /*public obj,*/ public dpcp_base {
protected:
    void SetUp() override
    {
        if (errno) {
            log_trace("dpcp_ibq_rq::SetUp errno= %d\n", errno);
            errno = EOK;
        }
    }
};

/**
 * @test dpcp_ibq_rq.ti_01_create
 * @brief
 *    Check pd::create method
 * @details
 *
 */
TEST_F(dpcp_ibq_rq, ti_01_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(ad, nullptr);

    status ret = ad->open();
    ASSERT_EQ(ret, DPCP_OK);

    s_ad = ad;

    cq_data cqd = {};
    ret = (status)create_cq(s_ad, &cqd);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(cqd.cqn, 0U);

    s_ibq_rq_attr = {1500, 10000, 0, cqd.cqn};

    // create buffer and obtain mkey for it
    size_t buf_length = s_ibq_rq_attr.buf_stride_sz * s_ibq_rq_attr.buf_stride_num;
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
    ASSERT_NE(mk_id, 0U);

    int32_t new_num;
    ret = mk->get_mkey_num(new_num);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_EQ(new_num, 0);

#if defined(IBQ_MKEY_UID_WA)
    // This workaround is required when direct_mkey is created with UID=0 (by ibv_reg_mr)
    // while IBQ itself will be created with UID!=0. Pattern Mkey by DPCP PRM will have the
    // same UID!=0 but indirectly will point to UID=0 direct_mkey. Still need to be sure
    // that data transfer will not issue syndrome.
    // Create indirect Mkey by PatternMkey
    pattern_mkey* ptmk = nullptr;
    pattern_mkey_bb mem_bb[1];
    mem_bb[0].m_key = mk;
    mem_bb[0].m_stride_sz = s_ibq_rq_attr.buf_stride_sz;
    mem_bb[0].m_length = buf_length;
    ret = ad->create_pattern_mkey(buf, MKEY_NONE, s_ibq_rq_attr.buf_stride_num, ARRAY_SIZE(mem_bb),
                                  mem_bb, ptmk);
    ASSERT_EQ(DPCP_OK, ret);

    ret = ptmk->get_id(mk_id);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(mk_id, 0);
#endif // defined(IBQ_MKEY_UID_WA)

    // Create
    s_mkey = mk_id;
    ibq_rq* drq = nullptr;

    s_ibq_protocol = dpcp::DPCP_IBQ_2110;
    ret = ad->create_ibq_rq(s_ibq_rq_attr, s_ibq_protocol, s_mkey, drq);
    if (ret != DPCP_OK) { // TODO!! Should be replaced with HW capabilities check
        delete s_mk;
        delete s_ad;
        return;
    }

    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(drq, nullptr);

    delete drq;

    s_ibq_protocol = dpcp::DPCP_IBQ_2110_EXT;
    ret = ad->create_ibq_rq(s_ibq_rq_attr, s_ibq_protocol, s_mkey, drq);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(drq, nullptr);

    s_ibq_rq = drq;
}

/**
 * @test dpcp_rq.ti_02_get_ibq_wire_protocol
 * @brief
 *    Check ibq wire protocol
 * @details
 */
TEST_F(dpcp_ibq_rq, ti_02_get_ibq_protocol)
{
    if (s_ibq_rq == nullptr) { // TODO!! Should be removed after adding HW capabilities check
        return;
    }

    dpcp_ibq_protocol protocol;
    status ret = s_ibq_rq->get_ibq_protocol(protocol);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_EQ(protocol, s_ibq_protocol);
}

/**
 * @test dpcp_rq.ti_03_get_mkey
 * @brief
 *    Check ibq wire protocol
 * @details
 */
TEST_F(dpcp_ibq_rq, ti_03_get_mkey)
{
    if (s_ibq_rq == nullptr) { // TODO!! Should be removed after adding HW capabilities check
        return;
    }

    uint32_t mkey;
    status ret = s_ibq_rq->get_mkey(mkey);
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
TEST_F(dpcp_ibq_rq, ti_04_modify_state)
{
    if (s_ibq_rq == nullptr) { // TODO!! Should be removed after adding HW capabilities check
        return;
    }

    status ret = s_ibq_rq->modify_state(RQ_RDY);
    ASSERT_EQ(ret, DPCP_OK);

    delete s_ibq_rq;
    delete s_mk;
    delete s_ad;
}
