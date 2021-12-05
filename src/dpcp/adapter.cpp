/*
 * Copyright (C) Mellanox Technologies, Ltd. 2020. ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Mellanox Technologies, Ltd.
 * (the "Company") and all right, title, and interest in and to the software
 * product, including all associated intellectual property rights, are and shall
 * remain exclusively with the Company. All rights in or to the software product
 * are licensed, not sold. All rights not licensed are reserved.
 *
 * This software product is governed by the End User License Agreement provided
 * with the software product.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>
#include <vector>

#include "utils/os.h"
#include "dpcp/internal.h"

namespace dpcp {

static void store_hca_device_frequency_khz_caps(adapter_hca_capabilities* external_hca_caps,
                                                const caps_map_t& caps_map)
{
    external_hca_caps->device_frequency_khz =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.device_frequency_khz);
    log_trace("Capability - device_frequency_khz: %d\n", external_hca_caps->device_frequency_khz);
}

static void store_hca_tls_caps(adapter_hca_capabilities* external_hca_caps,
                               const caps_map_t& caps_map)
{
    external_hca_caps->tls_tx = DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                                         capability.cmd_hca_cap.tls_tx);
    log_trace("Capability - tls_tx: %d\n", external_hca_caps->tls_tx);

    external_hca_caps->tls_rx = DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                                         capability.cmd_hca_cap.tls_rx);
    log_trace("Capability - tls_rx: %d\n", external_hca_caps->tls_rx);
}

static void store_hca_cap_crypto_enable(adapter_hca_capabilities* external_hca_caps,
                                        const caps_map_t& caps_map)
{
    external_hca_caps->crypto_enable = DEVX_GET(
        query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second, capability.cmd_hca_cap.crypto);
    log_trace("Capability - crypto: %d\n", external_hca_caps->crypto_enable);
}

static void store_hca_general_object_types_encryption_key_caps(
    adapter_hca_capabilities* external_hca_caps, const caps_map_t& caps_map)
{
    uint64_t general_obj_types =
        DEVX_GET64(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                   capability.cmd_hca_cap.general_obj_types);
    if (general_obj_types & MLX5_HCA_CAP_GENERAL_OBJECT_TYPES_ENCRYPTION_KEY) {
        external_hca_caps->general_object_types_encryption_key = true;
    }

    log_trace("Capability - general_object_types_encryption_key: %d\n",
              external_hca_caps->general_object_types_encryption_key);
}

static void store_hca_log_max_dek_caps(adapter_hca_capabilities* external_hca_caps,
                                       const caps_map_t& caps_map)
{
    external_hca_caps->log_max_dek =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.log_max_dek);
    log_trace("Capability - log_max_dek: %d\n", external_hca_caps->log_max_dek);
}

static void store_hca_tls_1_2_aes_gcm_128_caps(adapter_hca_capabilities* external_hca_caps,
                                               const caps_map_t& caps_map)
{
    external_hca_caps->tls_1_2_aes_gcm_128 =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_TLS)->second,
                 capability.tls_cap.tls_1_2_aes_gcm_128);
    log_trace("Capability - tls_1_2_aes_gcm_128_caps: %d\n",
              external_hca_caps->tls_1_2_aes_gcm_128);
}

static void store_hca_sq_ts_format_caps(adapter_hca_capabilities* external_hca_caps,
                                        const caps_map_t& caps_map)
{
    external_hca_caps->sq_ts_format =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.sq_ts_format);
    log_trace("Capability - sq_ts_format: %d\n", external_hca_caps->sq_ts_format);
}

static void store_hca_rq_ts_format_caps(adapter_hca_capabilities* external_hca_caps,
                                        const caps_map_t& caps_map)
{
    external_hca_caps->rq_ts_format =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.rq_ts_format);
    log_trace("Capability - rq_ts_format: %d\n", external_hca_caps->rq_ts_format);
}

static void store_hca_lro_caps(adapter_hca_capabilities* external_hca_caps,
                               const caps_map_t& caps_map)
{
    caps_map_t::const_iterator iter = caps_map.find(MLX5_CAP_ETHERNET_OFFLOADS);
    void* hcattr;
    int i;

    if (iter == caps_map.end()) {
        log_fatal("Incorrect caps_map object\n");
        return;
    }

    hcattr = DEVX_ADDR_OF(query_hca_cap_out, iter->second, capability);

    external_hca_caps->lro_cap = DEVX_GET(per_protocol_networking_offload_caps, hcattr, lro_cap);
    log_trace("Capability - lro_cap: %d\n", external_hca_caps->lro_cap);

    external_hca_caps->lro_psh_flag =
        DEVX_GET(per_protocol_networking_offload_caps, hcattr, lro_psh_flag);
    log_trace("Capability - lro_psh_flag: %d\n", external_hca_caps->lro_psh_flag);

    external_hca_caps->lro_time_stamp =
        DEVX_GET(per_protocol_networking_offload_caps, hcattr, lro_time_stamp);
    log_trace("Capability - lro_time_stamp: %d\n", external_hca_caps->lro_time_stamp);

    external_hca_caps->lro_max_msg_sz_mode =
        DEVX_GET(per_protocol_networking_offload_caps, hcattr, lro_max_msg_sz_mode);
    log_trace("Capability - lro_max_msg_sz_mode: %d\n", external_hca_caps->lro_max_msg_sz_mode);

    external_hca_caps->lro_min_mss_size =
        DEVX_GET(per_protocol_networking_offload_caps, hcattr, lro_min_mss_size);
    log_trace("Capability - lro_min_mss_size: %d\n", external_hca_caps->lro_min_mss_size);

    i = 0;
    external_hca_caps->lro_timer_supported_periods[i] = (uint8_t)DEVX_GET(
        per_protocol_networking_offload_caps, hcattr, lro_timer_supported_periods[0]);
    log_trace("Capability - lro_timer_supported_periods[%d]: %d\n", i,
              external_hca_caps->lro_timer_supported_periods[i]);

    i = 1;
    external_hca_caps->lro_timer_supported_periods[i] = (uint8_t)DEVX_GET(
        per_protocol_networking_offload_caps, hcattr, lro_timer_supported_periods[1]);
    log_trace("Capability - lro_timer_supported_periods[%d]: %d\n", i,
              external_hca_caps->lro_timer_supported_periods[i]);

    i = 2;
    external_hca_caps->lro_timer_supported_periods[i] = (uint8_t)DEVX_GET(
        per_protocol_networking_offload_caps, hcattr, lro_timer_supported_periods[2]);
    log_trace("Capability - lro_timer_supported_periods[%d]: %d\n", i,
              external_hca_caps->lro_timer_supported_periods[i]);

    i = 3;
    external_hca_caps->lro_timer_supported_periods[i] = (uint8_t)DEVX_GET(
        per_protocol_networking_offload_caps, hcattr, lro_timer_supported_periods[3]);
    log_trace("Capability - lro_timer_supported_periods[%d]: %d\n", i,
              external_hca_caps->lro_timer_supported_periods[i]);
}

static const std::vector<cap_cb_fn> caps_callbacks = {
    store_hca_device_frequency_khz_caps,
    store_hca_tls_caps,
    store_hca_general_object_types_encryption_key_caps,
    store_hca_log_max_dek_caps,
    store_hca_tls_1_2_aes_gcm_128_caps,
    store_hca_cap_crypto_enable,
    store_hca_sq_ts_format_caps,
    store_hca_rq_ts_format_caps,
    store_hca_lro_caps,
};

status pd_devx::create()
{
    uint32_t in[DEVX_ST_SZ_DW(alloc_pd_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(alloc_pd_out)] = {};
    size_t outlen = sizeof(out);

    DEVX_SET(alloc_pd_in, in, opcode, MLX5_CMD_OP_ALLOC_PD);
    status ret = obj::create(in, sizeof(in), out, outlen);

    if (DPCP_OK == ret) {
        m_pd_id = DEVX_GET(alloc_pd_out, out, pd);
    }
    return ret;
}

status pd_ibv::create()
{
    dcmd::ctx* ctx = obj::get_ctx();
    if (nullptr == ctx) {
        return DPCP_ERR_NO_CONTEXT;
    }

    m_ibv_pd = (void*)ibv_alloc_pd((ibv_context*)ctx->get_context());
    if (nullptr == m_ibv_pd) {
        return DPCP_ERR_NO_MEMORY;
    }

    int err = ctx->create_ibv_pd(m_ibv_pd, m_pd_id);
    if (err) {
        return DPCP_ERR_INVALID_ID;
    }

    return DPCP_OK;
}

status td::create()
{
    uint32_t in[DEVX_ST_SZ_DW(alloc_transport_domain_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(alloc_transport_domain_out)] = {};
    size_t outlen = sizeof(out);

    DEVX_SET(alloc_transport_domain_in, in, opcode, MLX5_CMD_OP_ALLOC_TRANSPORT_DOMAIN);
    status ret = obj::create(in, sizeof(in), out, outlen);

    if (DPCP_OK == ret) {
        m_td_id = DEVX_GET(alloc_transport_domain_out, out, transport_domain);
    }
    return ret;
}

adapter::adapter(dcmd::device* dev, dcmd::ctx* ctx)
    : m_dcmd_dev(dev)
    , m_dcmd_ctx(ctx)
    , m_td(nullptr)
    , m_pd(nullptr)
    , m_uarpool(nullptr)
    , m_ibv_pd(nullptr)
    , m_pd_id(0)
    , m_td_id(0)
    , m_eqn(0)
    , m_is_caps_available(false)
    , m_caps()
    , m_external_hca_caps(nullptr)
    , m_caps_callbacks(caps_callbacks)
    , m_opened(false)
{
    m_caps.insert(std::make_pair(MLX5_CAP_GENERAL, calloc(1, DEVX_ST_SZ_DW(query_hca_cap_out))));
    m_caps.insert(std::make_pair(MLX5_CAP_TLS, calloc(1, DEVX_ST_SZ_DW(query_hca_cap_out))));
    m_caps.insert(
        std::make_pair(MLX5_CAP_ETHERNET_OFFLOADS, calloc(1, DEVX_ST_SZ_DW(query_hca_cap_out))));
    if (m_caps[MLX5_CAP_GENERAL] != nullptr && m_caps[MLX5_CAP_TLS] != nullptr &&
        m_caps[MLX5_CAP_ETHERNET_OFFLOADS] != nullptr) {
        query_hca_caps();
        set_external_hca_caps();
    }
}

status adapter::set_pd(uint32_t pdn, void* ibv_pd)
{
    if (0 == pdn || nullptr == ibv_pd) {
        return DPCP_ERR_INVALID_PARAM;
    }
    m_pd_id = pdn;
    m_ibv_pd = ibv_pd; // TODO: till DevX GPU memory is supported
    return DPCP_OK;
}

status adapter::create_ibv_pd()
{
    status ret = DPCP_OK;

    m_pd = new (std::nothrow) pd_ibv(m_dcmd_ctx);
    if (nullptr == m_pd) {
        return DPCP_ERR_NO_MEMORY;
    }

    ret = m_pd->create();
    if (DPCP_OK != ret) {
        return ret;
    }

    ret = set_pd(m_pd->get_pd_id(), ((pd_ibv*)m_pd)->get_ibv_pd());
    if (DPCP_OK != ret) {
        return ret;
    }
    return ret;
}

status adapter::create_own_pd()
{
    status ret = DPCP_OK;

    m_pd = new (std::nothrow) pd_devx(m_dcmd_ctx);
    if (nullptr == m_pd) {
        return DPCP_ERR_NO_MEMORY;
    }

    ret = m_pd->create();
    if (DPCP_OK != ret) {
        return ret;
    }

    ret = m_pd->get_id(m_pd_id);
    if (DPCP_OK != ret) {
        return ret;
    }

    return ret;
}
std::string adapter::get_name()
{
    return m_dcmd_dev->get_name();
}

status adapter::set_td(uint32_t tdn)
{
    if (0 == tdn) {
        return DPCP_ERR_INVALID_PARAM;
    }
    m_td_id = tdn;
    if (nullptr != m_td) {
        delete m_td;
        m_td = nullptr;
    }
    return DPCP_OK;
}

void* adapter::get_ibv_context()
{
    return m_dcmd_ctx->get_context();
}

status adapter::get_real_time(uint64_t& real_time)
{
    uint64_t rtc = m_dcmd_ctx->get_real_time();
    if (0 == rtc) {
        return DPCP_ERR_NO_CONTEXT;
    }
    uint32_t nanoseconds = (uint32_t)(rtc & ~(0x3 << 30)); // get the low 30 bits
    uint32_t seconds = (uint32_t)(rtc >> 32); // get the high 32 bits
    std::chrono::seconds s(seconds);

    real_time = (uint64_t)(nanoseconds + std::chrono::nanoseconds(s).count());
    return DPCP_OK;
}

status adapter::open()
{
    status ret = DPCP_OK;
    if (is_opened()) {
        return ret;
    }
    // Allocate and Create Protection Domain
    if (!get_pd()) {
        ret = create_ibv_pd();
        if (DPCP_OK != ret) {
            return ret;
        }
    }
    // Allocate and Create Transport Domain
    if (!m_td_id) {
        m_td = new (std::nothrow) td(m_dcmd_ctx);
        if (nullptr == m_td) {
            return DPCP_ERR_NO_MEMORY;
        }
        ret = m_td->create();
        if (DPCP_OK != ret) {
            return ret;
        }
        ret = m_td->get_id(m_td_id);
        if (DPCP_OK != ret) {
            return ret;
        }
    }
    // Allocate UAR pool
    if (nullptr == m_uarpool) {
        m_uarpool = new (std::nothrow) uar_collection(get_ctx());
        if (nullptr == m_uarpool) {
            return DPCP_ERR_NO_MEMORY;
        }
    }
    // Mapping device ctx to iseg for getting RTC on BF2 device
    int err = m_dcmd_ctx->hca_iseg_mapping();
    if (err) {
        log_error("hca_iseg_mapping failed ret=0x%x\n", err);
        return DPCP_ERR_NO_CONTEXT;
    }
    m_opened = true;
    return ret;
}

status adapter::create_tir(uint32_t rqn, tir*& t_)
{
    tir* tr = new (std::nothrow) tir(get_ctx());
    if (nullptr == tr) {
        return DPCP_ERR_NO_MEMORY;
    }

    status ret = tr->create(m_td_id, rqn);
    if (DPCP_OK != ret) {
        delete tr;
        return DPCP_ERR_CREATE;
    }
    t_ = tr;

    return DPCP_OK;
}

status adapter::create_tir(tir::attr& tir_attr, tir*& tir_obj)
{
    status ret = DPCP_OK;
    tir* tr = nullptr;

    tr = new (std::nothrow) tir(get_ctx());
    if (nullptr == tr) {
        return DPCP_ERR_NO_MEMORY;
    }

    ret = tr->create(tir_attr);
    if (DPCP_OK != ret) {
        delete tr;
        return DPCP_ERR_CREATE;
    }
    tir_obj = tr;

    return DPCP_OK;
}

status adapter::create_tis(const uint64_t& flags, tis*& out_tis)
{
    tis* _tis = new (std::nothrow) tis(get_ctx(), flags);
    if (_tis == nullptr) {
        return DPCP_ERR_NO_MEMORY;
    }

    uint32_t tis_pd_id = 0;
    if (flags & tis_flags::TIS_TLS_EN) {
        tis_pd_id = m_pd_id;
    }

    status ret = _tis->create(m_td_id, tis_pd_id);
    if (ret != DPCP_OK) {
        delete _tis;
        return DPCP_ERR_CREATE;
    }
    out_tis = _tis;

    return DPCP_OK;
}

status adapter::create_direct_mkey(void* address, size_t length, mkey_flags flags,
                                   direct_mkey*& dmk)
{
    dmk = new (std::nothrow) direct_mkey(this, address, length, flags);
    log_trace("dmk: %p\n", dmk);
    if (nullptr == dmk) {
        return DPCP_ERR_NO_MEMORY;
    }
    // Register UserMEMory
    status ret = dmk->reg_mem(m_ibv_pd);
    if (DPCP_OK != ret) {
        delete dmk;
        return DPCP_ERR_UMEM;
    }
    // Create MKey
    ret = dmk->create();
    if (DPCP_OK != ret) {
        delete dmk;
        return DPCP_ERR_CREATE;
    }

    return DPCP_OK;
}

status adapter::create_pattern_mkey(void* addr, mkey_flags flags, size_t stride_num, size_t bb_num,
                                    pattern_mkey_bb bb_arr[], pattern_mkey*& pmk)
{
    pmk = new (std::nothrow) pattern_mkey(this, addr, flags, stride_num, bb_num, bb_arr);
    log_trace("pattern mkey: %p\n", pmk);
    if (nullptr == pmk) {
        return DPCP_ERR_NO_MEMORY;
    }

    // Create MKey
    status ret = pmk->create();
    if (DPCP_OK != ret) {
        delete pmk;
        return DPCP_ERR_CREATE;
    }

    return DPCP_OK;
}

status reg_mem(dcmd::ctx* ctx, void* buf, size_t sz, dcmd::umem*& umem, uint32_t& mem_id)
{
    if (nullptr == ctx) {
        return DPCP_ERR_NO_CONTEXT;
    }
    if (nullptr == buf) {
        return DPCP_ERR_NO_MEMORY;
    }
    if (0 == sz) {
        return DPCP_ERR_OUT_OF_RANGE;
    }

    dcmd::umem_desc dscr = {(void*)buf, sz, 1};

    umem = ctx->create_umem(&dscr);
    if (nullptr == umem) {
        return DPCP_ERR_UMEM;
    }
    mem_id = umem->get_id();
    return DPCP_OK;
}

status adapter::create_reserved_mkey(reserved_mkey_type type, void* address, size_t length,
                                     mkey_flags flags, reserved_mkey*& rmk)
{
    rmk = new (std::nothrow) reserved_mkey(this, type, address, (uint32_t)length, flags);
    log_trace("rmk: %p\n", rmk);
    if (nullptr == rmk) {
        return DPCP_ERR_NO_MEMORY;
    }
    // Create MKey
    status ret = rmk->create();
    if (DPCP_OK != ret) {
        delete rmk;
        return DPCP_ERR_CREATE;
    }

    return DPCP_OK;
}

status adapter::create_cq(const cq_attr& attrs, cq*& out_cq)
{
    // CQ_SIZE is mandatory
    if (!attrs.cq_attr_use.test(CQ_SIZE) || !attrs.cq_sz) {
        return DPCP_ERR_INVALID_PARAM;
    }
    // EventQueue Id number is also mandatory
    if (!attrs.cq_attr_use.test(CQ_EQ_NUM)) {
        return DPCP_ERR_INVALID_PARAM;
    }

    if (nullptr == m_uarpool) {
        // Allocate UAR pool
        m_uarpool = new (std::nothrow) uar_collection(get_ctx());
        if (nullptr == m_uarpool) {
            return DPCP_ERR_NO_MEMORY;
        }
    }
    cq* cq64 = new (std::nothrow) cq(this, attrs);
    if (nullptr == cq64) {
        return DPCP_ERR_NO_MEMORY;
    }
    // Obrain UAR for new CQ
    uar cq_uar = m_uarpool->get_uar(cq64);
    if (nullptr == cq_uar) {
        return DPCP_ERR_ALLOC_UAR;
    }
    uar_t uar_p;
    status ret = m_uarpool->get_uar_page(cq_uar, uar_p);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Allocate CQ Buf
    void* cq_buf = nullptr;
    size_t cq_buf_sz = cq64->get_cq_buf_sz();
    ret = cq64->allocate_cq_buf(cq_buf, cq_buf_sz);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Register UMEM for CQ Buffer
    ret = reg_mem(get_ctx(), (void*)cq_buf, cq_buf_sz, cq64->m_cq_buf_umem, cq64->m_cq_buf_umem_id);
    if (DPCP_OK != ret) {
        return ret;
    }
    log_trace("create_cq Buf: 0x%p sz: 0x%x umem_id: %x\n", cq_buf, (uint32_t)cq_buf_sz,
              cq64->m_cq_buf_umem_id);
    //
    // Allocated DB
    uint32_t* db_rec = nullptr;
    size_t db_rec_sz = 0;
    ret = cq64->allocate_db_rec(db_rec, db_rec_sz);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Register UMEM for DoorBell record
    ret = reg_mem(get_ctx(), (void*)db_rec, db_rec_sz, cq64->m_db_rec_umem, cq64->m_db_rec_umem_id);
    if (DPCP_OK != ret) {
        return ret;
    }
    log_trace("create_cq DB: 0x%p sz: 0x%zx umem_id: %x\n", db_rec, db_rec_sz,
              cq64->m_db_rec_umem_id);

    ret = cq64->init(&uar_p);
    if (DPCP_OK == ret) {
        out_cq = cq64;
    } else {
        delete cq64->m_db_rec_umem;
        cq64->release_db_rec(db_rec);
        delete cq64->m_cq_buf_umem;
        cq64->release_cq_buf(cq_buf);
    }
    return ret;
}

status adapter::prepare_basic_rq(basic_rq& srq)
{
    // Obrain UAR for new RQ
    uar rq_uar = m_uarpool->get_uar(&srq);
    if (nullptr == rq_uar) {
        return DPCP_ERR_ALLOC_UAR;
    }
    uar_t uar_p;
    status ret = m_uarpool->get_uar_page(rq_uar, uar_p);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Allocate WQ Buf
    void* wq_buf = nullptr;
    size_t wq_buf_sz = srq.get_wq_buf_sz();
    ret = srq.allocate_wq_buf(wq_buf, wq_buf_sz);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Register UMEM for WQ Buffer
    ret = reg_mem(get_ctx(), (void*)wq_buf, wq_buf_sz, srq.m_wq_buf_umem, srq.m_wq_buf_umem_id);
    if (DPCP_OK != ret) {
        return ret;
    }
    log_trace("prepare_basic_rq Buf: 0x%p sz: 0x%x umem_id: %x\n", wq_buf, (uint32_t)wq_buf_sz,
              srq.m_wq_buf_umem_id);
    //
    // Allocated DB
    uint32_t* db_rec = nullptr;
    size_t db_rec_sz = 0;
    ret = srq.allocate_db_rec(db_rec, db_rec_sz);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Register UMEM for DoorBell record
    ret = reg_mem(get_ctx(), (void*)db_rec, db_rec_sz, srq.m_db_rec_umem, srq.m_db_rec_umem_id);
    if (DPCP_OK != ret) {
        return ret;
    }
    log_trace("prepare_basic_rq DB: 0x%p sz: 0x%zx umem_id: %x\n", db_rec, db_rec_sz,
              srq.m_db_rec_umem_id);

    return srq.init(&uar_p);
}

status adapter::create_striding_rq(rq_attr& rq_attr, size_t wqes_num, size_t wqe_sz,
                                   striding_rq*& str_rq)
{
    rq_attr.wqe_num = wqes_num;
    rq_attr.wqe_sz = wqe_sz;
    return create_striding_rq(rq_attr, str_rq);
}

status adapter::create_striding_rq(rq_attr& rq_attr, striding_rq*& str_rq)
{
    if (!m_uarpool) {
        // Allocate UAR pool
        m_uarpool = new (std::nothrow) uar_collection(get_ctx());
        if (!m_uarpool)
            return DPCP_ERR_NO_MEMORY;
    }

    std::unique_ptr<striding_rq> srq(new (std::nothrow) striding_rq(this, rq_attr));
    if (!srq)
        return DPCP_ERR_NO_MEMORY;

    status ret = prepare_basic_rq(*srq);
    if (DPCP_OK == ret)
        str_rq = srq.release();

    return ret;
}

status adapter::create_regular_rq(rq_attr& rq_attr, regular_rq*& reg_rq)
{
    if (!m_uarpool) {
        // Allocate UAR pool
        m_uarpool = new (std::nothrow) uar_collection(get_ctx());
        if (!m_uarpool)
            return DPCP_ERR_NO_MEMORY;
    }

    std::unique_ptr<regular_rq> srq(new (std::nothrow) regular_rq(this, rq_attr));
    if (!srq)
        return DPCP_ERR_NO_MEMORY;

    status ret = prepare_basic_rq(*srq);
    if (DPCP_OK == ret)
        reg_rq = srq.release();

    return ret;
}

/**
 * @brief Creates and returns dpp_rq
 *
 * @param [in]  rq_attr             RQ attributes
 * @param [in]  dpcp_dpp_protocol   How to extract the sequence number from the
 *packet.
 * @param [in]  mkey                Direct Placement mkey of the buffer
 *of 2
 * @param [out] rq              On Success created dpp_rq
 *
 * @retval      Returns DPCP_OK on success
 */
