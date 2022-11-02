/*
 * Copyright (c) 2020-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <atomic>
#include <memory>
#include <stdlib.h>

#include "utils/os.h"
#include "dcmd/dcmd.h"
#include "dpcp/internal.h"

namespace dpcp {

sq::sq(dcmd::ctx* ctx, sq_attr& attr)
    : obj(ctx)
    , m_attr(attr)
    , m_state(SQ_RST)
    , m_wqe_num(attr.wqe_num)
    , m_wqe_sz(attr.wqe_sz)
{
}

status sq::get_wqe_num(uint32_t& wqe_num)
{
    wqe_num = (uint32_t)m_wqe_num;
    return DPCP_OK;
}

status sq::get_wqe_sz(uint32_t& wqe_sz)
{
    wqe_sz = (uint32_t)(m_wqe_sz);
    return DPCP_OK;
}

static char* sq_state_str(sq_state state)
{
    char* str = (char*)"UNDEF";
    switch (state) {
    case SQ_RDY:
        str = (char*)"SQ_RDY";
        break;
    case SQ_RST:
        str = (char*)"SQ_RST";
        break;
    case SQ_ERR:
        str = (char*)"SQ_ERR";
        break;
    }
    return str;
}

status sq::modify_state(sq_state new_state)
{
    if (((SQ_ERR == new_state) && (SQ_RST == m_state)) ||
        ((SQ_RDY == new_state) && (SQ_ERR == m_state))) {
        return DPCP_ERR_INVALID_PARAM;
    }
    uint32_t in[DEVX_ST_SZ_DW(modify_sq_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(modify_sq_out)] = {};
    size_t outlen = sizeof(out);
    status ret = DPCP_OK;
    //
    // Set fields in modify_sq_in
    DEVX_SET(modify_sq_in, in, sq_state, m_state);
    uint32_t sqn = 0;
    ret = obj::get_id(sqn);
    if ((DPCP_OK != ret) || (0 == sqn)) {
        log_trace("modify_state failed sqn=0x%x ret=%d\n", sqn, ret);
        return DPCP_ERR_INVALID_ID;
    }
    DEVX_SET(modify_sq_in, in, sqn, sqn);
    void* p_sqc = DEVX_ADDR_OF(create_sq_in, in, ctx);
    DEVX_SET(sqc, p_sqc, state, new_state);

    DEVX_SET(modify_sq_in, in, opcode, MLX5_CMD_OP_MODIFY_SQ);
    ret = obj::modify(in, sizeof(in), out, outlen);
    // Query if state was set correctly
    if (DPCP_OK != ret) {
        return ret;
    }
    /*
     * Query to check state change
     */
    uint32_t query_in[DEVX_ST_SZ_DW(query_sq_in)] = {};
    uint32_t query_out[DEVX_ST_SZ_DW(query_sq_out)] = {};
    outlen = sizeof(query_out);
    DEVX_SET(query_sq_in, query_in, sqn, sqn);

    DEVX_SET(query_sq_in, query_in, opcode, MLX5_CMD_OP_QUERY_SQ);
    ret = obj::query(query_in, sizeof(query_in), query_out, outlen);
    if (DPCP_OK != ret) {
        return ret;
    }
    void* p_sq_ctx = DEVX_ADDR_OF(query_sq_out, query_out, sq_context);
    sq_state cur_state = (sq_state)DEVX_GET(sqc, p_sq_ctx, state);
    m_state = cur_state;
    if (new_state != cur_state) {
        uint32_t cqn = DEVX_GET(sqc, p_sq_ctx, cqn);
        log_trace("modify_state cqn: 0x%x new_state: %s cur_state: %s\n", cqn,
                  sq_state_str(new_state), sq_state_str(cur_state));
        return DPCP_ERR_MODIFY;
    }

    return DPCP_OK;
}

status sq::get_cqn(uint32_t& cqn)
{
    cqn = m_attr.cqn;
    return DPCP_OK;
}

pp_sq::pp_sq(adapter* ad, sq_attr& attr)
    : sq(ad->get_ctx(), attr)
    , m_uar(nullptr)
    , m_adapter(ad)
    , m_wq_buf(nullptr)
    , m_wq_buf_umem(nullptr)
    , m_db_rec(nullptr)
    , m_db_rec_umem(nullptr)
    , m_pp(nullptr)
    , m_wqe_num(attr.wqe_num)
    , m_wqe_sz(attr.wqe_sz)
    , m_wq_buf_umem_id(0)
    , m_db_rec_umem_id(0)
    , m_pp_idx(0)
    , m_wq_type(WQ_CYCLIC)
{
    m_wq_buf_sz_bytes = (uint32_t)(16 * m_wqe_sz * m_wqe_num);
}

