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
#include "uar.h"

using namespace dcmd;

uar::uar(ctx_handle handle, struct uar_desc* desc)
{
    struct mlx5dv_devx_uar* devx_uar;

    if (!handle || !desc) {
        throw DCMD_EINVAL;
    }

    devx_uar = mlx5dv_devx_alloc_uar(handle, desc->flags);
    if (IS_ERR(devx_uar)) {
        throw DCMD_ENOTSUP;
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
    return m_handle->uar_index; // page_id in linux
}

void* uar::get_page()
{
    return m_handle->uar_page; // base_addr in linux
}

/* Used to do doorbell write (the write address of DB/BF) */
void* uar::get_reg()
{
    return (void*)((uint64_t)m_handle->uar_page + 0x800); // Windows doesn't have reg_addr
}