status adapter::create_dpp_rq(rq_attr& rq_attr, dpcp_dpp_protocol dpp_protocol, uint32_t mkey,
                              dpp_rq*& d_rq)
{
    dpp_rq* drq = new (std::nothrow) dpp_rq(this, rq_attr);
    if (nullptr == drq) {
        return DPCP_ERR_NO_MEMORY;
    }
    status ret = drq->init(dpp_protocol, mkey);
    if (DPCP_OK != ret) {
        delete drq;
        return ret;
    }
    d_rq = drq;

    return ret;
}

status adapter::create_flow_rule(uint16_t priority, match_params& match_criteria, flow_rule*& rule)
{
    flow_rule* fr = new (std::nothrow) flow_rule(m_dcmd_ctx, priority, match_criteria);
    if (nullptr == fr) {
        return DPCP_ERR_NO_MEMORY;
    }
    rule = fr;

    return DPCP_OK;
}

status adapter::create_comp_channel(comp_channel*& out_cch)
{
    comp_channel* cch = new (std::nothrow) comp_channel(this);
    if (nullptr == cch) {
        return DPCP_ERR_NO_MEMORY;
    }
    out_cch = cch;

    return DPCP_OK;
}

status adapter::query_eqn(uint32_t& eqn, uint32_t cpu_vector)
{
    uint32_t e;
    if (!m_dcmd_ctx->query_eqn(cpu_vector, e)) {
        eqn = m_eqn = e;
        log_trace("query_eqn: %d for cpu_vector 0x%x\n", eqn, cpu_vector);
        return DPCP_OK;
    }
    return DPCP_ERR_QUERY;
}