status pp_sq::destroy()
{
    status ret = obj::destroy();

    if (m_uar) {
        delete m_uar;
        m_uar = nullptr;
    }
    // Deregister UMEM for WQ and DB
    if (m_wq_buf_umem) {
        delete m_wq_buf_umem;
        m_wq_buf_umem = nullptr;
    }
    if (m_db_rec_umem) {
        delete m_db_rec_umem;
        m_db_rec_umem = nullptr;
    }
    // Deallocated WQ buffer and DoorBell record
    if (m_wq_buf) {
        ::aligned_free((void*)m_wq_buf);
        m_wq_buf = nullptr;
    }
    if (m_db_rec) {
        ::aligned_free((void*)m_db_rec);
        m_db_rec = nullptr;
    }
    return ret;
}

pp_sq::~pp_sq()
{
    delete (packet_pacing*)m_pp;
    m_pp = nullptr;
    destroy();
}

status pp_sq::allocate_wq_buf(void*& wq_buf, size_t sz)
{
    // Allocate WQ buffer
    wq_buf = ::aligned_alloc(get_page_size(), sz);
    if (nullptr == wq_buf) {
        return DPCP_ERR_NO_MEMORY;
    }
    memset(wq_buf, 0, sz);
    log_trace("Allocated SQ Buf %zd -> %p\n", sz, wq_buf);
    m_wq_buf = wq_buf;
    m_wq_buf_sz_bytes = (uint32_t)sz;
    return DPCP_OK;
}

status pp_sq::allocate_db_rec(uint32_t*& db_rec, size_t& sz)
{
    // Allocate BD record
    size_t cacheline_sz = get_cacheline_size();
    sz = 64;
    db_rec = (uint32_t*)::aligned_alloc(cacheline_sz, sz);
    if (nullptr == db_rec) {
        return DPCP_ERR_NO_MEMORY;
    }
    memset(db_rec, 0, sz);
    log_trace("Allocated SQ DBRec %zd -> %p\n", sz, db_rec);
    m_db_rec = db_rec;
    return DPCP_OK;
}

status pp_sq::get_wq_buf(void*& buf_addr)
{
    if (nullptr == m_wq_buf) {
        return DPCP_ERR_NO_MEMORY;
    }
    buf_addr = m_wq_buf;
    return DPCP_OK;
}

status pp_sq::get_dbrec(uint32_t*& db_rec)
{
    if (nullptr == m_db_rec) {
        return DPCP_ERR_NO_MEMORY;
    }
    db_rec = m_db_rec;
    return DPCP_OK;
}

status pp_sq::get_uar_page(volatile void*& uar_page)
{
    if (nullptr == m_uar) {
        return DPCP_ERR_NO_MEMORY;
    }
    uar_page = m_uar->m_page;
    return DPCP_OK;
}

