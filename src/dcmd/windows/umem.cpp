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
#include "umem.h"

using namespace dcmd;

umem::umem(ctx_handle handle, struct umem_desc* desc)
{
    struct mlx5dv_devx_umem* devx_umem;

    if (!handle || !desc) {
        throw DCMD_EINVAL;
    }
    devx_umem = mlx5dv_devx_umem_reg(handle, desc->addr, desc->size, desc->access, &m_id);
    if (IS_ERR(devx_umem)) {
        throw DCMD_ENOTSUP;
    }
    m_handle = devx_umem;
}

umem::~umem()
{
    if (m_handle) {
        int ret = mlx5dv_devx_umem_dereg(m_handle);
        if (ret) {
            log_trace("~umem: dereg ret: %d errno: %d\n", ret, errno);
        }
        m_handle = nullptr;
    }
}

uint32_t umem::get_id()
{
    return m_id;
}