status adapter::create_pp_sq(sq_attr& sq_attr, pp_sq*& packet_pacing_sq)
{
    if (nullptr == m_uarpool) {
        // Allocate UAR pool
        m_uarpool = new (std::nothrow) uar_collection(get_ctx());
        if (nullptr == m_uarpool) {
            return DPCP_ERR_NO_MEMORY;
        }
    }
    pp_sq* ppsq = new (std::nothrow) pp_sq(this, sq_attr);
    if (nullptr == ppsq) {
        return DPCP_ERR_NO_MEMORY;
    }
    packet_pacing_sq = ppsq;
    // Obrain UAR for new SQ
    uar sq_uar = m_uarpool->get_uar(ppsq);
    if (nullptr == sq_uar) {
        return DPCP_ERR_ALLOC_UAR;
    }
    uar_t uar_p;
    status ret = m_uarpool->get_uar_page(sq_uar, uar_p);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Allocate WQ Buf
    void* wq_buf = nullptr;
    size_t wq_buf_sz = ppsq->get_wq_buf_sz();
    ret = ppsq->allocate_wq_buf(wq_buf, wq_buf_sz);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Register UMEM for WQ Buffer
    ret = reg_mem(get_ctx(), (void*)wq_buf, wq_buf_sz, ppsq->m_wq_buf_umem, ppsq->m_wq_buf_umem_id);
    if (DPCP_OK != ret) {
        return ret;
    }
    log_trace("create_pp_sq Buf: 0x%p sz: 0x%x umem_id: %x\n", wq_buf, (uint32_t)wq_buf_sz,
              ppsq->m_wq_buf_umem_id);
    //
    // Allocated DB
    uint32_t* db_rec = nullptr;
    size_t db_rec_sz = 0;
    ret = ppsq->allocate_db_rec(db_rec, db_rec_sz);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Register UMEM for DoorBell record
    ret = reg_mem(get_ctx(), (void*)db_rec, db_rec_sz, ppsq->m_db_rec_umem, ppsq->m_db_rec_umem_id);
    if (DPCP_OK != ret) {
        return ret;
    }
    log_trace("create_pp_sq DB: 0x%p sz: 0x%zx umem_id: %x\n", db_rec, db_rec_sz,
              ppsq->m_db_rec_umem_id);

    ret = ppsq->init(&uar_p);
    return ret;
}

