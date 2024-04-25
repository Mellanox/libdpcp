/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2020-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
#include <chrono>

#include "dpcp_base.h"

using namespace dpcp;

using std::chrono::duration_cast;
using std::chrono::nanoseconds;
using std::chrono::steady_clock;

static adapter* s_ad = nullptr;
static striding_rq* s_srq = nullptr;

class dpcp_adapter : public dpcp_base {};

/**
 * @test dpcp_adapter.ti_01_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_adapter, ti_01_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    bool is_opened = ad->is_opened();
    ASSERT_FALSE(is_opened);
    delete ad;
}

/**
* @test dpcp_adapter.ti_02_is_opened
* @brief
*    Check is_opened()
* @details
*/
TEST_F(dpcp_adapter, ti_02_is_opened)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    bool is_opened_before = ad->is_opened();
    ASSERT_FALSE(is_opened_before);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    bool is_opened_after = ad->is_opened();
    ASSERT_TRUE(is_opened_after);
}

/**
 * @test dpcp_adapter.ti_03_set_get_pd
 * @brief
 *    Check set_pd()
 * @details
 */
TEST_F(dpcp_adapter, ti_03_set_get_pd)
{
    void* pd;
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    uint32_t id = ad->get_pd();
    ASSERT_EQ(0U, id);

    status ret = ad->create_ibv_pd();
    ASSERT_EQ(DPCP_OK, ret);

    ret = ad->get_ibv_pd(pd);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t id1 = 0xabba;
    ret = ad->set_pd(id1, pd);
    ASSERT_EQ(DPCP_OK, ret);

    id = ad->get_pd();
    ASSERT_NE(0U, id);
    ASSERT_EQ(id, id1);

    ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    id = ad->get_pd();
    ASSERT_NE(0U, id);
    ASSERT_EQ(id, id1);

    delete ad;
}

/**
 * @test dpcp_adapter.ti_04_set_get_pd_external
 * @brief
 *    Check set_pd()
 * @details
 */
TEST_F(dpcp_adapter, ti_04_set_get_pd_external)
{
    uint32_t pdn = 0;
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    // allocate protected domain obj
    ibv_context* ibv_ctx = (ibv_context*)ad->get_ibv_context();
    ASSERT_NE(nullptr, ibv_ctx);
    struct ibv_pd* pd_1 = ibv_alloc_pd(ibv_ctx);
    ASSERT_NE(nullptr, pd_1);

    // Get PD id from ibv_pd
#if defined(__linux__)
    mlx5dv_obj mlx5_obj = {};
    mlx5_obj.pd.in = pd_1;
    mlx5dv_pd out_pd;
    mlx5_obj.pd.out = &out_pd;
    int mlx5dv_ret = mlx5dv_init_obj(&mlx5_obj, MLX5DV_OBJ_PD);
    ASSERT_EQ(0, mlx5dv_ret);
    pdn = out_pd.pdn;
#else
    pdn = pd_1->handle;
#endif

    // Create PD on adapter using external PD
    status ret = ad->create_ibv_pd(pd_1);
    ASSERT_EQ(DPCP_OK, ret);

    // check that ibv_pd was set correctly.
    void* pd_ret = nullptr;
    ret = ad->get_ibv_pd(pd_ret);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(pd_1, pd_ret);

    // check that pd id was created correctly by comparing to mlx5dv_pd output
    uint32_t id = ad->get_pd();
    ASSERT_EQ(pdn, id);

    // check that open adapter will not change PD
    ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);
    id = ad->get_pd();
    ASSERT_EQ(pdn, id);

    // creating another PD with the same ibv_pd should not fail.
    ret = ad->create_ibv_pd(pd_1);
    ASSERT_EQ(DPCP_OK, ret);

    // createing another PD with different ibv_pd should faile.
    struct ibv_pd* pd_2 = ibv_alloc_pd(ibv_ctx);
    ASSERT_NE(nullptr, pd_2);
    ret = ad->create_ibv_pd(pd_2);
    ASSERT_NE(DPCP_OK, ret);

    ibv_dealloc_pd(pd_1);
    ibv_dealloc_pd(pd_2);
    delete ad;
}

/**
 * @test dpcp_adapter.ti_05_set_get_td
 * @brief
 *    Check set_td method
 * @details
 */
