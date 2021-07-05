/*
 Copyright (C) Mellanox Technologies, Ltd. 2001-2020. ALL RIGHTS RESERVED.

 This software product is a proprietary product of Mellanox Technologies, Ltd.
 (the "Company") and all right, title, and interest in and to the software
 product, including all associated intellectual property rights, are and shall
 remain exclusively with the Company.  All rights in or to the software product
 are licensed, not sold.  All rights not licensed are reserved.

 This software product is governed by the End User License Agreement provided
 with the software product.
*/

#include <string>

#include <utils/os.h>
#include "dcmd/dcmd.h"

using namespace dcmd;

ctx::ctx(dev_handle handle)
{
    devx_device_ctx* dvd_ctx = devx_open_device(handle);
    if (IS_ERR(dvd_ctx)) {
        dvd_ctx = nullptr;
        throw DCMD_ENOTSUP;
    }
    m_handle = dvd_ctx;
}

ctx::~ctx()
{
    if (m_handle) {
        devx_close_device(m_handle);
        m_handle = nullptr;
    }
}

void* ctx::get_context()
{
    return m_handle;
}

int ctx::exec_cmd(const void* in, size_t inlen, void* out, size_t outlen)
{
    int ret = devx_cmd(m_handle, (void*)in, inlen, out, outlen);
    return (ret ? DCMD_EIO : DCMD_EOK);
}

obj* ctx::create_obj(struct obj_desc* desc)
{
    obj* obj_ptr = nullptr;

    try {
        obj_ptr = new obj(m_handle, desc);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}

uar* ctx::create_uar(struct uar_desc* desc)
{
    uar* obj_ptr = nullptr;

    try {
        obj_ptr = new uar(m_handle, desc);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}

umem* ctx::create_umem(struct umem_desc* desc)
{
    umem* obj_ptr = nullptr;

    try {
        obj_ptr = new umem(m_handle, desc);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}

flow* ctx::create_flow(struct flow_desc* desc)
{
    flow* obj_ptr = nullptr;

    try {
        obj_ptr = new flow(m_handle, desc);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}

int ctx::query_eqn(uint32_t cpu_num, uint32_t& eqn)
{
    int ret = devx_query_eqn(m_handle, cpu_num, &eqn);
    log_trace("query_eqn: cpuNum: %x eqn: %x ret: %d\n", cpu_num, eqn, ret);
    return (ret ? DCMD_EIO : DCMD_EOK);
}

int ctx::hca_iseg_mapping()
{
    uint32_t cb_iseg;

    int ret = devx_query_hca_iseg_mapping(m_handle, &cb_iseg, &m_pv_iseg);

    if (ret) {
        log_error("devx_query_hca_iseg_mapping failed ret=0x%x\n", ret);
    }

    return (ret ? DCMD_EIO : DCMD_EOK);
}

uint64_t ctx::get_real_time()
{
    if (nullptr == m_pv_iseg) {
        log_error("m_pv_iseg is not initialized");
        return 0;
    }

    return (uint64_t)(DEVX_GET64(initial_seg, m_pv_iseg, real_time));
}

ibv_mr* ctx::ibv_reg_mem_reg_iova(struct ibv_pd* verbs_pd, void* addr, size_t length, uint64_t iova,
                                  unsigned int access)
{
    return ibv_reg_mr_iova2((ibv_pd*)verbs_pd, addr, length, iova, access);
}

ibv_mr* ctx::ibv_reg_mem_reg(struct ibv_pd* verbs_pd, void* addr, size_t length,
                             unsigned int access)
{
    return ibv_reg_mr((ibv_pd*)verbs_pd, addr, length, access);
}

int ctx::ibv_dereg_mem_reg(struct ibv_mr* ibv_mem)
{
    return ibv_dereg_mr(ibv_mem);
}

int ctx::create_ibv_pd(void* pd, uint32_t& pdn)
{
    pdn = ((ibv_pd*)pd)->handle;
    return 0;
}