status adapter::query_hca_caps()
{
    uint32_t in[DEVX_ST_SZ_DW(query_hca_cap_in)] = {};
    enum mlx5_cap_mode cap_mode = HCA_CAP_OPMOD_GET_CUR;

    enum mlx5_cap_type cap_type = MLX5_CAP_GENERAL;
    uint32_t opmod = (cap_type << 1) | cap_mode;
    DEVX_SET(query_hca_cap_in, in, opcode, MLX5_CMD_OP_QUERY_HCA_CAP);
    DEVX_SET(query_hca_cap_in, in, op_mod, opmod);
    int ret =
        m_dcmd_ctx->exec_cmd(in, sizeof(in), m_caps[cap_type], DEVX_ST_SZ_DW(query_hca_cap_out));
    if (ret) {
        log_trace("exec_cmd for HCA_CAP failed %d\n", ret);
        return DPCP_ERR_QUERY;
    }

    cap_type = MLX5_CAP_TLS;
    opmod = (cap_type << 1) | cap_mode;
    DEVX_SET(query_hca_cap_in, in, opcode, MLX5_CMD_OP_QUERY_HCA_CAP);
    DEVX_SET(query_hca_cap_in, in, op_mod, opmod);
    ret = m_dcmd_ctx->exec_cmd(in, sizeof(in), m_caps[cap_type], DEVX_ST_SZ_DW(query_hca_cap_out));
    if (ret) {
        log_trace("CAP_TLS query failed %d\n", ret);
    }

    cap_type = MLX5_CAP_ETHERNET_OFFLOADS;
    opmod = (cap_type << 1) | cap_mode;
    DEVX_SET(query_hca_cap_in, in, opcode, MLX5_CMD_OP_QUERY_HCA_CAP);
    DEVX_SET(query_hca_cap_in, in, op_mod, opmod);
    ret = m_dcmd_ctx->exec_cmd(in, sizeof(in), m_caps[cap_type], DEVX_ST_SZ_DW(query_hca_cap_out));
    if (ret) {
        log_trace("MLX5_CAP_ETHERNET_OFFLOADS query failed %d\n", ret);
    }

    return DPCP_OK;
}

