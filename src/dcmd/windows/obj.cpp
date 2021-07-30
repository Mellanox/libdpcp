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

#include "stdafx.h"
#include "obj.h"

using namespace dcmd;

obj::obj(ctx_handle handle, struct obj_desc* desc)
    : base_obj()
    , m_ctx_handle(handle)
    , m_handle(nullptr)
{
    if (!handle || !desc) {
        throw DCMD_EINVAL;
    }

    struct mlx5dv_devx_obj* devx_ctx =
        mlx5dv_devx_obj_create(handle, (void*)desc->in, desc->inlen, desc->out, desc->outlen);
    log_trace("errno: %d handle: %p devx_ctx: %p in: %p in_sz: %lld out: %p, out_sz: %lld\n", errno,
              handle, devx_ctx, desc->in, desc->inlen, desc->out, desc->outlen);
    if (IS_ERR(devx_ctx)) {
        throw DCMD_ENOTSUP;
    }
    m_handle = devx_ctx;
}

int obj::destroy()
{
    int ret = DCMD_EOK;
    if (m_handle) {
        ret = mlx5dv_devx_obj_destroy(m_handle);
        log_trace("obj::destroyed %p ret=%d errno=%d\n", m_handle, ret, errno);
        m_handle = nullptr;
    }
    return ret;
}

obj::~obj()
{
    destroy();
}

int obj::query(struct obj_desc* desc)
{
    if (!desc) {
        return DCMD_EINVAL;
    }
    int ret =
        mlx5dv_devx_obj_query(m_ctx_handle, (void*)desc->in, desc->inlen, desc->out, desc->outlen);
    log_trace("obj::query errno: %d in: %p in_sz: %lld out: %p, out_sz: %lld\n", errno, desc->in,
              desc->inlen, desc->out, desc->outlen);
    return (ret ? DCMD_EIO : DCMD_EOK);
}

int obj::modify(struct obj_desc* desc)
{

    if (!desc) {
        return DCMD_EINVAL;
    }
    int ret =
        mlx5dv_devx_obj_modify(m_ctx_handle, (void*)desc->in, desc->inlen, desc->out, desc->outlen);
    return (ret ? DCMD_EIO : DCMD_EOK);
}
