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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <atomic>
#include <stdlib.h>

#include "utils/os.h"
#include "dcmd/dcmd.h"
#include "dpcp/internal.h"

namespace dpcp {

struct mlx5_cqe64 {
    u8 tunneled_etc;
    u8 rsvd0;
    __be16 wqe_id;
    u8 lro_tcppsh_abort_dupack;
    u8 lro_min_ttl;
    __be16 lro_tcp_win;
    __be32 lro_ack_seq_num;
    __be32 rss_hash_result;
    u8 rss_hash_type;
    u8 ml_path;
    u8 rsvd20[2];
    __be16 check_sum;
    __be16 slid;
    __be32 flags_rqpn;
    u8 hds_ip_ext;
    u8 l4_hdr_type_etc;
    __be16 vlan_info;
    __be32 srqn; /* [31:24]: lro_num_seg, [23:0]: srqn */
    __be32 imm_inval_pkey;
    u8 rsvd40[4];
    __be32 byte_cnt;
    __be64 timestamp;
    __be32 sop_drop_qpn;
    __be16 wqe_counter;
    u8 signature;
    u8 op_own;
};

const uint32_t MAX_CQ_SZ = 1 << 22; /* in CQE number */

cq::cq(adapter* ad, const cq_attr& attrs)
    : obj(ad->get_ctx())
    , m_user_attr(attrs)
    , m_uar(nullptr)
    , m_adapter(ad)
    , m_cq_buf(nullptr)
    , m_cq_buf_umem(nullptr)
    , m_db_rec(nullptr)
    , m_arm_db(nullptr)
    , m_db_rec_umem(nullptr)
    , m_cqe_num(0)
    , m_cq_buf_umem_id(0)
    , m_db_rec_umem_id(0)
    , m_cqn(0)
    , m_eqn(0)
{
    // cq_sz is mandatory so confirmed to exist
    m_cqe_num = m_user_attr.cq_sz;
    m_cq_buf_sz_bytes = (uint32_t)(get_cqe_sz() * m_cqe_num);
}

status cq::destroy()
{
    status ret = obj::destroy();

    if (m_uar) {
        delete m_uar;
        m_uar = nullptr;
    }
    // Deregister UMEM for CQ and DB
    if (m_cq_buf_umem) {
        delete m_cq_buf_umem;
        m_cq_buf_umem = nullptr;
    }
    if (m_db_rec_umem) {
        delete m_db_rec_umem;
        m_db_rec_umem = nullptr;
    }
    // Deallocated WQ buffer and DoorBell record
    if (m_cq_buf) {
        release_cq_buf(m_cq_buf);
        m_cq_buf = nullptr;
    }
    if (m_db_rec) {
        release_db_rec(m_db_rec);
        m_db_rec = nullptr;
    }
    return ret;
}

cq::~cq()
{
    destroy();
}

status cq::allocate_cq_buf(void*& cq_buf, size_t sz)
{
    // Allocate CQ buffer
    cq_buf = ::aligned_alloc(get_page_size(), sz);
    if (nullptr == cq_buf) {
        return DPCP_ERR_NO_MEMORY;
    }
    log_trace("Allocated CQ Buf %zd -> %p\n", sz, cq_buf);
    m_cq_buf = cq_buf;
    m_cq_buf_sz_bytes = (uint32_t)sz;
    return DPCP_OK;
}

status cq::release_cq_buf(void* buf)
{
    ::aligned_free(buf);
    return DPCP_OK;
}

status cq::allocate_db_rec(uint32_t*& db_rec, size_t& sz)
{
    // Allocate BD record
    size_t cacheline_sz = get_cacheline_size();
    sz = 64;
    db_rec = (uint32_t*)::aligned_alloc(cacheline_sz, sz);
    if (nullptr == db_rec) {
        return DPCP_ERR_NO_MEMORY;
    }
    log_trace("Allocated DBRec %zd -> %p\n", sz, db_rec);
    m_db_rec = db_rec;
    return DPCP_OK;
}

status cq::release_db_rec(uint32_t* db_rec)
{
    ::aligned_free(db_rec);
    return DPCP_OK;
}

status cq::get_cq_buf(void*& buf_addr)
{
    if (nullptr == m_cq_buf) {
        return DPCP_ERR_NO_MEMORY;
    }
    buf_addr = m_cq_buf;
    return DPCP_OK;
}

status cq::get_dbrec(uint32_t*& db_rec)
{
    if (nullptr == m_db_rec) {
        return DPCP_ERR_NO_MEMORY;
    }
    db_rec = m_db_rec;
    return DPCP_OK;
}

status cq::get_uar_page(volatile void*& uar_page)
{
    if (nullptr == m_uar) {
        return DPCP_ERR_NO_MEMORY;
    }
    uar_page = m_uar->m_page;
    return DPCP_OK;
}

status cq::get_cqe_num(uint32_t& cqe_num)
{
    cqe_num = (uint32_t)m_cqe_num;
    return DPCP_OK;
}

status cq::create()
{
    uint32_t in[DEVX_ST_SZ_DW(create_cq_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(create_cq_out)] = {};
    size_t outlen = sizeof(out);

    DEVX_SET(create_cq_in, in, cq_umem_id, m_cq_buf_umem_id); // cq_umem_valid - implicit in kernel
    DEVX_SET64(create_cq_in, in, e_mtt_pointer_or_cq_umem_offset, 0LL /*cbMemOffset*/);

    // Set fields in cq_ctx
    void* cq_ctx = DEVX_ADDR_OF(create_cq_in, in, cq_context);
    // Log2 of CQ Buffer size..
    int32_t log_cq_buf_sz = ilog2((int)m_cqe_num);

    m_arm_db = m_db_rec + 1;
    // before first usage
    *m_db_rec = 0;
    *m_arm_db = 0;

    DEVX_SET(cqc, cq_ctx, log_cq_size, log_cq_buf_sz);

    DEVX_SET(cqc, cq_ctx, c_eqn, m_eqn);
    // dbr_umem_valid  - implicit in kernel
    DEVX_SET(cqc, cq_ctx, dbr_umem_id, m_db_rec_umem_id);
    DEVX_SET64(cqc, cq_ctx, dbr_addr, 0x0); // cbMemOffsetDb
    // UAR PageId
    DEVX_SET(cqc, cq_ctx, uar_page, m_uar->m_page_id);
    // Moderation attributes
    if (m_user_attr.cq_attr_use.test(CQ_MODERATION)) {
        uint32_t period = m_user_attr.moderation.cq_period;
        uint32_t max_cnt = m_user_attr.moderation.cq_max_cnt;
        DEVX_SET(cqc, cq_ctx, cq_period, period);
        DEVX_SET(cqc, cq_ctx, cq_max_count, max_cnt);
    }
    bool b_val = false;
    // CQE Collapsed flag
    if (m_user_attr.flags.test(ATTR_CQ_CQE_COLLAPSED_FLAG)) {
        b_val = true;
        DEVX_SET(cqc, cq_ctx, cc, b_val);
    }
    // Break Moderation Enable flag
    if (m_user_attr.flags.test(ATTR_CQ_BREAK_MODERATION_EN_FLAG)) {
        b_val = true;
        DEVX_SET(cqc, cq_ctx, scqe_break_moderation_en, b_val);
    }
    // Overrun Ignore flag
    if (m_user_attr.flags.test(ATTR_CQ_OVERRUN_IGNORE_FLAG)) {
        b_val = true;
        DEVX_SET(cqc, cq_ctx, oi, b_val);
    }
    // CQ compression is disabled
    DEVX_SET(cqc, cq_ctx, cqe_compression_en, false);
    // Send mailbox
    DEVX_SET(create_cq_in, in, opcode, MLX5_CMD_OP_CREATE_CQ);
    status ret = obj::create(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        return ret;
    }
    ret = obj::get_id(m_cqn);
    log_trace("CQ created cqn=0x%x ret=%d\n", m_cqn, ret);
    return ret;
}

status cq::init(const uar_t* cq_uar)
{
    if (m_user_attr.cq_sz > MAX_CQ_SZ) {
        return DPCP_ERR_INVALID_PARAM;
    }

    if (nullptr == cq_uar->m_page || 0 == cq_uar->m_page_id) {
        return DPCP_ERR_INVALID_PARAM;
    }
    // EventQueue Id number
    m_eqn = m_user_attr.eq_num;

    m_uar = new (std::nothrow) uar_t;
    if (nullptr == m_uar) {
        return DPCP_ERR_NO_MEMORY;
    }
    *m_uar = *cq_uar;
    // first round ownership bit is 1
    for (size_t i = 0; i < m_cqe_num; ++i) {
        mlx5_cqe64* cqe = (mlx5_cqe64*)m_cq_buf + i;
        cqe->op_own = 0xf1;
    }
    log_trace("use_set %s cqe num %zd eq num %d flags %s\n",
              m_user_attr.cq_attr_use.to_string().c_str(), m_cqe_num, m_eqn,
              m_user_attr.flags.to_string().c_str());

    status ret = create();

    return ret;
}
} // namespace dpcp