void adapter::set_external_hca_caps()
{
    m_external_hca_caps = new adapter_hca_capabilities();
    for (auto& callback : m_caps_callbacks) {
        callback(m_external_hca_caps, m_caps);
    }
    m_is_caps_available = true;
}

status adapter::get_hca_caps_frequency_khz(uint32_t& freq)
{
    if (!m_is_caps_available) {
        return DPCP_ERR_QUERY;
    }

    freq = m_external_hca_caps->device_frequency_khz;
    log_trace("Adapter frequency (khz) %d\n", freq);
    return DPCP_OK;
}

status adapter::create_dek(const encryption_key_type_t type, void* const key,
                           const uint32_t size_bytes, dek*& out_dek)
{
    if (type != encryption_key_type_t::ENCRYPTION_KEY_TYPE_TLS) {
        log_trace("Only TLS encryption key type is supported");
        return DPCP_ERR_NO_SUPPORT;
    }

    dek* _dek = new (std::nothrow) dek(get_ctx());
    if (_dek == nullptr) {
        return DPCP_ERR_NO_MEMORY;
    }

    if (m_is_caps_available && !m_external_hca_caps->general_object_types_encryption_key) {
        log_trace("The adapter doesn't support the creation of general object encryption key");
        delete _dek;
        return DPCP_ERR_NO_SUPPORT;
    }

    status ret = _dek->create(m_pd_id, key, size_bytes);
    if (ret != DPCP_OK) {
        delete _dek;
        return DPCP_ERR_CREATE;
    }
    out_dek = _dek;

    return DPCP_OK;
}

