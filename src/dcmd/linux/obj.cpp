/*
 * Copyright (c) 2019-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
