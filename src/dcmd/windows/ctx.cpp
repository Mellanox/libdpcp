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