adapter::~adapter()
{
    m_is_caps_available = false;

    if (m_pd) {
        delete m_pd;
        m_pd = nullptr;
    }
    if (m_td) {
        delete m_td;
        m_td = nullptr;
    }
    if (m_uarpool) {
        delete m_uarpool;
        m_uarpool = nullptr;
    }
    if (m_external_hca_caps) {
        delete m_external_hca_caps;
        m_external_hca_caps = nullptr;
    }
    delete m_dcmd_ctx;
    m_dcmd_ctx = nullptr;
}

uar_collection::uar_collection(dcmd::ctx* ctx)
    : m_mutex()
    , m_ex_uars()
    , m_sh_vc()
    , m_ctx(ctx)
    , m_shared_uar(nullptr)
{
}

uar uar_collection::get_uar(const void* p_key, uar_type type)
{
    uar u = nullptr;
    if (nullptr == p_key) {
        return u;
    }

    std::lock_guard<std::mutex> guard(m_mutex);

    if (SHARED_UAR == type) {
        if (nullptr == m_shared_uar) {
            // allocated shared UAR
            m_shared_uar = allocate();
            if (m_shared_uar) {
                m_sh_vc.push_back(p_key);
            }
        } else {
            // no need to add to shared if already shared.
            auto vit = std::find(m_sh_vc.begin(), m_sh_vc.end(), p_key);
            if (vit == m_sh_vc.end()) {
                m_sh_vc.push_back(p_key);
            }
        }
        return m_shared_uar;

    } else {
        // Exclusive UAR
        auto elem = m_ex_uars.find(p_key);

        if (elem != m_ex_uars.end()) {
            // Already allocated
            return elem->second;
        }
        // there is no UAR for this rq, find free slot

        elem = m_ex_uars.find(0);
        if (elem == m_ex_uars.end()) {
            // No free slots - allocate new uar.
            uar u_new = allocate();
            if (nullptr == u_new) {
                return nullptr;
            }
            u = add_uar(p_key, u_new);
        } else {
            // Move UAR to attached to specific rq
            u = add_uar(p_key, elem->second);
            m_ex_uars.erase(0);
        }
    }
    return u;
}

