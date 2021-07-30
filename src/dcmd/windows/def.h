/*
Copyright (C) Mellanox Technologies, Ltd. 2020. ALL RIGHTS RESERVED.

This software product is a proprietary product of Mellanox Technologies, Ltd.
(the "Company") and all right, title, and interest in and to the software
product, including all associated intellectual property rights, are and shall
remain exclusively with the Company. All rights in or to the software product
are licensed, not sold. All rights not licensed are reserved.

This software product is governed by the End User License Agreement provided
with the software product.
*/

#ifndef SRC_DCMD_WINDOWS_DCMD_DEF_H_
#define SRC_DCMD_WINDOWS_DCMD_DEF_H_
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <windows.h>
#include <winnt.h>
#include "intrin.h"

#pragma warning(push)
#pragma warning(disable : 4273)
#include <mlx5dv_win.h>
#pragma warning(pop)

typedef struct devx_device_bdf* dev_handle;
typedef devx_device_ctx* ctx_handle;
typedef struct mlx5dv_devx_obj* obj_handle;
typedef struct mlx5dv_devx_obj* cq_obj_handle;
typedef struct mlx5dv_devx_umem* umem_handle;
typedef struct mlx5dv_devx_uar* uar_handle;
typedef struct devx_obj_handle* flow_handle;

#if !defined(EVENT_CHANNEL)
typedef HANDLE event_channel;
#define EVENT_CHANNEL
#endif

#define MpByteSwapU16(x) _byteswap_ushort(x)
#define MpByteSwapU32(x) _byteswap_ulong(x)
#define MpByteSwapU64(x) _byteswap_uint64(x)

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif /* UNUSED */

inline void* aligned_alloc(size_t alignment, size_t size)
{
    return ::_aligned_malloc(size, alignment);
}

inline void aligned_free(void* mem_ptr)
{
    ::_aligned_free(mem_ptr);
    return;
}

#endif /* SRC_DCMD_WINDOWS_DEF_H_ */
