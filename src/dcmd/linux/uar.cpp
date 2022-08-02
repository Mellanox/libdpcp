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
