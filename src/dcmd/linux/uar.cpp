/*
 * Copyright (c) 2019-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include "dcmd/dcmd.h"
#include "utils/os.h"

using namespace dcmd;

uar::uar(ctx_handle handle, struct uar_desc* desc)
{
    struct mlx5dv_devx_uar* devx_uar;

    if (!handle || !desc) {
        throw DCMD_EINVAL;
    }

    /* Not all platforms support Combine Barrier which is required for
     * BlueFlame usage. In case the UAR cannot be created with BlueFlame
     * support - retry using No Cache mode */
    desc->flags |= MLX5_IB_UAPI_UAR_ALLOC_TYPE_BF;
    desc->flags &= ~MLX5_IB_UAPI_UAR_ALLOC_TYPE_NC;
    devx_uar = mlx5dv_devx_alloc_uar(handle, desc->flags);
    if (NULL == devx_uar) {
        desc->flags |= MLX5_IB_UAPI_UAR_ALLOC_TYPE_NC;
        desc->flags &= ~MLX5_IB_UAPI_UAR_ALLOC_TYPE_BF;
        devx_uar = mlx5dv_devx_alloc_uar(handle, desc->flags);
        if (NULL == devx_uar) {
            throw DCMD_ENOTSUP;
        }
    }
    m_handle = devx_uar;
}

uar::~uar()
{
    if (m_handle) {
        mlx5dv_devx_free_uar(m_handle);
        log_trace("~uar, handle=%p\n", m_handle);
        m_handle = nullptr;
    }
}

/* The device page id to be used */
uint32_t uar::get_id()
{
    return m_handle->page_id;
}

void* uar::get_page()
{
    return m_handle->base_addr;
}

/* Used to do doorbell write (The write address of DB/BF) */
void* uar::get_reg()
{
    return m_handle->reg_addr;
}