TEST_F(dpcp_adapter, ti_05_set_get_td)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    uint32_t id = ad->get_td();
    ASSERT_EQ(0U, id);

    uint32_t id1 = 0xabba;
    status ret = ad->set_td(id1);
    ASSERT_EQ(DPCP_OK, ret);

    id = ad->get_td();
    ASSERT_NE(0U, id);
    ASSERT_EQ(id, id1);

    ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    id = ad->get_td();
    ASSERT_NE(0U, id);
    ASSERT_EQ(id, id1);

    delete ad;
}

/**
 * @test dpcp_adapter.ti_06_create_direct_mkey
 * @brief
 *    Check adapter::create_direct_mkey method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_06_create_direct_mkey)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);
    int32_t length = 4096;
    void* buf = new (std::nothrow) uint8_t[length];
    ASSERT_NE(nullptr, buf);

    direct_mkey* mk = nullptr;
    ret = ad->create_direct_mkey(buf, length, (mkey_flags)0, mk);
    ASSERT_EQ(DPCP_OK, ret);

    void* addr;
    ret = mk->get_address(addr);
    ASSERT_NE(nullptr, addr);
    ASSERT_EQ(buf, addr);

    uint32_t new_id;
    ret = mk->get_id(new_id);
    ASSERT_EQ(DPCP_OK, ret);
    log_trace("CDMK Mkey Id: 0x%x\n", new_id);
    ASSERT_NE(new_id, 0U);

    int32_t new_num;
    ret = mk->get_mkey_num(new_num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(0, new_num);

    delete mk;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_07_mkey_zero_based
 * @brief
 *    Check adapter::create_direct_mkey method with Zero based address
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_07_mkey_zero_based)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);
    int32_t length = 4096;
    void* buf = new (std::nothrow) uint8_t[length];
    ASSERT_NE(nullptr, buf);

    direct_mkey* mk = nullptr;
    ret = ad->create_direct_mkey(buf, length, MKEY_ZERO_BASED, mk);
    ASSERT_EQ(DPCP_OK, ret);

    void* addr;
    ret = mk->get_address(addr);
    ASSERT_NE(nullptr, addr);
    ASSERT_EQ(buf, addr);

    uint32_t new_id;
    ret = mk->get_id(new_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, new_id);

    int32_t new_num;
    ret = mk->get_mkey_num(new_num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(0, new_num);

    delete mk;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_08_create_cq
 * @brief
 *    Check adapter::create_create_cq method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_08_create_cq)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t eqn = 0;
    ret = ad->query_eqn(eqn);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t cqe_num = 16384;
    std::bitset<ATTR_CQ_MAX_CNT_FLAG> flags;
    flags.set(ATTR_CQ_NONE_FLAG);
    std::bitset<CQ_ATTR_MAX_CNT> cq_attr_use;
    cq_attr_use.set(CQ_SIZE);
    cq_attr_use.set(CQ_EQ_NUM);
    cq_attr attr = {cqe_num, eqn, {0, 0}};
    attr.flags = flags;
    attr.cq_attr_use = cq_attr_use;
    cq* pcq = nullptr;
    ret = ad->create_cq(attr, pcq);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, pcq);

    delete pcq;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_09_create_striding_rq
 * @brief
 *    Check adapter::create_striding_rq method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_09_create_striding_rq)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);
    int32_t length = 4096;
    void* buf = new (std::nothrow) uint8_t[length];
    ASSERT_NE(nullptr, buf);

    cq_data cqd = {};
    ret = (status)create_cq(ad, &cqd);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, cqd.cqn);

    rq_attr rqattr = {};
    rqattr.buf_stride_sz = 2048;
    rqattr.buf_stride_num = 16384;
    rqattr.user_index = 0;
    rqattr.cqn = cqd.cqn;
    rqattr.wqe_num = 4;
    rqattr.wqe_sz = rqattr.buf_stride_num * rqattr.buf_stride_sz / 16; // in DS (16B)

    striding_rq* srq = nullptr;
    ret = ad->create_striding_rq(rqattr, srq);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, srq);

    // s_srq = srq;
    delete srq;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_10_create_tir
 * @brief
 *    Check adapter::create_tir method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_10_create_tir)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);
    int32_t length = 4096;
    void* buf = new (std::nothrow) uint8_t[length];
    ASSERT_NE(nullptr, buf);

    cq_data cqd = {};
    ret = (status)create_cq(ad, &cqd);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, cqd.cqn);

    rq_attr rqattr = {};
    rqattr.buf_stride_sz = 2048;
    rqattr.buf_stride_num = 16384;
    rqattr.user_index = 0;
    rqattr.cqn = cqd.cqn;
    rqattr.wqe_num = 4;
    rqattr.wqe_sz = rqattr.buf_stride_num * rqattr.buf_stride_sz / 16; // in DS (16B)

    striding_rq* srq = nullptr;
    ret = ad->create_striding_rq(rqattr, srq);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, srq);

    uint32_t rqn = 0;
    ret = srq->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rqn);

    tir* t1 = nullptr;
    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = TIR_ATTR_INLINE_RQN | TIR_ATTR_TRANSPORT_DOMAIN;
    tir_attr.inline_rqn = rqn;
    tir_attr.transport_domain = ad->get_td();
    ret = ad->create_tir(tir_attr, t1);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, t1);

    delete t1;
    delete srq;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_11_create_pattern_mkey
 * @brief
 *    Check adapter::create_pattern_mkey method
 * @detail
 *
 */