uar uar_collection::add_uar(const void* p_key, uar u)
{

    auto ret = m_ex_uars.emplace(std::make_pair(p_key, u));
    if (ret->second) {
        return ret->second;
    }
    return nullptr;
}

uar uar_collection::allocate()
{
    dcmd::uar_desc desc = {0};
    return m_ctx->create_uar(&desc);
}

void uar_collection::free(uar u)
{
    delete u;
}

status uar_collection::release_uar(const void* p_key)
{
    if (nullptr == p_key) {
        return DPCP_ERR_INVALID_PARAM;
    }

    std::lock_guard<std::mutex> guard(m_mutex);
    // check first shared
    auto vit = std::find(m_sh_vc.begin(), m_sh_vc.end(), p_key);
    if (vit != m_sh_vc.end()) {
        const auto new_end(remove(begin(m_sh_vc), end(m_sh_vc), p_key));
        m_sh_vc.erase(new_end, end(m_sh_vc));
        return DPCP_OK;
    }
    // Nothing was found, check in exclusive map
    auto it = m_ex_uars.find(p_key);
    if (it != m_ex_uars.end()) {
        // Find UAR, move it to free poll (p_key=0)
        uar u = it->second;
        m_ex_uars.erase(it);
        add_uar(0, u);
    } else {
        return DPCP_ERR_INVALID_PARAM;
    }
    return DPCP_OK;
}

