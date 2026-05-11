/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <memory>
#include <cstdlib>
#include <cstring>

#include "utils/os.h"
#include "dcmd/dcmd.h"
#include "dpcp/internal.h"

namespace dpcp {

static constexpr uint32_t DPCP_QP_DEFAULT_LOG_MSG_MAX = 30;
static constexpr uint32_t DPCP_QP_DEFAULT_MIN_RNR_NAK = 1;
static constexpr uint32_t DPCP_QP_DEFAULT_RETRY_COUNT = 7;
static constexpr uint32_t DPCP_QP_DEFAULT_RNR_RETRY = 1;
static constexpr uint32_t DPCP_QP_DEFAULT_ACK_TIMEOUT = 14;
static constexpr uint32_t DPCP_QP_LOG_RQ_STRIDE_SHIFT = 4;
static constexpr uint32_t DPCP_QP_RQ_TYPE_REGULAR = 0x0;
static constexpr uint32_t DPCP_QP_RQ_TYPE_ZERO_SIZE = 0x3;
static constexpr size_t DPCP_QP_WQEBB_SZ = 64;

/* static */
size_t qp::get_wq_buf_sz(size_t rq_wqe_sz, size_t rq_wqe_num, size_t sq_wqe_sz, size_t sq_wqe_num)
{
    size_t rq_bytes = rq_wqe_sz * rq_wqe_num;
    size_t sq_bytes = sq_wqe_sz * sq_wqe_num;

    if (sq_bytes != 0) {
        rq_bytes = align<DPCP_QP_WQEBB_SZ>(rq_bytes);
    }
    return rq_bytes + sq_bytes;
}

qp::qp(adapter* ad, const qp_attr& attr)
    : obj(ad->get_ctx())
    , m_adapter(ad)
    , m_attr(attr)
    , m_state(QP_RST)
    , m_qpn(0)
    , m_wq_buf(nullptr)
    , m_wq_buf_umem_id(0)
    , m_db_rec(nullptr)
    , m_db_rec_umem_id(0)
    , m_sq_wqe_num(attr.sq_wqe_num)
    , m_sq_wqe_sz(attr.sq_wqe_sz)
    , m_rq_wqe_num(attr.rq_wqe_num)
    , m_rq_wqe_sz(attr.rq_wqe_sz)
{
}

status qp::destroy()
{
    to_reset();
    status ret = obj::destroy();

    m_qpn = 0;
    m_state = QP_RST;

    m_uar.reset();
    m_wq_buf_umem.reset();
    m_db_rec_umem.reset();
    m_owned_wq_buf.reset();
    m_wq_buf = nullptr;
    m_owned_db_rec.reset();
    m_db_rec = nullptr;
    return ret;
}

qp::~qp()
{
    destroy();
}

status qp::allocate_wq_buf(void*& wq_buf, size_t sz)
{
    m_owned_wq_buf.reset(::aligned_alloc(get_page_size(), sz));
    if (!m_owned_wq_buf) {
        log_error("Failed to allocate WQ buffer of size %zd\n", sz);
        return DPCP_ERR_NO_MEMORY;
    }
    memset(m_owned_wq_buf.get(), 0, sz);
    m_wq_buf = m_owned_wq_buf.get();
    wq_buf = m_wq_buf;
    log_trace("Allocated QP WQ Buf %zd -> %p\n", sz, m_wq_buf);
    return DPCP_OK;
}

void qp::set_wq_buf(void* buf)
{
    log_trace("Set externally allocated QP WQ Buf %p\n", buf);
    m_wq_buf = buf;
}

status qp::allocate_db_rec(qp_db_rec*& db_rec, size_t& sz)
{
    size_t cacheline_sz = get_cacheline_size();
    sz = DB_REC_SIZE;
    m_owned_db_rec.reset(static_cast<qp_db_rec*>(::aligned_alloc(cacheline_sz, sz)));
    if (!m_owned_db_rec) {
        log_error("Failed to allocate DB record of size %zd\n", sz);
        return DPCP_ERR_NO_MEMORY;
    }
    memset(m_owned_db_rec.get(), 0, sz);
    m_db_rec = m_owned_db_rec.get();
    db_rec = m_db_rec;
    log_trace("Allocated QP DBRec %zd -> %p\n", sz, m_db_rec);
    return DPCP_OK;
}

void qp::set_db_rec(qp_db_rec* db_rec)
{
    log_trace("Set externally allocated QP DBRec %p\n", db_rec);
    m_db_rec = db_rec;
}

status qp::get_wq_buf(void*& buf_addr)
{
    if (m_wq_buf == nullptr) {
        log_error("WQ buffer not allocated\n");
        return DPCP_ERR_NO_MEMORY;
    }
    buf_addr = m_wq_buf;
    return DPCP_OK;
}

status qp::get_dbrec(qp_db_rec*& db_rec)
{
    if (m_db_rec == nullptr) {
        log_error("DB record not allocated\n");
        return DPCP_ERR_NO_MEMORY;
    }
    db_rec = m_db_rec;
    return DPCP_OK;
}

status qp::get_bf_reg(uint64_t*& bf_reg, size_t offset)
{
    if (!m_uar) {
        log_error("UAR not initialized\n");
        return DPCP_ERR_NO_SUPPORT;
    }
    bf_reg = reinterpret_cast<uint64_t*>(
        const_cast<uint8_t*>(reinterpret_cast<volatile uint8_t*>(m_uar->m_bf_reg)) + offset);
    return DPCP_OK;
}

status qp::get_uar_page(volatile void*& uar_page)
{
    if (!m_uar) {
        log_error("UAR not initialized\n");
        return DPCP_ERR_NO_MEMORY;
    }
    uar_page = m_uar->m_page;
    return DPCP_OK;
}

status qp::get_state(qp_state& state)
{
    state = m_state;
    return DPCP_OK;
}

status qp::get_qpn(uint32_t& qpn)
{
    if (m_qpn == 0) {
        log_error("QP number not available\n");
        return DPCP_ERR_INVALID_ID;
    }
    qpn = m_qpn;
    return DPCP_OK;
}

status qp::get_sq_wqe_sz(uint32_t& wqe_sz)
{
    wqe_sz = static_cast<uint32_t>(m_sq_wqe_sz);
    return DPCP_OK;
}

status qp::get_sq_wqe_num(uint32_t& wqe_num)
{
    wqe_num = static_cast<uint32_t>(m_sq_wqe_num);
    return DPCP_OK;
}

status qp::get_rq_wqe_sz(uint32_t& wqe_sz)
{
    wqe_sz = static_cast<uint32_t>(m_rq_wqe_sz);
    return DPCP_OK;
}

status qp::get_rq_wqe_num(uint32_t& wqe_num)
{
    wqe_num = static_cast<uint32_t>(m_rq_wqe_num);
    return DPCP_OK;
}

status qp::get_cqn_snd(uint32_t& cqn)
{
    cqn = m_attr.cqn_snd;
    return DPCP_OK;
}

status qp::get_cqn_rcv(uint32_t& cqn)
{
    cqn = m_attr.cqn_rcv;
    return DPCP_OK;
}

status qp::on_build_create(void* /*p_in*/, void* /*p_qpc*/, void* /*p_qpc_ext*/)
{
    return DPCP_OK;
}

status qp::on_build_rst2init(void* p_qpc)
{
    DEVX_SET(qpc, p_qpc, pm_state, MLX5_QPC_PM_STATE_MIGRATED);
    DEVX_SET(qpc, p_qpc, primary_address_path.vhca_port_num, m_attr.port_num);
    DEVX_SET(qpc, p_qpc, primary_address_path.pkey_index, m_attr.pkey_index);
    return DPCP_OK;
}

status qp::on_build_init2rtr(void* p_qpc)
{
    DEVX_SET(qpc, p_qpc, mtu, m_attr.mtu);
    DEVX_SET(qpc, p_qpc, log_msg_max, DPCP_QP_DEFAULT_LOG_MSG_MAX);
    DEVX_SET(qpc, p_qpc, min_rnr_nak, DPCP_QP_DEFAULT_MIN_RNR_NAK);
    DEVX_SET(qpc, p_qpc, primary_address_path.vhca_port_num, m_attr.port_num);
    DEVX_SET(qpc, p_qpc, primary_address_path.pkey_index, m_attr.pkey_index);
    return DPCP_OK;
}

status qp::on_build_rtr2rts(void* p_qpc)
{
    DEVX_SET(qpc, p_qpc, retry_count, DPCP_QP_DEFAULT_RETRY_COUNT);
    DEVX_SET(qpc, p_qpc, rnr_retry, DPCP_QP_DEFAULT_RNR_RETRY);
    DEVX_SET(qpc, p_qpc, primary_address_path.ack_timeout, DPCP_QP_DEFAULT_ACK_TIMEOUT);
    return DPCP_OK;
}

status qp::create()
{
    uint32_t in[(DEVX_ST_SZ_BYTES(create_qp_in) + DEVX_ST_SZ_BYTES(qpc_extension_and_pas_list)) /
                sizeof(uint32_t)] = {};
    uint32_t out[DEVX_ST_SZ_DW(create_qp_out)] = {};
    size_t inlen = sizeof(in);
    size_t outlen = sizeof(out);
    status ret = DPCP_OK;

    if (m_qpn != 0) {
        log_error("create_qp: already created qpn=0x%x\n", m_qpn);
        return DPCP_ERR_INVALID_PARAM;
    }

    if (!m_uar) {
        log_error("create_qp: UAR not initialized\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    switch (m_attr.st) {
    case QPST_RC:
    case QPST_UC:
    case QPST_UD:
    case QPST_DCI:
        break;
    default:
        log_error("create_qp: invalid service type 0x%x\n", m_attr.st);
        return DPCP_ERR_INVALID_PARAM;
    }

    adapter_hca_capabilities caps;
    if (m_adapter->get_hca_capabilities(caps) != DPCP_OK) {
        log_error("create_qp: HCA capabilities cache not populated\n");
        return DPCP_ERR_NO_CONTEXT;
    }
    if (m_attr.ts_format == QP_TS_FREE_RUNNING &&
        caps.qp_ts_format == MLX5_ROCE_CAP_QP_TS_FORMAT_REAL_TIME_TS) {
        log_error("create_qp: QP_TS_FREE_RUNNING not supported (qp_ts_format=%u)\n",
                  caps.qp_ts_format);
        return DPCP_ERR_NO_SUPPORT;
    }
    if (m_attr.ts_format == QP_TS_REAL_TIME &&
        caps.qp_ts_format == MLX5_ROCE_CAP_QP_TS_FORMAT_FREE_RUNNING_TS) {
        log_error("create_qp: QP_TS_REAL_TIME not supported (qp_ts_format=%u)\n",
                  caps.qp_ts_format);
        return DPCP_ERR_NO_SUPPORT;
    }

    uint32_t pd = m_adapter->get_pd();
    if (pd == 0) {
        log_error("create_qp: invalid PD\n");
        return DPCP_ERR_INVALID_ID;
    }

    void* p_qpc = DEVX_ADDR_OF(create_qp_in, in, qpc_data);
    void* p_qpc_ext = DEVX_ADDR_OF(create_qp_in, in, qpc_pas_list.qpc_data_extension);

    DEVX_SET(qpc, p_qpc, st, m_attr.st);
    DEVX_SET(qpc, p_qpc, pd, pd);
    DEVX_SET(qpc, p_qpc, uar_page, m_uar->m_page_id & 0xFFFFFF);
    DEVX_SET(qpc, p_qpc, user_index, m_attr.user_index & 0xFFFFFF);
    DEVX_SET(qpc, p_qpc, ts_format, m_attr.ts_format);

    if (m_sq_wqe_num > 0 && !is_pow2(m_sq_wqe_num)) {
        log_error("create_qp: SQ WQE num must be power-of-two, got %zu\n", m_sq_wqe_num);
        return DPCP_ERR_INVALID_PARAM;
    }
    if (m_rq_wqe_num > 0) {
        if (!is_pow2(m_rq_wqe_num) || !is_pow2(m_rq_wqe_sz) ||
            m_rq_wqe_sz < (1u << DPCP_QP_LOG_RQ_STRIDE_SHIFT)) {
            log_error("create_qp: invalid RQ geometry wqe_num=%zu wqe_sz=%zu\n", m_rq_wqe_num,
                      m_rq_wqe_sz);
            return DPCP_ERR_INVALID_PARAM;
        }
    }

    if (m_sq_wqe_num > 0) {
        DEVX_SET(qpc, p_qpc, cqn_snd, m_attr.cqn_snd);
        DEVX_SET(qpc, p_qpc, log_sq_size, ilog2(static_cast<int>(m_sq_wqe_num)));
    } else {
        DEVX_SET(qpc, p_qpc, no_sq, 1);
    }

    if (m_rq_wqe_num > 0) {
        DEVX_SET(qpc, p_qpc, rq_type, DPCP_QP_RQ_TYPE_REGULAR);
        DEVX_SET(qpc, p_qpc, cqn_rcv, m_attr.cqn_rcv);
        DEVX_SET(qpc, p_qpc, log_rq_size, ilog2(static_cast<int>(m_rq_wqe_num)));
        DEVX_SET(qpc, p_qpc, log_rq_stride,
                 ilog2(static_cast<int>(m_rq_wqe_sz)) - DPCP_QP_LOG_RQ_STRIDE_SHIFT);
    } else {
        DEVX_SET(qpc, p_qpc, rq_type, DPCP_QP_RQ_TYPE_ZERO_SIZE);
    }

    DEVX_SET(qpc, p_qpc, dbr_umem_id, m_db_rec_umem_id);
    DEVX_SET(qpc, p_qpc, dbr_umem_valid, 1);

    DEVX_SET(create_qp_in, in, wq_umem_id, m_wq_buf_umem_id);
    DEVX_SET(create_qp_in, in, wq_umem_valid, 1);

    ret = on_build_create(in, p_qpc, p_qpc_ext);
    if (ret != DPCP_OK) {
        log_error("Failed to build create QP\n");
        return ret;
    }

    DEVX_SET(create_qp_in, in, opcode, MLX5_CMD_OP_CREATE_QP);

    ret = obj::create(in, inlen, out, outlen);
    if (ret != DPCP_OK) {
        log_error("Failed to create QP\n");
        return ret;
    }
    ret = obj::get_id(m_qpn);
    log_trace("QP created qpn=0x%x ret=%d\n", m_qpn, ret);
    return ret;
}

status qp::init(const uar_t* qp_uar)
{
    if (m_uar) {
        log_error("init: already initialized\n");
        return DPCP_ERR_INVALID_PARAM;
    }
    if (qp_uar == nullptr || qp_uar->m_page == nullptr || qp_uar->m_page_id == 0) {
        log_error("Invalid UAR provided for QP creation\n");
        return DPCP_ERR_INVALID_PARAM;
    }
    m_uar.reset(new (std::nothrow) uar_t);
    if (!m_uar) {
        log_error("Failed to allocate memory for UAR\n");
        return DPCP_ERR_NO_MEMORY;
    }
    *m_uar = *qp_uar;
    status ret = create();
    if (ret != DPCP_OK) {
        log_error("Failed to create QP\n");
        m_uar.reset();
    }
    return ret;
}

status qp::modify_state(qp_state new_state)
{
    switch (new_state) {
    case QP_RST:
        return to_reset();

    case QP_INIT: {
        if (m_state != QP_RST) {
            log_error("Invalid state transition from %d to QP_INIT\n", m_state);
            return DPCP_ERR_INVALID_PARAM;
        }
        uint32_t in[DEVX_ST_SZ_DW(rst2init_qp_in)] = {};
        uint32_t out[DEVX_ST_SZ_DW(rst2init_qp_out)] = {};
        size_t inlen = sizeof(in);
        size_t outlen = sizeof(out);

        DEVX_SET(rst2init_qp_in, in, opcode, MLX5_CMD_OP_RST2INIT_QP);
        DEVX_SET(rst2init_qp_in, in, qpn, m_qpn);
        void* p_qpc = DEVX_ADDR_OF(rst2init_qp_in, in, qpc_data);
        status ret = on_build_rst2init(p_qpc);
        if (ret != DPCP_OK) {
            log_error("Failed to build RST2INIT QP\n");
            return ret;
        }
        ret = obj::modify(in, inlen, out, outlen);
        if (ret != DPCP_OK) {
            log_error("RST2INIT failed for qpn=0x%x\n", m_qpn);
            return ret;
        }
        m_state = QP_INIT;
        return DPCP_OK;
    }

    case QP_RTR: {
        if (m_state != QP_INIT) {
            log_error("Invalid state transition from %d to QP_RTR\n", m_state);
            return DPCP_ERR_INVALID_PARAM;
        }
        uint32_t in[DEVX_ST_SZ_DW(init2rtr_qp_in)] = {};
        uint32_t out[DEVX_ST_SZ_DW(init2rtr_qp_out)] = {};
        size_t inlen = sizeof(in);
        size_t outlen = sizeof(out);
        DEVX_SET(init2rtr_qp_in, in, opcode, MLX5_CMD_OP_INIT2RTR_QP);
        DEVX_SET(init2rtr_qp_in, in, qpn, m_qpn);
        void* p_qpc = DEVX_ADDR_OF(init2rtr_qp_in, in, qpc_data);
        status ret = on_build_init2rtr(p_qpc);
        if (ret != DPCP_OK) {
            log_error("Failed to build INIT2RTR QP\n");
            return ret;
        }
        ret = obj::modify(in, inlen, out, outlen);
        if (ret != DPCP_OK) {
            log_error("INIT2RTR failed for qpn=0x%x\n", m_qpn);
            return ret;
        }
        m_state = QP_RTR;
        return DPCP_OK;
    }

    case QP_RTS: {
        if (m_state != QP_RTR) {
            log_error("Invalid state transition from %d to QP_RTS\n", m_state);
            return DPCP_ERR_INVALID_PARAM;
        }
        uint32_t in[DEVX_ST_SZ_DW(rtr2rts_qp_in)] = {};
        uint32_t out[DEVX_ST_SZ_DW(rtr2rts_qp_out)] = {};
        size_t inlen = sizeof(in);
        size_t outlen = sizeof(out);
        DEVX_SET(rtr2rts_qp_in, in, opcode, MLX5_CMD_OP_RTR2RTS_QP);
        DEVX_SET(rtr2rts_qp_in, in, qpn, m_qpn);
        void* p_qpc = DEVX_ADDR_OF(rtr2rts_qp_in, in, qpc_data);
        status ret = on_build_rtr2rts(p_qpc);
        if (ret != DPCP_OK) {
            log_error("Failed to build RTR2RTS QP\n");
            return ret;
        }
        ret = obj::modify(in, inlen, out, outlen);
        if (ret != DPCP_OK) {
            log_error("RTR2RTS failed for qpn=0x%x\n", m_qpn);
            return ret;
        }
        m_state = QP_RTS;
        return DPCP_OK;
    }

    default:
        return DPCP_ERR_NO_SUPPORT;
    }
}

status qp::query_state(qp_state& cur_state)
{
    uint32_t in[DEVX_ST_SZ_DW(query_qp_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(query_qp_out)] = {};
    size_t inlen = sizeof(in);
    size_t outlen = sizeof(out);

    DEVX_SET(query_qp_in, in, opcode, MLX5_CMD_OP_QUERY_QP);
    DEVX_SET(query_qp_in, in, qpn, m_qpn);

    status ret = obj::query(in, inlen, out, outlen);
    if (ret != DPCP_OK) {
        log_error("Failed to query QP state for qpn=0x%x\n", m_qpn);
        return ret;
    }
    void* p_qpc = DEVX_ADDR_OF(query_qp_out, out, qpc_data);
    cur_state = static_cast<qp_state>(DEVX_GET(qpc, p_qpc, state));
    m_state = cur_state;
    return DPCP_OK;
}

status qp::to_reset()
{
    if (m_qpn == 0) {
        return DPCP_OK;
    }

    uint32_t in[DEVX_ST_SZ_DW(qp_2rst_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(qp_2rst_out)] = {};
    size_t inlen = sizeof(in);
    size_t outlen = sizeof(out);

    DEVX_SET(qp_2rst_in, in, opcode, MLX5_CMD_OP_2RST_QP);
    DEVX_SET(qp_2rst_in, in, qpn, m_qpn);

    status ret = obj::modify(in, inlen, out, outlen);
    if (ret != DPCP_OK) {
        log_trace("2RST_QP best-effort failure qpn=0x%x ret=%d\n", m_qpn, ret);
    }
    m_state = QP_RST;
    return DPCP_OK;
}

status qp::transition_to_rts()
{
    status ret = modify_state(QP_INIT);
    if (ret != DPCP_OK) {
        log_error("Failed to transition QP to INIT state\n");
        return ret;
    }
    ret = modify_state(QP_RTR);
    if (ret != DPCP_OK) {
        log_error("Failed to transition QP to RTR state\n");
        return ret;
    }
    ret = modify_state(QP_RTS);
    if (ret != DPCP_OK) {
        log_error("Failed to transition QP to RTS state\n");
        return ret;
    }
    return DPCP_OK;
}

} // namespace dpcp
