/*
 * Copyright © 2019-2022 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Nvidia Corporation and its affiliates
 * (the "Company") and all right, title, and interest in and to the software
 * product, including all associated intellectual property rights, are and
 * shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 */

#include <string>
#include <errno.h>
#include "utils/os.h"
#include "dcmd/dcmd.h"

using namespace dcmd;

obj::obj(ctx_handle handle, struct obj_desc* desc)
{
    struct mlx5dv_devx_obj* devx_ctx;

    if (!handle || !desc) {
        throw DCMD_EINVAL;
    }

    devx_ctx = mlx5dv_devx_obj_create(handle, desc->in, desc->inlen, desc->out, desc->outlen);
    log_trace("obj(%p) handle: %p in: %p in_sz: %ld out: %p, out_sz: %ld errno=%d\n", devx_ctx,
              handle, desc->in, desc->inlen, desc->out, desc->outlen, errno);
    if (NULL == devx_ctx) {
        throw DCMD_ENOTSUP;
    }
    m_handle = devx_ctx;
}

int obj::destroy()
{
    int ret = DCMD_EOK;

    if (m_handle) {
        ret = mlx5dv_devx_obj_destroy(m_handle);
        log_trace("obj::destroy(%p) ret=%d errno=%d\n", m_handle, ret, errno);
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

    int ret = mlx5dv_devx_obj_query(m_handle, desc->in, desc->inlen, desc->out, desc->outlen);
    log_trace("obj::query(%p) in: %p in_sz: %ld out: %p, out_sz: %ld errno=%d\n", m_handle,
              desc->in, desc->inlen, desc->out, desc->outlen, errno);
    return (ret ? DCMD_EIO : DCMD_EOK);
}

int obj::modify(struct obj_desc* desc)
{

    if (!desc) {
        return DCMD_EINVAL;
    }

    int ret = mlx5dv_devx_obj_modify(m_handle, desc->in, desc->inlen, desc->out, desc->outlen);
    log_trace("obj::modify(%p) in: %p in_sz: %ld out: %p, out_sz: %ld errno=%d ret=%d\n", m_handle,
              desc->in, desc->inlen, desc->out, desc->outlen, errno, ret);
    return (ret ? DCMD_EIO : DCMD_EOK);
}
