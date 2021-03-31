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

#include "utils/os.h"
#include "dpcp/internal.h"

namespace dpcp {

status pd::create()
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
    , m_caps(nullptr)
    , m_ibv_pd(nullptr)
    , m_pd_id(0)
    , m_td_id(0)
    , m_eqn(0)
    , m_is_caps_available(false)
    , m_pv_iseg(nullptr)
{
    m_caps = calloc(1, DEVX_ST_SZ_DW(query_hca_cap_out));
    if (nullptr != m_caps) {
        query_hca_caps();
    }
}

status adapter::set_pd(uint32_t pdn, void* ibv_pd)
{
    if (0 == pdn) {
        return DPCP_ERR_INVALID_PARAM;
    }
    m_pd_id = pdn;
    m_ibv_pd = ibv_pd; // TODO: till DevX GPU memory is supported
    if (nullptr != m_pd) {
        delete m_pd;
        m_pd = nullptr;
    }
    return DPCP_OK;
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

uint64_t adapter::get_real_time()
{
    if (nullptr == m_pv_iseg) {
        log_error("m_pv_iseg is not initialized");
        return 0;
    }
    // TODO: work how to save into pointer(via DEVX_ADDR_OF)
    uint64_t rtc = (uint64_t)(DEVX_GET64(initial_seg, m_pv_iseg, real_time));
    uint32_t nanoseconds = (uint32_t)(rtc & ~(0x3 << 30)); // get the low 30 bits
    uint32_t seconds = (uint32_t)(rtc >> 32); // get the high 32 bits
    std::chrono::seconds s(seconds);

    return (uint64_t)(nanoseconds + std::chrono::nanoseconds(s).count());
}

status adapter::open()
{
    status ret = DPCP_OK;
    // Allocate and Create Protection Domain
    if (!m_pd_id) {
        m_pd = new (std::nothrow) pd(m_dcmd_ctx);
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
    // Mapping divece ctx to iseg for getting RTC on BF2 device
    if (nullptr == m_pv_iseg) {
        int err = m_dcmd_ctx->hca_iseg_mapping(m_pv_iseg);
        if (err) {
            log_error("hca_iseg_mapping failed ret=0x%x\n", err);
            return DPCP_ERR_NO_CONTEXT;
        }
    }
    return ret;
}

status adapter::create_tir(uint32_t rqn, tir*& t_)
{
    tir* tr = new (std::nothrow) tir(get_ctx());
    if (nullptr == tr) {
        return DPCP_ERR_NO_MEMORY;
    }
    t_ = tr;
    // Create TIR
    status ret = tr->create(m_td_id, rqn);
    if (DPCP_OK != ret) {
        delete tr;
        return DPCP_ERR_CREATE;
    }

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
    dmk = new (std::nothrow) direct_mkey(this, address, (uint32_t)length, flags);
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

status adapter::create_striding_rq(rq_attr& rq_attr, size_t wqes_num, size_t wqe_sz,
                                   striding_rq*& str_rq)
{
    if (nullptr == m_uarpool) {
        // Allocate UAR pool
        m_uarpool = new (std::nothrow) uar_collection(get_ctx());
        if (nullptr == m_uarpool) {
            return DPCP_ERR_NO_MEMORY;
        }
    }
    striding_rq* srq = new (std::nothrow) striding_rq(this, rq_attr, wqes_num, wqe_sz);
    if (nullptr == srq) {
        return DPCP_ERR_NO_MEMORY;
    }
    str_rq = srq;
    // Obrain UAR for new RQ
    uar rq_uar = m_uarpool->get_uar(srq);
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
    size_t wq_buf_sz = srq->get_wq_buf_sz();
    ret = srq->allocate_wq_buf(wq_buf, wq_buf_sz);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Register UMEM for WQ Buffer
    ret = reg_mem(get_ctx(), (void*)wq_buf, wq_buf_sz, srq->m_wq_buf_umem, srq->m_wq_buf_umem_id);
    if (DPCP_OK != ret) {
        return ret;
    }
    log_trace("create_striderq Buf: 0x%p sz: 0x%x umem_id: %x\n", wq_buf, (uint32_t)wq_buf_sz,
              srq->m_wq_buf_umem_id);
    //
    // Allocated DB
    uint32_t* db_rec = nullptr;
    size_t db_rec_sz = 0;
    ret = srq->allocate_db_rec(db_rec, db_rec_sz);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Register UMEM for DoorBell record
    ret = reg_mem(get_ctx(), (void*)db_rec, db_rec_sz, srq->m_db_rec_umem, srq->m_db_rec_umem_id);
    if (DPCP_OK != ret) {
        return ret;
    }
    log_trace("create_striderq DB: 0x%p sz: 0x%zx umem_id: %x\n", db_rec, db_rec_sz,
              srq->m_db_rec_umem_id);

    ret = srq->init(&uar_p);
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

status adapter::query_hca_caps()
{
    uint32_t in[DEVX_ST_SZ_DW(query_hca_cap_in)] = {};
    int ret;

    DEVX_SET(query_hca_cap_in, in, opcode, MLX5_CMD_OP_QUERY_HCA_CAP);
    DEVX_SET(query_hca_cap_in, in, op_mod,
             MLX5_SET_HCA_CAP_OP_MOD_GENERAL_DEVICE | HCA_CAP_OPMOD_GET_MAX);

    ret = m_dcmd_ctx->exec_cmd(in, sizeof(in), m_caps, DEVX_ST_SZ_DW(query_hca_cap_out));
    if (ret) {
        log_trace("exec_cmd failed %d\n", ret);
        return DPCP_ERR_QUERY;
    }

    m_is_caps_available = true;
    return DPCP_OK;
}

status adapter::get_hca_caps_frequency_khz(uint32_t& freq)
{
    if (!m_is_caps_available) {
        return DPCP_ERR_QUERY;
    }

    freq = DEVX_GET(query_hca_cap_out, m_caps, capability.cmd_hca_cap.device_frequency_khz);
    log_trace("Adapter frequency (khz) %d\n", freq);
    return DPCP_OK;
}

adapter::~adapter()
{
    m_is_caps_available = false;
    free(m_caps);
    m_caps = nullptr;

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
} // namespace dpcp