TEST_F(dpcp_adapter, ti_11_create_pattern_mkey)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);
    // create buffers and obtain mkeys for it
    int32_t strides_num = 256;
    int32_t hdr_stride_sz = 16;
    int32_t hdr_len = 14;
    int32_t hdr_buf_sz = strides_num * hdr_stride_sz;
    uint8_t* hdr_buf = (uint8_t*)::aligned_alloc(16, hdr_buf_sz);
    ASSERT_NE(nullptr, hdr_buf);

    direct_mkey* hdr_mk = nullptr;
    ret = ad->create_direct_mkey(hdr_buf, hdr_buf_sz, MKEY_NONE, hdr_mk);
    ASSERT_EQ(DPCP_OK, ret);

    int32_t dat_stride_sz = 512;
    int32_t dat_len = 512;
    int32_t dat_buf_sz = strides_num * dat_stride_sz;
    uint8_t* dat_buf = (uint8_t*)::aligned_alloc(16, dat_buf_sz);
    ASSERT_NE(nullptr, dat_buf);

    direct_mkey* dat_mk = nullptr;
    ret = ad->create_direct_mkey(dat_buf, dat_buf_sz, MKEY_NONE, dat_mk);
    ASSERT_EQ(DPCP_OK, ret);
    // prepare memory descriptor
    pattern_mkey_bb mem_bb[2];
    mem_bb[0].m_key = hdr_mk;
    mem_bb[0].m_stride_sz = hdr_stride_sz;
    mem_bb[0].m_length = hdr_len;

    mem_bb[1].m_key = dat_mk;
    mem_bb[1].m_stride_sz = dat_stride_sz;
    mem_bb[1].m_length = dat_len;

    pattern_mkey* ptmk = nullptr;
    ret =
        ad->create_pattern_mkey(hdr_buf, MKEY_NONE, strides_num, ARRAY_SIZE(mem_bb), mem_bb, ptmk);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t new_id;
    ret = ptmk->get_id(new_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, new_id);

    int32_t new_num;
    ret = ptmk->get_mkey_num(new_num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(1, new_num);

    delete ptmk;
    delete dat_mk;
    ::aligned_free(dat_buf);
    delete hdr_mk;
    ::aligned_free(hdr_buf);
    delete ad;
}
/**
 * @test dpcp_adapter.ti_12_create_ibq_rq
 * @brief
 *    Check adapter::create_ibq_rq method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_12_create_ibq_rq)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(ad, nullptr);

    status ret = ad->open();
    ASSERT_EQ(ret, DPCP_OK);

    cq_data cqd = {};
    ret = (status)create_cq(ad, &cqd);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(cqd.cqn, 0U);

    rq_attr rqattr = {};
    rqattr.buf_stride_sz = 1500;
    rqattr.buf_stride_num = 100;
    rqattr.user_index = 0;
    rqattr.cqn = cqd.cqn;

    // create buffer and obtain mkey for it
    size_t buf_length = rqattr.buf_stride_sz * rqattr.buf_stride_num;
    void* buf = new (std::nothrow) uint8_t[buf_length];
    ASSERT_NE(buf, nullptr);

    direct_mkey* mk = nullptr;
    ret = ad->create_direct_mkey(buf, buf_length, MKEY_ZERO_BASED, mk);
    ASSERT_EQ(ret, DPCP_OK);

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

    // Create
    uint32_t mkey = mk_id;
    ibq_rq* drq = nullptr;
    ret = ad->create_ibq_rq(rqattr, dpcp::DPCP_IBQ_2110, mkey, drq);
    if (ret != DPCP_OK) { // TODO!! Should be replaced with HW capabilities check
        delete mk;
        delete ad;
        return;
    }
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(drq, nullptr);

    delete drq;
    delete mk;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_13_get_hca_freq
 * @brief
 *    Check query_hca_freq method
 * @details
 */
