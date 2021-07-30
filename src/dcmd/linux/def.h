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
#ifndef SRC_DCMD_LINUX_DEF_H_
#define SRC_DCMD_LINUX_DEF_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_DEVX)

#include <infiniband/verbs.h>
#include <infiniband/mlx5dv.h>

typedef struct ibv_device* dev_handle;
typedef struct ibv_context* ctx_handle;
typedef struct mlx5dv_devx_obj* obj_handle;
typedef struct mlx5dv_devx_umem* umem_handle;
typedef struct mlx5dv_devx_uar* uar_handle;
typedef struct ibv_flow* flow_handle;
typedef struct ibv_cq* cq_obj_handle;
#if !defined(EVENT_CHANNEL)
typedef struct {
    int fd;
} event_channel;
#define EVENT_CHANNEL
#endif
typedef struct ibv_comp_channel comp_channel;
typedef ibv_event_type devx_event_type;
typedef void* LPOVERLAPPED;
#else

typedef void* dev_handle;
typedef void* ctx_handle;
typedef void* obj_handle;
typedef void* umem_handle;
typedef void* uar_handle;
typedef void* flow_handle;

#endif /* HAVE_DEVX */

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif /* UNUSED */

inline void aligned_free(void* mem_ptr)
{
    ::free(mem_ptr);
    return;
}

#endif /* SRC_DCMD_LINUX_DEF_H_ */
