/*
Copyright (C) Mellanox Technologies, Ltd. 2019-2020. ALL RIGHTS RESERVED.

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

static adapter* s_ad = nullptr;
static striding_rq* s_srq = nullptr;

class dpcp_adapter : /*public obj,*/ public dpcp_base {
protected:
    void SetUp() override
    {
        if (errno) {
            log_trace("dpcp_adapter errno= %d\n", errno);
            errno = EOK;
        }
    }
};

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
    delete ad;
}

/**
 * @test dpcp_adapter.ti_02_set_get_pd
 * @brief
 *    Check set_pd()
 * @details
 */
TEST_F(dpcp_adapter, ti_02_set_get_pd)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    uint32_t id = ad->get_pd();
    ASSERT_EQ(0, id);

    uint32_t id1 = 0xabba;
    status ret = ad->set_pd(id1);
    ASSERT_EQ(DPCP_OK, ret);

    id = ad->get_pd();
    ASSERT_NE(0, id);
    ASSERT_EQ(id, id1);

    ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    id = ad->get_pd();
    ASSERT_NE(0, id);
    ASSERT_EQ(id, id1);

    delete ad;
}

/**
 * @test dpcp_adapter.ti_03_set_get_td
 * @brief
 *    Check set_td method
 * @details
 */
TEST_F(dpcp_adapter, ti_03_set_get_td)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    uint32_t id = ad->get_td();
    ASSERT_EQ(0, id);

    uint32_t id1 = 0xabba;
    status ret = ad->set_td(id1);
    ASSERT_EQ(DPCP_OK, ret);

    id = ad->get_td();
    ASSERT_NE(0, id);
    ASSERT_EQ(id, id1);

    ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    id = ad->get_td();
    ASSERT_NE(0, id);
    ASSERT_EQ(id, id1);

    delete ad;
}

/**
 * @test dpcp_adapter.ti_04_create_direct_mkey
 * @brief
 *    Check adapter::create_direct_mkey method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_04_create_direct_mkey)
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
    ASSERT_NE(new_id, 0);

    int32_t new_num;
    ret = mk->get_mkey_num(new_num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(1, new_num);

    delete mk;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_05_mkey_zero_based
 * @brief
 *    Check adapter::create_direct_mkey method with Zero based address
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_05_mkey_zero_based)
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
    ASSERT_NE(0, new_id);

    int32_t new_num;
    ret = mk->get_mkey_num(new_num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(1, new_num);

    delete mk;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_06_create_cq
 * @brief
 *    Check adapter::create_create_cq method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_06_create_cq)
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
 * @test dpcp_adapter.ti_07_create_striding_rq
 * @brief
 *    Check adapter::create_striding_rq method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_07_create_striding_rq)
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
    ASSERT_NE(0, cqd.cqn);

    rq_attr rqattr = {};
    rqattr.buf_stride_sz = 2048;
    rqattr.buf_stride_num = 16384;
    rqattr.user_index = 0;
    rqattr.cqn = cqd.cqn;

    uint32_t rq_num = 4;
    uint32_t wqe_sz = rqattr.buf_stride_num * rqattr.buf_stride_sz / 16; // in DS (16B)

    striding_rq* srq = nullptr;
    ret = ad->create_striding_rq(rqattr, rq_num, wqe_sz, srq);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, srq);

    // s_srq = srq;
    delete srq;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_08_create_tir
 * @brief
 *    Check adapter::create_tir method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_08_create_tir)
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
    ASSERT_NE(0, cqd.cqn);

    rq_attr rqattr = {};
    rqattr.buf_stride_sz = 2048;
    rqattr.buf_stride_num = 16384;
    rqattr.user_index = 0;
    rqattr.cqn = cqd.cqn;

    uint32_t rq_num = 4;
    uint32_t wqe_sz = rqattr.buf_stride_num * rqattr.buf_stride_sz / 16; // in DS (16B)

    striding_rq* srq = nullptr;
    ret = ad->create_striding_rq(rqattr, rq_num, wqe_sz, srq);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, srq);

    uint32_t rqn = 0;
    ret = srq->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0, rqn);

    tir* t1 = nullptr;
    ret = ad->create_tir(rqn, t1);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, t1);

    delete t1;
    delete srq;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_09_create_pattern_mkey
 * @brief
 *    Check adapter::create_pattern_mkey method
 * @detail
 *
 */
TEST_F(dpcp_adapter, ti_09_create_pattern_mkey)
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
    ASSERT_NE(0, new_id);

    int32_t new_num;
    ret = ptmk->get_mkey_num(new_num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(3, new_num);

    delete ptmk;
    delete dat_mk;
    ::aligned_free(dat_buf);
    delete hdr_mk;
    ::aligned_free(hdr_buf);
    delete ad;
}
/**
 * @test dpcp_adapter.ti_08_create_dpp_rq
 * @brief
 *    Check adapter::create_dpp_rq method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_08_create_dpp_rq)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(ad, nullptr);

    status ret = ad->open();
    ASSERT_EQ(ret, DPCP_OK);

    cq_data cqd = {};
    ret = (status)create_cq(ad, &cqd);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(cqd.cqn, 0);

    rq_attr rqattr = {};
    rqattr.buf_stride_sz = 1500;
    rqattr.buf_stride_num = 100;
    rqattr.user_index = 0;
    rqattr.cqn = cqd.cqn;

    // create buffer and obtain mkey for it
    int32_t buf_length = rqattr.buf_stride_sz * rqattr.buf_stride_num;
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
    ASSERT_NE(mk_id, 0);

    int32_t new_num;
    ret = mk->get_mkey_num(new_num);
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_EQ(new_num, 1);

    // Create
    uint32_t mkey = mk_id;
    dpp_rq* drq = nullptr;
    ret = ad->create_dpp_rq(rqattr, dpcp::DPCP_DPP_2110, mkey, drq);
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
 * @test dpcp_adapter.ti_07_get_hca_freq
 * @brief
 *    Check query_hca_freq method
 * @details
 */
TEST_F(dpcp_adapter, ti_07_get_hca_freq)
{
    adapter *ad = OpenAdapter();
    ASSERT_NE(ad, nullptr);

    status ret = ad->open();
    ASSERT_EQ(ret, DPCP_OK);

    uint32_t freq = 0;

    ret = ad->get_hca_caps_frequency_khz(freq);
    if (ret != DPCP_OK) {
        delete ad;
	return;
    }
    ASSERT_NE(freq, 0);

    delete ad;
}