status pp_sq::create()
{
    uint32_t in[DEVX_ST_SZ_DW(create_sq_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(create_sq_out)] = {};
    size_t outlen = sizeof(out);
    status ret = DPCP_OK;
    //
    // Set fields in sq_ctx
    void* p_sqc = DEVX_ADDR_OF(create_sq_in, in, ctx);
    // Will use Reserved LKey
    DEVX_SET(sqc, p_sqc, rlkey, 0); // 1 = syndrome 0x13F177
    // When set RQ will flush in error posted WQEs
    DEVX_SET(sqc, p_sqc, flush_in_error_en, 0x1);
    // Will use MultiPacket Send WQE
    DEVX_SET(sqc, p_sqc, allow_multi_pkt_send_wqe, 1);
    // Flush in Error
    DEVX_SET(sqc, p_sqc, flush_in_error_en, 1);
    // Inline mode for SQ - no inline
    DEVX_SET(sqc, p_sqc, min_wqe_inline_mode, 0);
    // SQ in RESET
    DEVX_SET(sqc, p_sqc, state, m_state);
    // Indicates the timestamp format.
    // The supported format reported in HCA_CAP.sq_ts_format.
    //    0x0: FREE_RUNNING_TS
    //    0x1 : DEFAULT_TS - default that is selected by the device
    //    0x2 : REAL_TIME_TS
    DEVX_SET(sqc, p_sqc, ts_format, 1); // !!! TODO: check caps
    // ID - SQ will return it via CQE.user_index
    DEVX_SET(rqc, p_sqc, user_index, (m_attr.user_index & 0xFFFFFF));
    // CompletionQueue Number
    uint32_t id = 0;
    ret = get_cqn(id);
    if (DPCP_OK != ret) {
        return DPCP_ERR_INVALID_ID;
    }
    DEVX_SET(sqc, p_sqc, cqn, id);
    // Packet Pacing Index
    DEVX_SET(sqc, p_sqc, packet_pacing_rate_limit_index, (m_pp_idx & 0xFFFF));
    // TIS
    DEVX_SET(sqc, p_sqc, tis_lst_sz, 1);
    DEVX_SET(sqc, p_sqc, tis_num_0, (m_attr.tis_num & 0xFFFFFF));
    //
    // WQ
    void* p_wq = DEVX_ADDR_OF(sqc, p_sqc, wq);
    // CYCLIC_WQ for SQ
    DEVX_SET(wq, p_wq, wq_type, m_wq_type);
    // Protection Domain
    id = m_adapter->get_pd();
    if (0 == id) {
        return DPCP_ERR_INVALID_ID;
    }
    log_trace("createSQ: pd: %u\n", id);
    DEVX_SET(wq, p_wq, pd, (id & 0xFFFFFF));
    // UAR PageId number
    DEVX_SET(wq, p_wq, uar_page, (m_uar->m_page_id & 0xFFFFFF));
    // Offset of the DB record address inside DB umem
    DEVX_SET64(wq, p_wq, dbr_addr, 0x0);
    // Log of WQ stride size. The size of a WQ stride equals 2^log_wq_stride.
    int32_t log_wq_stride = ilog2((int)m_wqe_sz);
    DEVX_SET(wq, p_wq, log_wq_stride, log_wq_stride);
    // Log (base 2) of page size in units of 4Kbyte
    DEVX_SET(wq, p_wq, log_wq_pg_sz, 0x0);
    // Log of SQ WQEs number
    uint32_t log_wqe_num = ilog2((int)m_wqe_num);
    DEVX_SET(wq, p_wq, log_wq_sz, log_wqe_num);
    log_trace("CreateSQ: m_wqe_sz: %zd log_wq_stride: %d wqe_num_in_rq: %zd log_wqe_num: %d\n",
              m_wqe_sz, log_wq_stride, m_wqe_num, log_wqe_num);
    // Indicates that umem is used to pass db  record instead of physical address
    DEVX_SET(wq, p_wq, dbr_umem_valid, 0x1);
    // Indicates that umem is used to pass rq buffer instead of physical address (pas)
    DEVX_SET(wq, p_wq, wq_umem_valid, 0x1);
    // DB record umem Id
    DEVX_SET(wq, p_wq, dbr_umem_id, m_db_rec_umem_id);
    // WQ buffer umem Id
    DEVX_SET(wq, p_wq, wq_umem_id, m_wq_buf_umem_id);
    // Offset of RQ buffer inside WQ umem
    DEVX_SET64(wq, p_wq, wq_umem_offset, 0x0);

    // Send mailbox
    DEVX_SET(create_sq_in, in, opcode, MLX5_CMD_OP_CREATE_SQ);
    ret = obj::create(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        return ret;
    }
    ret = obj::get_id(id);
    log_trace("STR_SQ created id=0x%x ret=%d\n", id, ret);
    return ret;
}

status pp_sq::init(const uar_t* sq_uar)
{
    if (nullptr == sq_uar->m_page || 0 == sq_uar->m_page_id) {
        return DPCP_ERR_INVALID_PARAM;
    }
    m_uar = new (std::nothrow) uar_t;
    if (nullptr == m_uar) {
        return DPCP_ERR_NO_MEMORY;
    }
    *m_uar = *sq_uar;
    /* Setting Packet Pacing */
    if ((m_attr.qos_attrs_sz != 1) || (m_attr.qos_attrs == nullptr) ||
        (m_attr.qos_attrs->qos_type != QOS_TYPE::QOS_PACKET_PACING)) {
        log_error("Packet Pacing wasn't set, attrs_sz: %d\n", m_attr.qos_attrs_sz);
        return DPCP_ERR_INVALID_PARAM;
    }
    qos_packet_pacing& pp_attr = m_attr.qos_attrs->qos_attr.packet_pacing_attr;
    status ret = DPCP_OK;
    if (pp_attr.sustained_rate) {
        // Per PRM doc burst_sz = 0  is valid "and indicates packet bursts will be limited
        // to the device defauts". Packet_sz = 0 is also valid and "indicates the packet
        // size is unknown, and assumed to be MTU"
        packet_pacing* pp = new (std::nothrow) packet_pacing(get_ctx(), pp_attr);
        if (!pp) {
            log_error("Packet Pacing wasn't set for rate %d\n", pp_attr.sustained_rate);
            return DPCP_ERR_CREATE;
        }
        ret = pp->create();
        if (DPCP_OK != ret) {
            log_error("Packet Pacing wasn't set for rate %d pkt_sz %d burst %d\n",
                      pp_attr.sustained_rate, pp_attr.packet_sz, pp_attr.burst_sz);
            return ret;
        }
        m_pp_idx = pp->get_index();
        m_pp = pp;
    }
    ret = create();

    return ret;
}

status pp_sq::get_bf_reg(uint64_t*& bf_reg, size_t offset)
{
    if (m_uar) {
        bf_reg = (uint64_t*)((uint8_t*)m_uar->m_bf_reg + offset);
        return DPCP_OK;
    }
    return DPCP_ERR_NO_SUPPORT;
}

status pp_sq::modify(sq_attr& attr)
{
    /* Setting Packet Pacing */
    if ((attr.qos_attrs_sz != 1) || (attr.qos_attrs == nullptr) ||
        (attr.qos_attrs->qos_type != QOS_TYPE::QOS_PACKET_PACING)) {
        log_error("Packet Pacing wasn't set, attrs_sz: %d\n", attr.qos_attrs_sz);
        return DPCP_ERR_INVALID_PARAM;
    }
    qos_packet_pacing& pp_attr = attr.qos_attrs->qos_attr.packet_pacing_attr;
    status ret = DPCP_OK;
    packet_pacing* pp = nullptr;
    uint32_t pp_idx = 0;
    if (pp_attr.sustained_rate) {
        // Per PRM doc burst_sz = 0  is valid "and indicates packet bursts will be limited
        // to the device defauts". Packet_sz = 0 is also valid and "indicates the packet
        // size is unknown, and assumed to be MTU"
        pp = new (std::nothrow) packet_pacing(get_ctx(), pp_attr);
        if (!pp) {
            log_error("Packet Pacing wasn't set for rate %d\n", pp_attr.sustained_rate);
            return DPCP_ERR_CREATE;
        }
        ret = pp->create();
        if (DPCP_OK != ret) {
            log_error("Packet Pacing wasn't set for rate %d pkt_sz %d burst %d\n",
                      pp_attr.sustained_rate, pp_attr.packet_sz, pp_attr.burst_sz);
            delete pp;
            return ret;
        }
        pp_idx = pp->get_index() & 0xFFFF;
    } else {
        log_warn("Packet Pacing wasn't set, sustainated rate is 0 - SQ will use full bandwidth\n");
    }

    uint32_t in[DEVX_ST_SZ_DW(modify_sq_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(modify_sq_out)] = {};
    size_t outlen = sizeof(out);
    //
    // Set PP index to be modified in bitmask
    uint64_t bitmask = 0x1;
    DEVX_SET64(modify_sq_in, in, modify_bitmask, bitmask);
    uint32_t sqn = 0;
    ret = obj::get_id(sqn);
    if ((DPCP_OK != ret) || (0 == sqn)) {
        log_trace("modify_state failed sqn=0x%x ret=%d\n", sqn, ret);
        delete pp;
        return DPCP_ERR_INVALID_ID;
    }
    DEVX_SET(modify_sq_in, in, sqn, sqn);
    sq_state new_state = SQ_RDY;
    DEVX_SET(modify_sq_in, in, sq_state, new_state);
    void* p_sqc = DEVX_ADDR_OF(modify_sq_in, in, ctx);
    // There is state in ctx, it also should be set
    DEVX_SET(sqc, p_sqc, state, new_state);
    // Packet Pacing Index
    DEVX_SET(sqc, p_sqc, packet_pacing_rate_limit_index, pp_idx);
    DEVX_SET(modify_sq_in, in, opcode, MLX5_CMD_OP_MODIFY_SQ);
    ret = obj::modify(in, sizeof(in), out, outlen);
    // Query if state was set correctly
    if (DPCP_OK != ret) {
        delete pp;
        return ret;
    }
    // delete old pp
    delete (packet_pacing*)m_pp;
    m_pp = pp;
    m_pp_idx = pp ? pp->get_index() : 0;

    log_trace("New Packet Pacing was set for rate %d pkt_sz %d burst %d IDX %d\n",
              pp_attr.sustained_rate, pp_attr.packet_sz, pp_attr.burst_sz, m_pp_idx);
    return ret;
}

} // namespace dpcp