TEST_F(dpcp_adapter, ti_13_get_hca_freq)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(ad, nullptr);

    status ret = ad->open();
    ASSERT_EQ(ret, DPCP_OK);

    uint32_t freq = 0;

    ret = ad->get_hca_caps_frequency_khz(freq);
    if (ret != DPCP_OK) {
        delete ad;
        return;
    }
    ASSERT_NE(freq, 0U);

    delete ad;
}

/**
 * @test dpcp_adapter.ti_14_get_real_time
 *
 */
TEST_F(dpcp_adapter, ti_14_get_real_time)
{
    adapter* ad = OpenAdapter(DevPartIdBlueField);
    if (!ad) {
        log_warn("Adapter with PCI DevID %x not found, test is not run!\n", DevPartIdBlueField);
        return;
    }
    status ret = ad->open();
    ASSERT_EQ(ret, DPCP_OK);

    adapter_hca_capabilities caps;
    ret = ad->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    if (!caps.sq_ts_format) {
        log_trace("RTC is is not enabled\n");
        delete ad;
        return;
    }

    uint64_t real_time, real_time_after = 0;
    uint64_t chrono_time, chrono_time_after = 0;

    ret = ad->get_real_time(real_time);
    chrono_time =
        (uint64_t)duration_cast<std::chrono::nanoseconds>(steady_clock::now().time_since_epoch())
            .count();
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(real_time, 0U);

    std::this_thread::sleep_for(std::chrono::microseconds(10));

    ret = ad->get_real_time(real_time_after);
    chrono_time_after =
        (uint64_t)duration_cast<std::chrono::nanoseconds>(steady_clock::now().time_since_epoch())
            .count();
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(real_time_after, 0U);

    ASSERT_GT(real_time_after, real_time);

    uint64_t real_time_delta = real_time_after - real_time;
    uint64_t chrono_delta = chrono_time_after - chrono_time;
    uint64_t delta = std::llabs(real_time_delta - chrono_delta);

    ASSERT_LE(delta, 50000);
}
/*
 * @test dpcp_adapter.ti_15_create_pp_sq
 * @brief
 *    Check adapter::create_pp_sq method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_15_create_pp_sq)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);
    int32_t length = 4096;
    void* buf = new (std::nothrow) uint8_t[length];
    ASSERT_NE(nullptr, buf);

    cq_data cqd = {};
    ret = (status)create_cq(ad, &cqd);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, cqd.cqn);

    tis* s_tis;
    struct tis::attr tis_attr;
    memset(&tis_attr, 0, sizeof(tis_attr));
    tis_attr.flags = TIS_ATTR_TRANSPORT_DOMAIN;
    tis_attr.transport_domain = ad->get_td();
    ret = ad->create_tis(tis_attr, s_tis);
    ASSERT_EQ(DPCP_OK, ret);
    uint32_t tis_n = 0;
    ret = s_tis->get_tisn(tis_n);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, tis_n);

    sq_attr sqattr = {};
    qos_attributes qos_attr;
    qos_attr.qos_type = QOS_TYPE::QOS_PACKET_PACING;
    qos_attr.qos_attr.packet_pacing_attr.burst_sz = 1;
    qos_attr.qos_attr.packet_pacing_attr.packet_sz = 1200;
    qos_attr.qos_attr.packet_pacing_attr.sustained_rate = 1200000;
    sqattr.qos_attrs_sz = 1;
    sqattr.qos_attrs = &qos_attr;
    sqattr.user_index = 0;
    sqattr.wqe_num = 32768;
    sqattr.wqe_sz = 64;

    sqattr.cqn = cqd.cqn;
    sqattr.tis_num = tis_n;

    pp_sq* ppsq = nullptr;
    ret = ad->create_pp_sq(sqattr, ppsq);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, ppsq);

    delete ppsq;
    delete ad;
}

#if defined(__linux__)
/**
 * @test dpcp_adapter.ti_16_set_get_ibv_pd
 * @brief
 *    Check set_pd() with ibv_pd* and get_ibv_pd
 * @details
 */