status uar_collection::get_uar_page(const uar u, uar_t& uar_dsc)
{
    if (nullptr == u) {
        return DPCP_ERR_INVALID_PARAM;
    }
    uar_dsc.m_page = u->get_page();
    uar_dsc.m_bf_reg = u->get_reg();
    uar_dsc.m_page_id = u->get_id();
    return DPCP_OK;
}

uar_collection::~uar_collection()
{
    delete m_shared_uar;
    log_trace("~uar_collection shared=%zd ex=%zd\n", m_sh_vc.size(), m_ex_uars.size());
    m_ex_uars.clear();
    m_sh_vc.clear();
}

enum {
    MLX5_PP_DATA_RATE = 0x0,
    MLX5_PP_WQE_RATE = 0x1,
};

status packet_pacing::create()
{
    uint32_t pp[DEVX_ST_SZ_DW(set_pp_rate_limit_context)] = {};

    DEVX_SET(set_pp_rate_limit_context, &pp, burst_upper_bound, m_attr.burst_sz);
    DEVX_SET(set_pp_rate_limit_context, &pp, typical_packet_size, m_attr.packet_sz);
    DEVX_SET(set_pp_rate_limit_context, &pp, rate_limit, m_attr.sustained_rate);
    DEVX_SET(set_pp_rate_limit_context, &pp, rate_mode, MLX5_PP_DATA_RATE);
    m_pp_handle = devx_alloc_pp((ctx_handle)get_ctx()->get_context(), pp, sizeof(pp), 0);
    if (IS_ERR(m_pp_handle)) {
        log_error("alloc_pp failed for rate %u burst %u packet_sz %u\n", m_attr.sustained_rate,
                  m_attr.burst_sz, m_attr.sustained_rate);
        return DPCP_ERR_CREATE;
    }
    m_index = get_pp_index(m_pp_handle);
    log_trace("packet pacing index: %u for rate: %d burst: %d packet_sz: %d\n", m_index,
              m_attr.sustained_rate, m_attr.burst_sz, m_attr.packet_sz);
    return DPCP_OK;
}

} // namespace dpcp
