/*
Copyright (C) Mellanox Technologies, Ltd. 2019-2021. ALL RIGHTS RESERVED.

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
    ASSERT_EQ(0, id);

    status ret = ad->create_ibv_pd();
    ASSERT_EQ(DPCP_OK, ret);

    ret = ad->get_ibv_pd(pd);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t id1 = 0xabba;
    ret = ad->set_pd(id1, pd);
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
 * @test dpcp_adapter.ti_04_set_get_td
 * @brief
 *    Check set_td method
 * @details
 */
TEST_F(dpcp_adapter, ti_04_set_get_td)
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
 * @test dpcp_adapter.ti_05_create_direct_mkey
 * @brief
 *    Check adapter::create_direct_mkey method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_05_create_direct_mkey)
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
    ASSERT_EQ(0, new_num);

    delete mk;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_06_mkey_zero_based
 * @brief
 *    Check adapter::create_direct_mkey method with Zero based address
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_06_mkey_zero_based)
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
    ASSERT_EQ(0, new_num);

    delete mk;
    delete ad;
}

/**
 * @test dpcp_adapter.ti_07_create_cq
 * @brief
 *    Check adapter::create_create_cq method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_07_create_cq)
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
 * @test dpcp_adapter.ti_08_create_striding_rq
 * @brief
 *    Check adapter::create_striding_rq method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_08_create_striding_rq)
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
 * @test dpcp_adapter.ti_09_create_tir
 * @brief
 *    Check adapter::create_tir method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_09_create_tir)
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
 * @test dpcp_adapter.ti_10_create_pattern_mkey
 * @brief
 *    Check adapter::create_pattern_mkey method
 * @detail
 *
 */
TEST_F(dpcp_adapter, ti_10_create_pattern_mkey)
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
    ASSERT_EQ(1, new_num);

    delete ptmk;
    delete dat_mk;
    ::aligned_free(dat_buf);
    delete hdr_mk;
    ::aligned_free(hdr_buf);
    delete ad;
}
/**
 * @test dpcp_adapter.ti_11_create_dpp_rq
 * @brief
 *    Check adapter::create_dpp_rq method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_11_create_dpp_rq)
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
    ASSERT_EQ(new_num, 0);

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
 * @test dpcp_adapter.ti_12_get_hca_freq
 * @brief
 *    Check query_hca_freq method
 * @details
 */
TEST_F(dpcp_adapter, ti_12_get_hca_freq)
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
    ASSERT_NE(freq, 0);

    delete ad;
}

/**
 * @test dpcp_adapter.ti_13_get_real_time
 *
 */
TEST_F(dpcp_adapter, ti_13_get_real_time)
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
    ASSERT_NE(real_time, 0);

    std::this_thread::sleep_for(std::chrono::microseconds(10));

    ret = ad->get_real_time(real_time_after);
    chrono_time_after =
        (uint64_t)duration_cast<std::chrono::nanoseconds>(steady_clock::now().time_since_epoch())
            .count();
    ASSERT_EQ(ret, DPCP_OK);
    ASSERT_NE(real_time_after, 0);

    ASSERT_GT(real_time_after, real_time);

    uint64_t real_time_delta = real_time_after - real_time;
    uint64_t chrono_delta = chrono_time_after - chrono_time;
    uint64_t delta = std::llabs(real_time_delta - chrono_delta);

    ASSERT_LE(delta, 50000);
}
/*
 * @test dpcp_adapter.ti_14_create_pp_sq
 * @brief
 *    Check adapter::create_pp_sq method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_14_create_pp_sq)
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

    tis* s_tis;
    ret = ad->create_tis(0, s_tis);
    ASSERT_EQ(DPCP_OK, ret);
    uint32_t tis_n = 0;
    ret = s_tis->get_tisn(tis_n);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0, tis_n);

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
 * @test dpcp_adapter.ti_15_set_get_ibv_pd
 * @brief
 *    Check set_pd() with ibv_pd* and get_ibv_pd
 * @details
 */
TEST_F(dpcp_adapter, ti_15_set_get_ibv_pd)
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
 * @test dpcp_adapter.ti_16_get_hca_capabilities
 * @brief
 *    Check query_hca_freq method
 * @details
 */
TEST_F(dpcp_adapter, ti_16_get_hca_capabilities)
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
    log_trace("Capability - sq_ts_format: %d\n", caps.sq_ts_format);
    log_trace("Capability - rq_ts_format: %d\n", caps.rq_ts_format);
    log_trace("Capability - lro_cap: %d\n", caps.lro_cap);

    delete ad;
}

/**
 * @test dpcp_adapter.ti_17_create_tis
 * @brief
 *    Check adapter::create_tis method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_17_create_tis)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    tis* _tis = nullptr;
    uint64_t flags = tis_flags::TIS_NONE;
    ad->create_tis(flags, _tis);
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
 * @test dpcp_adapter.ti_18_create_tls_tis
 * @brief
 *    Check adapter::create_tls with @ref tis_flags::TIS_TLS_EN method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_18_create_tls_tis)
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
    uint64_t flags = tis_flags::TIS_TLS_EN;
    ad->create_tis(flags, _tis);
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
 * @test dpcp_adapter.ti_19_create_dek
 * @brief
 *    Check adapter::create_dek method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_19_create_dek)
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

    dek* _dek = nullptr;
    ret = ad->create_dek(
        encryption_key_type_t::ENCRYPTION_KEY_TYPE_TLS,
        key, key_size_bytes, _dek);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek->get_key_id();
    log_trace("key_id: 0x%x\n", key_id);

    delete ad;
}
#endif

/**
 * @test dpcp_adapter.ti_20_create_tir_by_attr
 * @brief
 *    Check adapter::create_tir method
 * @details
 *
 */
TEST_F(dpcp_adapter, ti_20_create_tir_by_attr)
{
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    status ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = adapter_obj->get_td();
    ASSERT_NE(0, tdn);

    striding_rq* srq_obj = open_str_rq(adapter_obj, m_rqp);
    ASSERT_NE(nullptr, srq_obj);

    uint32_t rqn = 0;
    ret = srq_obj->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0, rqn);
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