TEST_F(dpcp_adapter, ti_16_set_get_ibv_pd)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    ibv_context* ibv_ctx = (ibv_context*)ad->get_ibv_context();
    ASSERT_NE(nullptr, ibv_ctx);

    struct ibv_pd* pd = ibv_alloc_pd(ibv_ctx);
    ASSERT_NE(nullptr, pd);
    // Get PD id
    mlx5dv_obj mlx5_obj = {};
    mlx5_obj.pd.in = pd;
    mlx5dv_pd out_pd;
    mlx5_obj.pd.out = &out_pd;
    int ret = mlx5dv_init_obj(&mlx5_obj, MLX5DV_OBJ_PD);
    ASSERT_EQ(DPCP_OK, ret);

    void* p_ibv_pd = nullptr;
    ret = ad->get_ibv_pd(p_ibv_pd);
    ASSERT_EQ(DPCP_ERR_NO_CONTEXT, ret);

    ret = ad->set_pd(out_pd.pdn, pd);
    ASSERT_EQ(DPCP_OK, ret);

    ret = ad->get_ibv_pd(p_ibv_pd);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(pd, p_ibv_pd);

    uint id = ad->get_pd();
    ASSERT_EQ(out_pd.pdn, id);

    ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    id = ad->get_pd();
    ASSERT_NE(0, id);
    ASSERT_EQ(out_pd.pdn, id);

    delete ad;
}

/**
 * @test dpcp_adapter.ti_17_get_hca_capabilities
 * @brief
 *    Check query_hca_freq method
 * @details
 */
TEST_F(dpcp_adapter, ti_17_get_hca_capabilities)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(ad, nullptr);

    status ret = ad->open();
    ASSERT_EQ(ret, DPCP_OK);

    adapter_hca_capabilities caps;
    ret = ad->get_hca_capabilities(caps);
    if (ret != DPCP_OK) {
        delete ad;
        return;
    }

    log_trace("Capability - device_frequency_khz: %d\n", caps.device_frequency_khz);
    log_trace("Capability - tls_tx: %d\n", caps.tls_tx);
    log_trace("Capability - tls_rx: %d\n", caps.tls_rx);
    log_trace("Capability - general_object_types_encryption_key: %d\n", caps.general_object_types_encryption_key);
    log_trace("Capability - log_max_dek: %d\n", caps.log_max_dek);
    log_trace("Capability - tls_1_2_aes_gcm_128: %d\n", caps.tls_1_2_aes_gcm_128);
    log_trace("Capability - tls_1_2_aes_gcm_256: %d\n", caps.tls_1_2_aes_gcm_256);
    log_trace("Capability - synchronize_dek: %d\n", caps.synchronize_dek);
    log_trace("Capability - log_max_num_deks: %d\n", caps.log_max_num_deks);
    log_trace("Capability - sq_ts_format: %d\n", caps.sq_ts_format);
    log_trace("Capability - rq_ts_format: %d\n", caps.rq_ts_format);
    log_trace("Capability - lro_cap: %d\n", caps.lro_cap);
    if (caps.nvmeotcp_caps.enabled) {
        log_trace("Capability - nvmeotcp: 1, zerocopy:%d, crc_rx: %d, crc_tx: %d, version: %d, "
                  "log_max_nvmeotcp_tag_buffer_table: %d, log_max_nvmeotcp_tag_buffer_size: %d\n",
                  caps.nvmeotcp_caps.zerocopy, caps.nvmeotcp_caps.crc_rx, caps.nvmeotcp_caps.crc_tx,
                  caps.nvmeotcp_caps.version, caps.nvmeotcp_caps.log_max_nvmeotcp_tag_buffer_table,
                  caps.nvmeotcp_caps.log_max_nvmeotcp_tag_buffer_size);
    } else {
        log_trace("Capability - nvmeotcp: 0\n");
    }

    delete ad;
}

