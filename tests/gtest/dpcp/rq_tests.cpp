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

static rq_attr s_rq_attr;
static adapter* s_ad;
static striding_rq* s_srq;
static regular_rq*  s_rrq;
static uint32_t s_rq_num;
static size_t s_wqe_sz;

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
    ASSERT_NE(0U, cqd.cqn);

    s_rq_attr = {2048, 16384, 0, cqd.cqn};
    s_rq_num = 4;
    s_wqe_sz = s_rq_attr.buf_stride_num * s_rq_attr.buf_stride_sz / 16; // in DS (16B)

    rq_attr rqattr = {};
    rqattr.buf_stride_sz = s_rq_attr.buf_stride_sz;
    rqattr.buf_stride_num = s_rq_attr.buf_stride_num;
    rqattr.user_index = s_rq_attr.user_index;
    rqattr.cqn = s_rq_attr.cqn;
    rqattr.wqe_num = s_rq_num;
    rqattr.wqe_sz = s_wqe_sz;

    striding_rq* srq = nullptr;
    ret = ad->create_striding_rq(rqattr, srq);
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
}

/**
 * @test dpcp_rq.ti_09_create_regular_rq
 * @brief
 *    Check pd::create method
 * @details
 *
 */
TEST_F(dpcp_rq, ti_09_create_regular_rq)
{
    s_rq_attr.buf_stride_num = 0;
    s_rq_attr.buf_stride_sz = 0;
    s_rq_attr.wqe_num = s_rq_num = 16384;
    s_rq_attr.wqe_sz = s_wqe_sz = 1; 

    regular_rq* rrq = nullptr;
    status ret = s_ad->create_regular_rq(s_rq_attr, rrq);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, rrq);
    s_rrq = rrq;
    void* buf = nullptr;
    ret = s_rrq->get_wq_buf(buf);
}
/**
 * @test dpcp_rq.ti_10_get_wq_buf_regular_rq
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_rq, ti_10_get_wq_buf_regular_rq)
{
    void* buf = nullptr;
    status ret = s_rrq->get_wq_buf(buf);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, buf);
}
/**
 * @test dpcp_rq.ti_11_get_dbrec_regular_rq
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_rq, ti_11_get_dbrec_regular_rq)
{
    uint32_t* dbrec = nullptr;
    status ret = s_rrq->get_dbrec(dbrec);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, dbrec);
}
/**
 * @test dpcp_rq.ti_12_get_uar_page_regular_rq
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_rq, ti_12_get_uar_page_regular_rq)
{
    volatile void* uar_page = nullptr;
    status ret = s_rrq->get_uar_page(uar_page);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, (void*)uar_page);
}
/**
 * @test dpcp_rq.ti_13_get_wqe_num_regular_rq
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_rq, ti_13_get_wqe_num_regular_rq)
{
    uint32_t wqe_num;
    status ret = s_rrq->get_wqe_num(wqe_num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(wqe_num, s_rq_num);
}
/**
 * @test dpcp_rq.ti_14_get_wq_stride_sz_regular_rq
 * @brief
 *    Check get_wq_stride_sz
 * @details
 */
TEST_F(dpcp_rq, ti_14_get_wq_stride_sz_regular_rq)
{
    uint32_t wq_stride_sz;
    status ret = s_rrq->get_wq_stride_sz(wq_stride_sz);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(wq_stride_sz, s_wqe_sz * 16);
}
/**
 * @test dpcp_rq.ti_15_get_wq_buf_sz_regular_rq
 * @brief
 *    Check get_wq_buf_sz
 * @details
 */
TEST_F(dpcp_rq, ti_15_get_wq_buf_sz_regular_rq)
{
    size_t wq_buf_sz = s_rrq->get_wq_buf_sz();
    log_trace("wq_buf_sz: %zd\n", wq_buf_sz);
}
/**
 * @test dpcp_rq.ti_16_modify_state_regular_rq
 * @brief
 *    Check striding_rq::modify_state method
 * @details
 *
 */
TEST_F(dpcp_rq, ti_16_modify_state_regular_rq)
{
    status ret = s_rrq->modify_state(RQ_RDY);
    ASSERT_EQ(DPCP_OK, ret);
}

/**
 * @test dpcp_rq.ti_17_create_striding_rq_attr_api
 * @brief
 *    Check pd::create_striding_rq method re_attr api
 * @details
 *
 */
TEST_F(dpcp_rq, ti_17_create_striding_rq_attr_api)
{
    s_rq_attr.buf_stride_num = 16384;
    s_rq_attr.buf_stride_sz = 2048;
    s_rq_attr.wqe_num = s_rq_num = 4;
    s_rq_attr.wqe_sz = s_wqe_sz = 32U; 

    striding_rq* srq = nullptr;
    status ret = s_ad->create_striding_rq(s_rq_attr, srq);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, srq);
    void* buf = nullptr;
    ret = srq->get_wq_buf(buf);
    delete srq;

    delete s_rrq;
    delete s_ad;
}

