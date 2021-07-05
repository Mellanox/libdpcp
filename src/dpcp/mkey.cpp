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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <atomic>

#include "utils/os.h"
#include "dpcp/internal.h"

#ifndef IBV_ACCESS_ZERO_BASED
#define IBV_ACCESS_ZERO_BASED (1 << 5)
#endif

namespace dpcp {

static std::atomic<int> g_mkey_cnt;

/*static*/
void mkey::init_mkeys(void)
{
    // it is analog of  std::atomic_init(&direct_mkey::g_mkey_cnt,0);
    // see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64658
    //  as bug is fixed on gcc4.9 only
    g_mkey_cnt.store(0, std::memory_order_relaxed);
    log_trace("g_mkey_cnt initialized\n");
}

status mkey::get_mkey_num(int& num)
{
    num = g_mkey_cnt.load();
    return DPCP_OK;
}

direct_mkey::direct_mkey(adapter* ad, void* address, uint32_t length, mkey_flags flags)
    : mkey(ad->get_ctx())
    , m_adapter(ad)
    , m_umem(nullptr)
    , m_address(address)
    , m_ibv_mem(nullptr)
    , m_length(length)
    , m_flags(flags)
    , m_idx(0)
{
    log_trace("CTR dmk: adapter %p addr %p flags %u\n", m_adapter, m_address, (int)m_flags);
}

// Destroy is handled by kernel driver.
status direct_mkey::destroy()
{
    dcmd::ctx* ctx = m_adapter->get_ctx();
    if (nullptr == ctx) {
        return DPCP_ERR_NO_CONTEXT;
    }

    if (m_ibv_mem) {
        int err = ctx->ibv_dereg_mem_reg((struct ibv_mr*)m_ibv_mem);
        log_trace("d_mkey::dereg_mem idx 0x%x ibv_mr %p for %p status=%d, errno=%d\n", m_idx,
                  m_ibv_mem, this, err, errno);
        if (err) {
            return DPCP_ERR_NO_MEMORY;
        }
        m_ibv_mem = nullptr;
        return DPCP_OK;
    }
    status ret = obj::destroy();
    log_trace("d_mkey::destroy idx 0x%x umem %p for %p status=%d\n", m_idx, m_umem, this, ret);
    delete m_umem;
    return ret;
}

direct_mkey::~direct_mkey()
{
    destroy();
}

status direct_mkey::get_address(void*& address)
{
    address = m_address;
    if (nullptr == address) {
        return DPCP_ERR_NO_MEMORY;
    }
    return DPCP_OK;
}

status direct_mkey::get_length(uint32_t& len)
{
    len = (uint32_t)m_length;
    if (0 == len) {
        return DPCP_ERR_OUT_OF_RANGE;
    }
    return DPCP_OK;
}

status direct_mkey::get_flags(mkey_flags& flags)
{
    flags = m_flags;
    return DPCP_OK;
}

status direct_mkey::get_id(uint32_t& id)
{
    id = m_idx;
    return DPCP_OK;
}

// Register UserMEMory
status direct_mkey::reg_mem(void* verbs_pd)
{
    dcmd::ctx* ctx = m_adapter->get_ctx();
    if (nullptr == ctx) {
        return DPCP_ERR_NO_CONTEXT;
    }

    if (nullptr == m_address) {
        return DPCP_ERR_NO_MEMORY;
    }

    if (0 == m_length) {
        return DPCP_ERR_OUT_OF_RANGE;
    }

    if (verbs_pd) {
        uint32_t access = ctx->ibv_get_access_flags();
        struct ibv_mr* ibv_mem = nullptr;
        if (MKEY_ZERO_BASED == m_flags) {
            // MKEY_ZERO_BASE is broken in ibv_reg_mr() so using ibv_reg_mr_iova() instead.
            access |= IBV_ACCESS_ZERO_BASED;
            size_t page_sz = get_page_size();
            ibv_mem = ctx->ibv_reg_mem_reg_iova((ibv_pd*)verbs_pd, m_address, m_length,
                                                (uint64_t)m_address % page_sz, access);
            log_trace("direct_mkey::access %x is zero based, m_address: %p page size %zu\n", access,
                      m_address, page_sz);
        } else {
            ibv_mem = ctx->ibv_reg_mem_reg((ibv_pd*)verbs_pd, m_address, m_length, access);
        }
        if (nullptr == ibv_mem) {
            log_trace("direct_mkey::ibv_reg_mem failed: addr: %p len: %zd ibv_pd: %p ibv_mr: %p "
                      "errno: %d\n",
                      m_address, m_length, verbs_pd, ibv_mem, errno);
            return DPCP_ERR_UMEM;
        }
        m_ibv_mem = ibv_mem;
        m_idx = ((struct ibv_mr*)ibv_mem)->lkey;
        log_trace("direct_mkey::ibv_reg_mem: addr: %p len: %zd ibv_pd: %p ibv_mr: %p l_key: 0x%x\n",
                  m_address, m_length, verbs_pd, ibv_mem, m_idx);
        if (!m_idx) {
            return DPCP_ERR_NO_MEMORY;
        }
        return DPCP_OK;
    }

    return DPCP_ERR_UMEM;
}

status direct_mkey::create()
{
    if (m_ibv_mem) {
        return DPCP_OK;
    }
    uint32_t in[DEVX_ST_SZ_DW(create_mkey_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(create_mkey_out)] = {};
    size_t outlen = sizeof(out);
    uint32_t id = m_adapter->get_pd();
    if (0 == id) {
        log_error("direct_mkey::create PD num is not avalaible!\n");
        return DPCP_ERR_CREATE;
    }
    uint32_t umem_id = m_umem->get_id();
    log_trace("direct_mkey::create: addr: %p len: %zd pd: 0x%x mem_id: 0x%x\n", m_address, m_length,
              id, umem_id);
    // Set fields in mkey_entry
    void* p_mkeyc = DEVX_ADDR_OF(create_mkey_in, in, memory_key_mkey_entry);
    // 3 MSB of access mode (should be 0x1: MTT, so MSB are zeroed)
    DEVX_SET(mkc, p_mkeyc, access_mode_4_2, 0);
    // Allow local write access
    DEVX_SET(mkc, p_mkeyc, lw, 0x1);
    // Allow local read access
    DEVX_SET(mkc, p_mkeyc, lr, 0x1);
    // 2 LSB of the access mode, shall be set to 0x1 (MTT)
    DEVX_SET(mkc, p_mkeyc, access_mode_1_0, 0x1);
    // QPN this mkey is attached to.
    // When no QPN is attached must be set to 0xffffff.
    DEVX_SET(mkc, p_mkeyc, qpn, 0xffffff);
    // Obtain next Mkey counter from static value
    int mkey_cnt = g_mkey_cnt.load();
    while (!g_mkey_cnt.compare_exchange_strong(mkey_cnt, mkey_cnt + 1, std::memory_order_seq_cst) &&
           (mkey_cnt < g_mkey_cnt))
        ;
    // mkey_cnt now include incremented unique value
    //
    // SW managed ID that comes to reduce a chance were a wrong key is used due to
    // mkey
    // index reuse (SW reuses mkey)
    DEVX_SET(mkc, p_mkeyc, mkey_7_0, mkey_cnt % 0xFF);
    log_trace("create mkey_cnt %u\n", mkey_cnt);
    // Protection Domain
    DEVX_SET(mkc, p_mkeyc, pd, id);
    // Memory region address
    uint64_t addr = (uint64_t)m_address;
    if (m_flags & MKEY_ZERO_BASED) {
        // For Zero based addresses we need provide offset from page start
        addr = (uint64_t)m_address % get_page_size();
    }
    DEVX_SET64(mkc, p_mkeyc, start_addr, addr);
    // Memory region length
    DEVX_SET64(mkc, p_mkeyc, len, (uint64_t)m_length);
    // umem_id
    DEVX_SET(create_mkey_in, in, mkey_umem_valid, 1);
    DEVX_SET(create_mkey_in, in, mkey_umem_id, umem_id);

    DEVX_SET(create_mkey_in, in, opcode, MLX5_CMD_OP_CREATE_MKEY);
    status ret = obj::create(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        return ret;
    }
    // 3 Bytes of the mkey index. Mkey index equals to 24 MSBs of the 32 bits
    // mkey.
    // The rest 8 LSB of the mkey are the variant part of the mkey, which is
    // selected
    // by the SW and set via Mkey Context.mkey_7_0
    //
    m_idx = DEVX_GET(create_mkey_out, out, mkey_index) << 8;
    m_idx |= (mkey_cnt % 0xFF);
    log_trace("mkey_cnt: %d mkey_idx: 0x%x\n", mkey_cnt, m_idx);
    return ret;
}

indirect_mkey::indirect_mkey(adapter* ad)
    : mkey(ad->get_ctx())
{
}

pattern_mkey::pattern_mkey(adapter* ad, void* address, mkey_flags flags, size_t stride_num,
                           size_t bbs_num, pattern_mkey_bb* bbs)
    : indirect_mkey(ad)
    , m_adapter(ad)
    , m_bbs_arr(bbs)
    , m_mkeys_arr(nullptr)
    , m_address(address)
    , m_stride_sz(0)
    , m_stride_num(stride_num)
    , m_bbs_num(bbs_num)
    , m_flags(flags)
    , m_idx(0)
{
    log_trace("stride_num %zd bbs_num %zd\n", stride_num, bbs_num);
    for (size_t i = 0; i < m_bbs_num; i++) {
        m_stride_sz += m_bbs_arr[i].m_length;
    }
    m_mkeys_arr = new (std::nothrow) mkey*[m_bbs_num];
    if (m_mkeys_arr) {
        for (size_t i = 0; i < m_bbs_num; i++) {
            m_mkeys_arr[i] = m_bbs_arr[i].m_key;
        }
    } else {
        log_warn("memory allocation failed for m_keys_arr!\n");
    }
}

pattern_mkey::~pattern_mkey()
{
    delete[] m_mkeys_arr;
}

status pattern_mkey::get_mkeys_num(size_t& mkeys_num)
{
    mkeys_num = m_bbs_num;
    return DPCP_OK;
}

status pattern_mkey::get_mkeys_lst(mkey*& mkeys_lst)
{
    if (m_mkeys_arr) {
        mkeys_lst = (mkey*)m_mkeys_arr;
        return DPCP_OK;
    }
    return DPCP_ERR_NO_MEMORY;
}

status pattern_mkey::get_stride_sz(size_t& stride_sz)
{
    stride_sz = m_stride_sz;
    return DPCP_OK;
}

status pattern_mkey::get_stride_num(size_t& stride_num)
{
    stride_num = m_stride_num;
    return DPCP_OK;
}

status pattern_mkey::get_id(uint32_t& id)
{
    id = m_idx;
    return DPCP_OK;
}

status pattern_mkey::get_address(void*& address)
{
    address = m_address;
    if (nullptr == address) {
        return DPCP_ERR_NO_MEMORY;
    }
    return DPCP_OK;
}

status pattern_mkey::get_length(uint32_t& len_)
{
    size_t len = m_stride_sz * m_stride_num;
    if (0 == len) {
        return DPCP_ERR_OUT_OF_RANGE;
    }
    len_ = (uint32_t)len;
    return DPCP_OK;
}

status pattern_mkey::get_flags(mkey_flags& flags)
{
    flags = m_flags;
    return DPCP_OK;
}

/*
 * See PRM sec. 9.6.1, 18.3.1.1 and 30.6.1
 */
status pattern_mkey::create()
{
    status ret = DPCP_OK;
    // Max size of the Repeated Block header and entries.
    // Must be aligned to 4 octwords (octword in this case is 16Bytess).
    // If needed the last entries are padded with zeros.
    uint32_t aligned_sz = align((uint32_t)(m_bbs_num + 1), 4);
    uint32_t repeat_block_sz =
        sizeof(mlx5_wqe_umr_repeat_block_seg) + aligned_sz * sizeof(mlx5_wqe_umr_repeat_ent_seg);
    size_t inlen = DEVX_ST_SZ_DW(create_mkey_in) + repeat_block_sz / sizeof(uint32_t);
    uint32_t* in = new (std::nothrow) uint32_t[inlen];
    if (nullptr == in) {
        return DPCP_ERR_NO_MEMORY;
    }
    inlen *= sizeof(uint32_t);
    memset(in, 0, inlen);
    uint32_t out[DEVX_ST_SZ_DW(create_mkey_out)] = {};
    size_t outlen = sizeof(out);
    log_trace("create this: %p inlen:%zd outl %zd\n", this, inlen, outlen);
    log_trace("create: %p repeat_block_sz: %d aligned_sz:%d addr: %p\n", this, repeat_block_sz,
              aligned_sz, m_address);
    DEVX_SET(create_mkey_in, in, translations_octword_actual_size, aligned_sz);
    // Set fields in mkey_entry context
    //
    void* p_mkeyc = DEVX_ADDR_OF(create_mkey_in, in, memory_key_mkey_entry);
    // 3 MSB of access mode (should be 0x1: MTT, so MSB are zeroed)
    DEVX_SET(mkc, p_mkeyc, access_mode_4_2, 0x0);
    // Allow local write access
    DEVX_SET(mkc, p_mkeyc, lw, 0x1);
    // Allow local read access
    DEVX_SET(mkc, p_mkeyc, lr, 0x1);
    // 2 LSB of the access mode, shall be set to 0x2 (KLM - indirect access)
    DEVX_SET(mkc, p_mkeyc, access_mode_1_0, 0x2);
    // QPN this mkey is attached to.
    // When no QPN is attached must be set to 0xffffff.
    DEVX_SET(mkc, p_mkeyc, qpn, 0xffffff);
    // Obtain next Mkey counter from static value
    int mkey_cnt = g_mkey_cnt.load();
    while (!g_mkey_cnt.compare_exchange_strong(mkey_cnt, mkey_cnt + 1, std::memory_order_seq_cst) &&
           (mkey_cnt < g_mkey_cnt))
        ;
    // mkey_cnt now include incremented unique value
    //
    // SW managed ID that comes to reduce a chance were a wrong key is used due to
    // mkey index reuse (SW reuses mkey)
    DEVX_SET(mkc, p_mkeyc, mkey_7_0, mkey_cnt % 0xFF);
    log_trace("create mkey_cnt %u\n", mkey_cnt);
    // Protection Domain
    uint32_t id = m_adapter->get_pd();
    if (0 == id) {
        log_error("direct_mkey::create PD num is not avalaible!\n");
        delete[] in;
        return DPCP_ERR_CREATE;
    }
    DEVX_SET(mkc, p_mkeyc, pd, id);
    // Memory region address
    uint64_t addr = (uint64_t)m_address % get_page_size();
    if (0 == (m_flags & MKEY_ZERO_BASED)) {
        addr = (uint64_t)m_address;
    }
    DEVX_SET64(mkc, p_mkeyc, start_addr, addr);
    // Memory region length
    addr = m_stride_sz * m_stride_num;
    DEVX_SET64(mkc, p_mkeyc, len, (uint64_t)addr);
    // size of the Repeated Block header and entries.
    // Must be aligned to 4 octwords (octword in this case is 16 Bytes).
    // If needed the last entries are padded with zeros.
    // aligned_sz = align(m_bbs_num+1, 4);
    DEVX_SET(mkc, p_mkeyc, translations_octword_size, aligned_sz);
    //
    // Fill repeated block format
    mlx5_wqe_umr_repeat_block_seg* p_repbls =
        (mlx5_wqe_umr_repeat_block_seg*)DEVX_ADDR_OF(create_mkey_in, in, klm_pas_mtt_bsf);
    // Memory Region stride size.
    // Memory region is divided into strides of a constant size.
    p_repbls->byte_count = htobe32((uint32_t)m_stride_sz);
    // 0x400 is reserved mkey that indicates to HW / FW that this is Repeated Block Format
    p_repbls->op = htobe32(0x400);
    // Number of strides in the memory region
    p_repbls->repeat_count = htobe32((uint32_t)m_stride_num);
    // Each stride of this memory region consists from “num_entries” number of building blocks
    // that can belong to a different mkey each.
    p_repbls->num_ent = htobe16((uint16_t)m_bbs_num);
    log_trace("bytecnt/cyc %zd repeatcnt %zd num_entries %zd\n", m_stride_sz, m_stride_num,
              m_bbs_num);
    //
    // Fill repeated block entries
    mlx5_wqe_umr_repeat_ent_seg* entries = (mlx5_wqe_umr_repeat_ent_seg*)&p_repbls->entries;
    for (uint32_t i = 0; i < m_bbs_num; i++) {

        entries[i].stride = htobe16((uint16_t)m_bbs_arr[i].m_stride_sz);

        entries[i].byte_count = htobe16((uint16_t)m_bbs_arr[i].m_length);

        ret = m_bbs_arr[i].m_key->get_id(id);
        if (DPCP_OK != ret) {
            log_trace("Can't get id for MKey %p ret = %d\n", m_bbs_arr[i].m_key, ret);
            delete[] in;
            return ret;
        }
        entries[i].memkey = htobe32(id);
        void* ad = nullptr;
        ret = m_bbs_arr[i].m_key->get_address(ad);
        if (DPCP_OK != ret) {
            log_trace("Can't get address for MKey %p ret = %d\n", m_bbs_arr[i].m_key, ret);
            delete[] in;
            return ret;
        }
        entries[i].va = htobe64((uint64_t)ad);
        log_trace("id 0x%x stride_sz %zd len %zd addr %p\n", id, m_bbs_arr[i].m_stride_sz,
                  m_bbs_arr[i].m_length, ad);
    }
    // send command
    DEVX_SET(create_mkey_in, in, opcode, MLX5_CMD_OP_CREATE_MKEY);
    ret = obj::create(in, inlen, out, outlen);
    if (DPCP_OK != ret) {
        delete[] in;
        return ret;
    }
    // 3 Bytes of the mkey index. Mkey index equals to 24 MSBs of the 32 bits
    // mkey.
    // The rest 8 LSB of the mkey are the variant part of the mkey, which is
    // selected
    // by the SW and set via Mkey Context.mkey_7_0
    //
    // Send command
    m_idx = DEVX_GET(create_mkey_out, out, mkey_index) << 8;
    m_idx |= (mkey_cnt % 0xFF);
    log_trace("mkey_cnt: %d mkey_idx: 0x%x\n", mkey_cnt, m_idx);
    delete[] in;
    return ret;
}

reserved_mkey::reserved_mkey(adapter* ad, reserved_mkey_type type, void* address, uint32_t length,
                             mkey_flags flags)
    : mkey(ad->get_ctx())
    , m_address(address)
    , m_length(length)
    , m_idx(0)
    , m_type(type)
    , m_flags(flags)
{
    log_trace("RMKEY CTR ad: %p type %u flags: %u\n", ad, (int)m_type, (int)m_flags);
}

reserved_mkey::~reserved_mkey()
{
}

status reserved_mkey::create()
{
    if (MKEY_RESERVED_DUMP_AND_FILL == m_type) {
        m_idx = 0x700;
        return DPCP_OK;
    }
    return DPCP_ERR_CREATE;
}

status reserved_mkey::get_address(void*& address)
{
    if (!m_idx) {
        return DPCP_ERR_CREATE;
    }
    address = m_address;
    return DPCP_OK;
}

status reserved_mkey::get_length(uint32_t& len)
{
    if (!m_idx) {
        return DPCP_ERR_CREATE;
    }
    len = m_length;
    if (0 == len) {
        return DPCP_ERR_OUT_OF_RANGE;
    }
    return DPCP_OK;
}

status reserved_mkey::get_type(reserved_mkey_type& type)
{
    type = m_type;
    return DPCP_OK;
}

status reserved_mkey::get_flags(mkey_flags& flags)
{
    if (!m_idx) {
        return DPCP_ERR_CREATE;
    }
    flags = m_flags;
    return DPCP_OK;
}

status reserved_mkey::get_id(uint32_t& id)
{
    if (!m_idx) {
        return DPCP_ERR_CREATE;
    }
    id = m_idx;
    return DPCP_OK;
}
} // namespace dpcp
