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

rq::rq(dcmd::ctx* ctx, rq_attr& attr)
    : obj(ctx)
    , m_attr(attr)
    , m_state(RQ_RST)
{
}

static char* rq_state_str(rq_state state)
{
    char* str = (char*)"UNDEF";
    switch (state) {
    case RQ_RDY:
        str = (char*)"RQ_RDY";
        break;
    case RQ_RST:
        str = (char*)"RQ_RST";
        break;
    case RQ_ERR:
        str = (char*)"RQ_ERR";
        break;
    }
    return str;
}

status rq::modify_state(rq_state new_state)
{
    if (((RQ_ERR == new_state) && (RQ_RST == m_state)) ||
        ((RQ_RDY == new_state) && (RQ_ERR == m_state))) {
        return DPCP_ERR_INVALID_PARAM;
    }
    uint32_t in[DEVX_ST_SZ_DW(modify_rq_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(modify_rq_out)] = {};
    size_t outlen = sizeof(out);
    status ret = DPCP_OK;
    //
    // Set fields in modify_rq_in
    DEVX_SET(modify_rq_in, in, rq_state, m_state);
    uint32_t rqn = 0;
    ret = obj::get_id(rqn);
    if ((DPCP_OK != ret) || (0 == rqn)) {
        log_trace("modify_state failed rqn=0x%x ret=%d\n", rqn, ret);
        return DPCP_ERR_INVALID_ID;
    }
    DEVX_SET(modify_rq_in, in, rqn, rqn);
    void* p_rqc = DEVX_ADDR_OF(create_rq_in, in, ctx);
    DEVX_SET(rqc, p_rqc, state, new_state);

    DEVX_SET(modify_rq_in, in, opcode, MLX5_CMD_OP_MODIFY_RQ);
    ret = obj::modify(in, sizeof(in), out, outlen);
    // Query if state was set correctly
    if (DPCP_OK != ret) {
        return ret;
    }
    uint32_t query_in[DEVX_ST_SZ_DW(query_rq_in)] = {};
    uint32_t query_out[DEVX_ST_SZ_DW(query_rq_out)] = {};
    outlen = sizeof(query_out);
    DEVX_SET(query_rq_in, query_in, rqn, rqn);

    DEVX_SET(query_rq_in, query_in, opcode, MLX5_CMD_OP_QUERY_RQ);
    ret = obj::query(query_in, sizeof(query_in), query_out, outlen);
    if (DPCP_OK != ret) {
        return ret;
    }
    void* p_rq_ctx = DEVX_ADDR_OF(query_rq_out, query_out, rq_context);
    rq_state cur_state = (rq_state)DEVX_GET(rqc, p_rq_ctx, state);
    m_state = cur_state;
    if (new_state != cur_state) {
        uint32_t cqn = DEVX_GET(rqc, p_rq_ctx, cqn);
        log_trace("modify_state cqn: 0x%x new_state: %s cur_state: %s\n", cqn,
                  rq_state_str(new_state), rq_state_str(cur_state));
        return DPCP_ERR_MODIFY;
    }

    return DPCP_OK;
}

status rq::get_hw_buff_stride_sz(size_t& buff_stride_sz)
{
    buff_stride_sz = m_attr.buf_stride_sz;
    return DPCP_OK;
}

status rq::get_hw_buff_stride_num(size_t& buff_stride_num)
{
    buff_stride_num = m_attr.buf_stride_num;
    return DPCP_OK;
}

status rq::get_cqn(uint32_t& cqn)
{
    cqn = m_attr.cqn;
    return DPCP_OK;
}

basic_rq::basic_rq(adapter* ad, rq_attr& attr)
    : rq(ad->get_ctx(), attr)
    , m_uar(nullptr)
    , m_adapter(ad)
    , m_wq_buf(nullptr)
    , m_wq_buf_umem(nullptr)
    , m_db_rec(nullptr)
    , m_db_rec_umem(nullptr)
    , m_wq_buf_umem_id(0)
    , m_db_rec_umem_id(0)
    , m_mem_type(MEMORY_RQ_INLINE)
{
    m_wq_buf_sz_bytes = (uint32_t)(16 * m_attr.wqe_sz * m_attr.wqe_num);
}

dpp_rq::dpp_rq(adapter* ad, rq_attr& attr)
    : rq(ad->get_ctx(), attr)
    , m_adapter(ad)
    , m_protocol(DPCP_DPP_NOT_INITIALIZED)
    , m_mkey(0)
{
}

status basic_rq::destroy()
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

status dpp_rq::destroy()
{
    status ret = obj::destroy();
    return ret;
}

basic_rq::~basic_rq()
{
    destroy();
}

dpp_rq::~dpp_rq()
{
    destroy();
}

status dpp_rq::get_dpp_protocol(dpcp_dpp_protocol& protocol)
{
    protocol = m_protocol;
    return DPCP_OK;
}

status basic_rq::allocate_wq_buf(void*& wq_buf, size_t sz)
{
    // Allocate WQ buffer
    // Registered memory must be aligned and multiple of page-size.
    size_t page_size = get_page_size();
    size_t mul_of_page_sz = (sz + page_size - 1) & ~(page_size - 1);
    wq_buf = ::aligned_alloc(page_size, mul_of_page_sz);
    if (nullptr == wq_buf) {
        return DPCP_ERR_NO_MEMORY;
    }
    log_trace("Allocated WQ Buf %zd -> %p\n", sz, wq_buf);
    m_wq_buf = wq_buf;
    m_wq_buf_sz_bytes = (uint32_t)sz;
    return DPCP_OK;
}

status basic_rq::allocate_db_rec(uint32_t*& db_rec, size_t& sz)
{
    // Allocate BD record
    sz = 64;
    // Latter, this memory is going to be registerd. It causes memory corruptions
    // when registering a portion of an allocated spaces that is not a multiple of PAGESIZE.
    db_rec = (uint32_t*)::aligned_alloc(get_page_size(), get_page_size());
    if (nullptr == db_rec) {
        return DPCP_ERR_NO_MEMORY;
    }
    log_trace("Allocated DBRec %zd -> %p\n", sz, db_rec);
    m_db_rec = db_rec;
    return DPCP_OK;
}

status basic_rq::get_wq_buf(void*& buf_addr)
{
    if (nullptr == m_wq_buf) {
        return DPCP_ERR_NO_MEMORY;
    }
    buf_addr = m_wq_buf;
    return DPCP_OK;
}

status basic_rq::get_dbrec(uint32_t*& db_rec)
{
    if (nullptr == m_db_rec) {
        return DPCP_ERR_NO_MEMORY;
    }
    db_rec = m_db_rec;
    return DPCP_OK;
}

status basic_rq::get_uar_page(volatile void*& uar_page)
{
    if (nullptr == m_uar) {
        return DPCP_ERR_NO_MEMORY;
    }
    uar_page = m_uar->m_page;
    return DPCP_OK;
}

status basic_rq::get_wqe_num(uint32_t& wqe_num)
{
    wqe_num = (uint32_t)m_attr.wqe_num;
    return DPCP_OK;
}

status basic_rq::get_wq_stride_sz(uint32_t& wq_stride_sz)
{
    wq_stride_sz = (uint32_t)(m_attr.wqe_sz * 16);
    return DPCP_OK;
}

striding_rq::striding_rq(adapter* ad, rq_attr& attr)
    : basic_rq(ad, attr)
{
}

status striding_rq::create()
{
    uint32_t in[DEVX_ST_SZ_DW(create_rq_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(create_rq_out)] = {};
    size_t outlen = sizeof(out);
    status ret = DPCP_OK;
    //
    // Set fields in rq_ctx
    void* p_rqc = DEVX_ADDR_OF(create_rq_in, in, ctx);
    DEVX_SET(rqc, p_rqc, rlkey, 0);
    DEVX_SET(rqc, p_rqc, delay_drop_en, 0);
    DEVX_SET(rqc, p_rqc, scatter_fcs, 0);
    // Disable VLAN stripping
    DEVX_SET(rqc, p_rqc, vlan_strip_disable, 1);
    // Inlined memory queue
    DEVX_SET(rqc, p_rqc, mem_rq_type, m_mem_type);
    // RQ in RESET
    DEVX_SET(rqc, p_rqc, state, m_state);
    // When set RQ will flush in error posted WQEs
    DEVX_SET(rqc, p_rqc, flush_in_error_en, 0x1);
    DEVX_SET(rqc, p_rqc, hairpin, 0);
    // Set RQ ts_format
    DEVX_SET(rqc, p_rqc, ts_format, m_attr.ts_format);
    // ID - RQ will return it via CQE.user_index
    DEVX_SET(rqc, p_rqc, user_index, (m_attr.user_index & 0xFFFFFF));
    // CompletionQueue Number
    uint32_t id = 0;
    ret = get_cqn(id);
    if (DPCP_OK != ret) {
        return DPCP_ERR_INVALID_ID;
    }
    DEVX_SET(rqc, p_rqc, cqn, id);
    //
    // WQ
    void* p_wq = DEVX_ADDR_OF(rqc, p_rqc, wq);
    // CYCLIC_STRIDING_WQ- Striding RQ
    DEVX_SET(wq, p_wq, wq_type, CYCLIC_STRIDING_WQ);
    // Protection Domain
    id = m_adapter->get_pd();
    if (0 == id) {
        return DPCP_ERR_INVALID_ID;
    }
    log_trace("createRQ: pd: %u\n", id);
    DEVX_SET(wq, p_wq, pd, (id & 0xFFFFFF));
    // UAR PageId number
    // DEVX_SET(wq, p_wq, uar_page, (m_uar->m_page_id & 0xFFFFFF));
    // Offset of the DB record address inside DB umem
    DEVX_SET64(wq, p_wq, dbr_addr, 0x0);
    // Log of WQ stride size. The size of a WQ stride equals 2^log_wq_stride.
    int32_t log_wq_stride = ilog2((int)m_attr.wqe_sz);
    DEVX_SET(wq, p_wq, log_wq_stride, log_wq_stride);
    // Log (base 2) of page size in units of 4Kbyte
    DEVX_SET(wq, p_wq, log_wq_pg_sz, 0x0);
    // Log of RQ WQEs number
    uint32_t log_wqe_num = ilog2((int)m_attr.wqe_num);
    DEVX_SET(wq, p_wq, log_wq_sz, log_wqe_num);
    log_trace("wqe_sz: %zd log_wq_stride: %d wqe_num_in_rq: %zd log_wqe_num: %d\n", m_attr.wqe_sz,
              log_wq_stride, m_attr.wqe_num, log_wqe_num);
    // Indicates that umem is used to pass db  record instead of physical address
    DEVX_SET(wq, p_wq, dbr_umem_valid, 0x1);
    // Indicates that umem is used to pass rq buffer instead of physical address (pas)
    DEVX_SET(wq, p_wq, wq_umem_valid, 0x1);
    //
    // Sets number of strides in each WQE
    size_t single_wqe_log_num_of_strides;
    ret = get_hw_buff_stride_num(single_wqe_log_num_of_strides);
    if (DPCP_OK != ret) {
        return DPCP_ERR_INVALID_ID;
    }
    int log_single_wqe_log_num_of_strides = ilog2((int)single_wqe_log_num_of_strides);

    if ((log_single_wqe_log_num_of_strides < MLX5_MIN_SINGLE_WQE_LOG_NUM_STRIDES) ||
        (log_single_wqe_log_num_of_strides > MLX5_MAX_SINGLE_WQE_LOG_NUM_STRIDES)) {
        return DPCP_ERR_OUT_OF_RANGE;
    }
    log_single_wqe_log_num_of_strides -= MLX5_MIN_SINGLE_WQE_LOG_NUM_STRIDES;
    DEVX_SET(wq, p_wq, single_wqe_log_num_of_strides, log_single_wqe_log_num_of_strides);
    log_trace("single_wqe_log_num_of_strides: %zd log_single_wqe_log_log_nuum_of_strides: %d\n",
              single_wqe_log_num_of_strides, log_single_wqe_log_num_of_strides);
    // Sets if HW pads 2 bytes of zeros before each packet.
    DEVX_SET(wq, p_wq, two_byte_shift_en, 0x0);
    //
    // Sets single stride size  Stride size =
    // (2^single_stride_log_num_of_-bytes)*64B
    size_t single_stride_log_num_of_bytes;
    ret = rq::get_hw_buff_stride_sz(single_stride_log_num_of_bytes);
    if (DPCP_OK != ret) {
        return DPCP_ERR_INVALID_ID;
    }
    int log_single_stride_log_num_of_bytes = ilog2((int)single_stride_log_num_of_bytes);

    if ((log_single_stride_log_num_of_bytes < MLX5_MIN_SINGLE_STRIDE_LOG_NUM_BYTES) ||
        (log_single_stride_log_num_of_bytes > MLX5_MAX_SINGLE_STRIDE_LOG_NUM_BYTES)) {
        return DPCP_ERR_OUT_OF_RANGE;
    }
    log_single_stride_log_num_of_bytes -= MLX5_MIN_SINGLE_STRIDE_LOG_NUM_BYTES;
    DEVX_SET(wq, p_wq, single_stride_log_num_of_bytes, log_single_stride_log_num_of_bytes);
    log_trace("single_stride_log_num_of_bytes: %zd log_single_stride_log_num_of_bytes: %d\n",
              single_stride_log_num_of_bytes, log_single_stride_log_num_of_bytes);
    // DB record umem Id
    DEVX_SET(wq, p_wq, dbr_umem_id, m_db_rec_umem_id);
    // WQ buffer umem Id
    DEVX_SET(wq, p_wq, wq_umem_id, m_wq_buf_umem_id);
    // Offset of RQ buffer inside WQ umem
    DEVX_SET64(wq, p_wq, wq_umem_offset, 0x0);

    // Send mailbox
    DEVX_SET(create_rq_in, in, opcode, MLX5_CMD_OP_CREATE_RQ);
    ret = obj::create(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        return ret;
    }
    ret = obj::get_id(id);
    log_trace("STR_RQ created id=0x%x ret=%d\n", id, ret);
    return ret;
}

regular_rq::regular_rq(adapter* ad, rq_attr& attr)
    : basic_rq(ad, attr)
{
}

status regular_rq::create()
{
    uint32_t in[DEVX_ST_SZ_DW(create_rq_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(create_rq_out)] = {};
    size_t outlen = sizeof(out);
    status ret = DPCP_OK;
    //
    // Set fields in rq_ctx
    void* p_rqc = DEVX_ADDR_OF(create_rq_in, in, ctx);
    DEVX_SET(rqc, p_rqc, rlkey, 0);
    DEVX_SET(rqc, p_rqc, delay_drop_en, 0);
    DEVX_SET(rqc, p_rqc, scatter_fcs, 0);
    // Disable VLAN stripping
    DEVX_SET(rqc, p_rqc, vlan_strip_disable, 1);
    // Inlined memory queue
    DEVX_SET(rqc, p_rqc, mem_rq_type, m_mem_type);
    // RQ in RESET
    DEVX_SET(rqc, p_rqc, state, m_state);
    // When set RQ will flush in error posted WQEs
    DEVX_SET(rqc, p_rqc, flush_in_error_en, 0x1);
    DEVX_SET(rqc, p_rqc, hairpin, 0);
    // Set RQ ts_format
    DEVX_SET(rqc, p_rqc, ts_format, m_attr.ts_format);
    // ID - RQ will return it via CQE.user_index
    DEVX_SET(rqc, p_rqc, user_index, (m_attr.user_index & 0xFFFFFF));
    // CompletionQueue Number
    uint32_t id = 0;
    ret = get_cqn(id);
    if (DPCP_OK != ret) {
        return DPCP_ERR_INVALID_ID;
    }
    DEVX_SET(rqc, p_rqc, cqn, id);
    //
    // WQ
    void* p_wq = DEVX_ADDR_OF(rqc, p_rqc, wq);
    // CYCLIC_STRIDING_WQ- Striding RQ
    DEVX_SET(wq, p_wq, wq_type, WQ_CYCLIC);
    // Protection Domain
    id = m_adapter->get_pd();
    if (0 == id) {
        return DPCP_ERR_INVALID_ID;
    }
    log_trace("createRQ: pd: %u\n", id);
    DEVX_SET(wq, p_wq, pd, (id & 0xFFFFFF));
    // UAR PageId number
    // DEVX_SET(wq, p_wq, uar_page, (m_uar->m_page_id & 0xFFFFFF));
    // Offset of the DB record address inside DB umem
    DEVX_SET64(wq, p_wq, dbr_addr, 0x0);
    // Log of WQ stride size. The size of a WQ stride equals 2^log_wq_stride.
    uint32_t wqe_stride_size = 0U;
    get_wq_stride_sz(wqe_stride_size);
    int32_t log_wq_stride = ilog2((int)wqe_stride_size);
    DEVX_SET(wq, p_wq, log_wq_stride, log_wq_stride);
    // Log (base 2) of page size in units of 4Kbyte
    DEVX_SET(wq, p_wq, log_wq_pg_sz, 0x0);
    // Log of RQ WQEs number
    uint32_t log_wqe_num = ilog2((int)m_attr.wqe_num);
    DEVX_SET(wq, p_wq, log_wq_sz, log_wqe_num);
    log_trace("wqe_sz: %zd log_wq_stride: %d wqe_num_in_rq: %zd log_wqe_num: %d\n", m_attr.wqe_sz,
              log_wq_stride, m_attr.wqe_num, log_wqe_num);
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
    DEVX_SET(create_rq_in, in, opcode, MLX5_CMD_OP_CREATE_RQ);
    ret = obj::create(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        return ret;
    }
    ret = obj::get_id(id);
    log_trace("REG_RQ created id=0x%x ret=%d\n", id, ret);
    return ret;
}

status dpp_rq::create()
{
    uint32_t in[DEVX_ST_SZ_DW(create_rq_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(create_rq_out)] = {};
    size_t outlen = sizeof(out);
    status ret = DPCP_OK;
    //
    // Set fields in rqt_context
    void* p_rqc = DEVX_ADDR_OF(create_rq_in, in, ctx);
    // Disable VLAN stripping
    DEVX_SET(rqc, p_rqc, vlan_strip_disable, 1);
    // DPP memory queue
    DEVX_SET(rqc, p_rqc, mem_rq_type, MEMORY_RQ_DPP);
    // RQ in RESET
    DEVX_SET(rqc, p_rqc, state, m_state);
    // Set RQ ts_format
    DEVX_SET(rqc, p_rqc, ts_format, m_attr.ts_format);
    // ID - RQ will return it via CQE.user_index
    DEVX_SET(rqc, p_rqc, user_index, (m_attr.user_index & 0xFFFFFF));
    // CompletionQueue Number
    uint32_t id = 0;
    ret = get_cqn(id);
    if (DPCP_OK != ret) {
        return DPCP_ERR_INVALID_ID;
    }
    DEVX_SET(rqc, p_rqc, cqn, id);
    // Sequence number extraction protocol
    DEVX_SET(rqc, p_rqc, dpp_wire_protocol, m_protocol);

    // DPP segment size in granularity of Bytes
    size_t buff_stride_sz = 0;
    ret = get_hw_buff_stride_sz(buff_stride_sz);
    if (DPCP_OK != ret && buff_stride_sz) {
        return DPCP_ERR_INVALID_PARAM;
    }

    DEVX_SET(rqc, p_rqc, dpp_segment_size, buff_stride_sz);
    // DPP segment size in granularity of Bytes
    size_t buff_stride_num = 0;
    ret = get_hw_buff_stride_num(buff_stride_num);
    if (DPCP_OK != ret && buff_stride_num) {
        return DPCP_ERR_INVALID_PARAM;
    }

    // Log of WQ stride size. The size of a WQ stride equals 2^log_wq_stride.
    int32_t log_buff_stride_num = ilog2((int)buff_stride_num);
    DEVX_SET(rqc, p_rqc, log_dpp_buffer_size, log_buff_stride_num);
    DEVX_SET(rqc, p_rqc, dpp_scatter_offset, m_attr.dpp_scatter_offset);
    DEVX_SET(rqc, p_rqc, dpp_mkey, m_mkey);

    // WQ
    void* p_wq = DEVX_ADDR_OF(rqc, p_rqc, wq);
    id = m_adapter->get_pd();
    if (0 == id) {
        return DPCP_ERR_INVALID_ID;
    }

    log_trace("create DPP_RQ: pd: %u mkey: 0x%x\n", id, m_mkey);
    DEVX_SET(wq, p_wq, pd, (id & 0xFFFFFF));

    // Send mailbox
    DEVX_SET(create_rq_in, in, opcode, MLX5_CMD_OP_CREATE_RQ);
    ret = obj::create(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        return ret;
    }

    ret = obj::get_id(id);
    log_trace("DPP_RQ created id=0x%x ret=%d\n", id, ret);
    return ret;
}

status basic_rq::init(const uar_t* rq_uar)
{
    if (nullptr == rq_uar->m_page || 0 == rq_uar->m_page_id) {
        return DPCP_ERR_INVALID_PARAM;
    }
    m_uar = new (std::nothrow) uar_t;
    if (nullptr == m_uar) {
        return DPCP_ERR_NO_MEMORY;
    }
    *m_uar = *rq_uar;

    status ret = create();

    return ret;
}

status dpp_rq::init(dpcp_dpp_protocol protocol, uint32_t mkey)
{
    m_protocol = protocol;
    m_mkey = mkey;
    status ret = create();
    return ret;
}

status dpp_rq::get_mkey(uint32_t& mkey)
{
    mkey = m_mkey;
    return DPCP_OK;
}
} // namespace dpcp