/**
 * @test dpcp_adapter.ti_18_create_tis
 * @brief
 *    Check adapter::create_tis method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_18_create_tis)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    tis* _tis = nullptr;
    struct tis::attr tis_attr;
    memset(&tis_attr, 0, sizeof(tis_attr));
    tis_attr.flags = TIS_ATTR_TRANSPORT_DOMAIN;
    tis_attr.transport_domain = ad->get_td();
    ret = ad->create_tis(tis_attr, _tis);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, _tis);

    uint32_t tisn = 0;
    ret = _tis->get_tisn(tisn);
    ASSERT_EQ(DPCP_OK, ret);
    log_trace("tisn: 0x%x\n", tisn);

    delete _tis;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_19_create_tls_tis
 * @brief
 *    Check adapter::create_tls with @ref tis_flags::TIS_TLS_EN method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_19_create_tls_tis)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    adapter_hca_capabilities caps;
    ret = ad->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    bool is_tls_supported = (caps.tls_tx || caps.tls_rx)
        && caps.general_object_types_encryption_key
        && caps.log_max_dek && caps.tls_1_2_aes_gcm_128;
    if (!is_tls_supported) {
        log_trace("TLS is not supported\n");
        delete ad;
        return;
    }

    tis* _tis = nullptr;
    struct tis::attr tis_attr;
    memset(&tis_attr, 0, sizeof(tis_attr));
    tis_attr.flags = TIS_ATTR_TRANSPORT_DOMAIN | TIS_ATTR_TLS | TIS_ATTR_PD;
    tis_attr.transport_domain = ad->get_td();
    tis_attr.tls_en = 1;
    tis_attr.pd = ad->get_pd();
    ret = ad->create_tis(tis_attr, _tis);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, _tis);

    uint32_t tisn = 0;
    ret = _tis->get_tisn(tisn);
    ASSERT_EQ(DPCP_OK, ret);
    log_trace("tisn: 0x%x\n", tisn);

    delete _tis;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_20_create_dek
 * @brief
 *    Check adapter::create_dek method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_20_create_dek)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    adapter_hca_capabilities caps;
    ret = ad->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    bool is_dek_supported = caps.general_object_types_encryption_key
        && caps.log_max_dek;
    if (!is_dek_supported) {
        log_trace("DEK is not supported\n");
        delete ad;
        return;
    }

    const uint32_t tls_cipher_aes_gcm_128_key_size = 16;
    uint32_t key_size_bytes = tls_cipher_aes_gcm_128_key_size;
    void* key = new char[key_size_bytes];

    memcpy(key, "a6a7ee7abec9c4ce", key_size_bytes);  // Random key for the test.
    key_size_bytes = key_size_bytes;

    tls_dek* _dek = nullptr;
    struct dek_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.key_blob = key;
    attr.key_blob_size = key_size_bytes;
    attr.key_size = key_size_bytes;
    attr.pd_id = ad->get_pd();
    ret = ad->create_tls_dek(attr, _dek);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek->get_key_id();
    log_trace("key_id: 0x%x\n", key_id);

    delete ad;
}
#endif

/**
 * @test dpcp_adapter.ti_21_create_tir_by_attr
 * @brief
 *    Check adapter::create_tir method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_21_create_tir_by_attr)
{
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    status ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = adapter_obj->get_td();
    ASSERT_NE(0U, tdn);

    striding_rq* srq_obj = open_str_rq(adapter_obj, m_rqp);
    ASSERT_NE(nullptr, srq_obj);

    uint32_t rqn = 0;
    ret = srq_obj->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rqn);
    log_trace("tdn: 0x%x rqn: 0x%x\n", tdn, rqn);

    tir* tir_obj = nullptr;
    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = TIR_ATTR_INLINE_RQN | TIR_ATTR_TRANSPORT_DOMAIN;
    tir_attr.inline_rqn = rqn;
    tir_attr.transport_domain = tdn;
    ret = adapter_obj->create_tir(tir_attr, tir_obj);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, tir_obj);

    delete tir_obj;
    delete srq_obj;
    delete adapter_obj;
}

/**
 * @test dpcp_adapter.ti_22_create_ref_mkey
 * @brief
 *    Check adapter::create_ref_mkey method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_22_create_ref_mkey)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);
    int32_t length = 4096;
    uint8_t* buf = new (std::nothrow) uint8_t[length];
    ASSERT_NE(nullptr, buf);

    direct_mkey* parent = nullptr;
    ret = ad->create_direct_mkey(buf, length, (mkey_flags)0, parent);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t parent_id;
    ret = parent->get_id(parent_id);
    ASSERT_EQ(DPCP_OK, ret);

    // create reference
    ref_mkey* ref = nullptr;
    ret = ad->create_ref_mkey(parent, buf + 1, length - 2, ref);
    ASSERT_EQ(DPCP_OK, ret);

    void* addr;
    ret = ref->get_address(addr);
    ASSERT_NE(nullptr, addr);
    ASSERT_EQ(buf + 1, addr);

    uint32_t new_id;
    ret = ref->get_id(new_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(new_id, parent_id);

    size_t new_length;
    ret = ref->get_length(new_length);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(new_length, length - 2);

    delete ref;
    delete parent;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_23_create_flow_table_by_attr
 * @brief
 *    Check adapter::create_flow_table method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_23_create_flow_table_by_attr)
{
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    std::shared_ptr<flow_table> table;
    flow_table_attr attr;
    attr.def_miss_action = flow_table_miss_action::FT_MISS_ACTION_DEF;
    attr.op_mod = flow_table_op_mod::FT_OP_MOD_NORMAL;
    attr.level = 1;
    attr.log_size = 4;
    attr.type = flow_table_type::FT_RX;

    status ret = adapter_obj->create_flow_table(attr, table);
    ASSERT_EQ(DPCP_OK, ret);

    ret = table->create();
    ASSERT_EQ(DPCP_OK, ret);

    delete adapter_obj;
}

/**
 * @test dpcp_adapter.ti_24_get_root_flow_table_tx_rx
 * @brief
 *    Check adapter::get_root_table method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_24_get_root_flow_table_tx_rx)
{
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    // Get root table rx, check that we get the same pointer if we call twice.
    std::shared_ptr<flow_table> root_rx1(adapter_obj->get_root_table(flow_table_type::FT_RX));
    ASSERT_NE(root_rx1, nullptr);
    std::shared_ptr<flow_table> root_rx2(adapter_obj->get_root_table(flow_table_type::FT_RX));
    ASSERT_NE(root_rx2, nullptr);
    ASSERT_EQ(root_rx1, root_rx2);

    // Get root table tx, check that we get the same pointer if we call twice.
    std::shared_ptr<flow_table> root_tx1(adapter_obj->get_root_table(flow_table_type::FT_TX));
    ASSERT_NE(root_tx1, nullptr);
    std::shared_ptr<flow_table> root_tx2(adapter_obj->get_root_table(flow_table_type::FT_TX));
    ASSERT_NE(root_tx2, nullptr);
    ASSERT_EQ(root_tx1, root_tx2);

    // Ceck RX and TX is not the same obj
    ASSERT_NE(root_tx1, root_rx1);

    delete adapter_obj;
}

/**
 * @test dpcp_adapter.ti_25_create_extern_mkey
 * @brief
 *    Check adapter::create_extern_mkey method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_25_create_extern_mkey)
{
    auto* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    auto ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    const size_t length = 4096;
    auto* address = new (std::nothrow) uint8_t[length];
    ASSERT_NE(nullptr, address);

    direct_mkey* someRegisteredKey;
    ret = ad->create_direct_mkey(address, length, (mkey_flags)0, someRegisteredKey);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t expectedId;
    ret = someRegisteredKey->get_id(expectedId);
    ASSERT_EQ(DPCP_OK, ret);

    extern_mkey* mkey = nullptr;
    ret = ad->create_extern_mkey(address, length, expectedId, mkey);
    ASSERT_NE(nullptr, mkey);

    uint32_t receivedId;
    ret = mkey->get_id(receivedId);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(expectedId, receivedId);

    void* receivedAddr;
    ret = mkey->get_address(receivedAddr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(address, receivedAddr);

    size_t receivedLength;
    ret = mkey->get_length(receivedLength);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(length, receivedLength);

    delete someRegisteredKey;
    delete[] address;
    delete ad;
}

/**
* @test dpcp_adapter.DISABLED_perf_100k_dek_modify
* @brief
*    Check 100K dek modify() performance
* @details
*/
TEST_F(dpcp_adapter, DISABLED_perf_100k_dek_modify)
{
    std::unique_ptr<adapter> ad(OpenAdapter());
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    adapter_hca_capabilities caps;
    ret = ad->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    bool is_tls_tx_supported = caps.tls_tx
        && caps.general_object_types_encryption_key
        && caps.log_max_num_deks && caps.tls_1_2_aes_gcm_256
        && caps.synchronize_dek;
    if (!is_tls_tx_supported) {
        log_trace("TLS TX DEK modify is not supported\n");
        return;
    }

    uint32_t tdn = ad->get_td();
    ASSERT_NE(0U, tdn);

    uint32_t pdn = ad->get_pd();
    ASSERT_NE(0U, pdn);
    log_trace("pdn: 0x%x\n", pdn);

    const size_t LOOP_COUNT = 500000U;
    const size_t SUB_LOOP_COUNT = 10000U;
    std::vector<std::unique_ptr<dek>> dek_arr(LOOP_COUNT);
    std::vector<std::unique_ptr<char[]>> key_arr(LOOP_COUNT);
    std::vector<std::unique_ptr<char[]>> key_arr2(LOOP_COUNT);
    const std::vector<std::unique_ptr<char[]>> *key_arr_ptr = &key_arr2;
    uint32_t key_size_bytes = 32;

    std::vector<char> key_base = {'a','6','a','7','e','e','7','a','b','e','c','9','c','4','c','e',
                                  'a','6','a','7','e','e','7','a','b','e','c','9','c','4','c','e'};
    uint64_t* ptr_key = reinterpret_cast<uint64_t*>(key_base.data());
    dpcp::tls_dek* temp_dek_ptr = nullptr;

    log_trace("Creating %zu Keys ...\n", SUB_LOOP_COUNT);

    for (size_t idx = 0U; idx < SUB_LOOP_COUNT; ++idx) {
        key_arr[idx].reset(new char[key_size_bytes]);
        key_arr2[idx].reset(new char[key_size_bytes]);
        memcpy(key_arr[idx].get(), key_base.data(), key_size_bytes);  // Random key for the test.
        ++*ptr_key;
        memcpy(key_arr2[idx].get(), key_base.data(), key_size_bytes);  // Random key for the test.

        dek_attr attr;
        memset(&attr, 0, sizeof(attr));
        attr.key_blob = key_arr[idx].get();
        attr.key_blob_size = key_size_bytes;
        attr.key_size = key_size_bytes;
        attr.pd_id = ad->get_pd();
        ret = ad->create_tls_dek(attr, temp_dek_ptr);
        ASSERT_EQ(DPCP_OK, ret);
        dek_arr[idx].reset(temp_dek_ptr);
        ++*ptr_key;
    }

    int64_t pid = sys_getpid();

    static constexpr size_t FMT_MAX_SIZE = 512U;
    char timestr[FMT_MAX_SIZE] = {'\0'};
    auto temp_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    strftime(timestr, FMT_MAX_SIZE - 1U, "%F %T %Z", localtime(&temp_now));

    log_trace("[PID-%" PRId64 "] Measurement started: %s (%zu)\n", pid, timestr, LOOP_COUNT);

#define perf_log_trace(prefix, diff) \
    log_trace("[PID-%zu] " prefix " Time, Time(sec): %.2f, Per-Second: %llu, Latency(usec): %llu\n", \
        pid, static_cast<double>(diff) / 1000.0, (diff != 0 ? (LOOP_COUNT * 1000ULL) / diff : 0ULL), (diff * 1000ULL) / LOOP_COUNT)

    auto temp_ts = std::chrono::high_resolution_clock::now();
    auto diff_ts = std::chrono::duration_cast<std::chrono::milliseconds>(temp_ts - temp_ts);
    auto totalCount = diff_ts.count();
    auto tc_dek_modify = totalCount;

    dek_attr modify_attr = {};
    auto iterations = LOOP_COUNT / SUB_LOOP_COUNT;
    while (iterations-- > 0ULL) {
        auto start_ts = std::chrono::high_resolution_clock::now();
        for (size_t idx = 0U; idx < SUB_LOOP_COUNT; ++idx) {
            modify_attr.key_blob = (*key_arr_ptr)[idx].get();
            modify_attr.key_blob_size = key_size_bytes;
            modify_attr.key_size = key_size_bytes;
            ret = dek_arr[idx]->modify(modify_attr);
            ASSERT_EQ(DPCP_OK, ret);
        }

        auto end_ts = std::chrono::high_resolution_clock::now();
        diff_ts = std::chrono::duration_cast<std::chrono::milliseconds>(end_ts - start_ts);
        tc_dek_modify += diff_ts.count();
        totalCount += diff_ts.count();

        key_arr_ptr = (key_arr_ptr == &key_arr2 ? &key_arr : &key_arr2);
        ret = ad->sync_crypto_tls();
        ASSERT_EQ(DPCP_OK, ret);
    }

    perf_log_trace("DEK Modify", tc_dek_modify);

    log_trace("[PID-%zu] Overall Time, Time(sec): %.2f, Per-Second: %llu, Latency(usec): %llu\n",
        pid, totalCount / 1000.0, (totalCount != 0 ? (LOOP_COUNT * 1000ULL) / totalCount : 0), (totalCount * 1000ULL) / LOOP_COUNT);

    memset(timestr, 0, sizeof(timestr));
    temp_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    strftime(timestr, FMT_MAX_SIZE - 1U, "%F %T %Z", localtime(&temp_now));
    log_trace("[PID-%zu] Measurement finished: %s\n", pid, timestr);
}
