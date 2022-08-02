/*
 * Copyright © 2020-2022 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Nvidia Corporation and its affiliates
 * (the "Company") and all right, title, and interest in and to the software
 * product, including all associated intellectual property rights, are and
 * shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 */

#ifndef SRC_DCMD_LINUX_DEF_H_
#define SRC_DCMD_LINUX_DEF_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <cstdlib>
#include <infiniband/verbs.h>
#include <infiniband/mlx5dv.h>

typedef struct ibv_device* dev_handle;
typedef struct ibv_context* ctx_handle;
typedef struct mlx5dv_devx_obj* obj_handle;
typedef struct mlx5dv_devx_umem* umem_handle;
typedef struct mlx5dv_devx_uar* uar_handle;
typedef struct ibv_flow* flow_handle;
typedef struct ibv_cq* cq_handle;
/*
 * Packet Pacing
 */
typedef struct mlx5dv_pp pp_handle;

inline pp_handle* devx_alloc_pp(ctx_handle ctx, const void* pp_ctx, size_t pp_sz, uint32_t flags)
{
    return mlx5dv_pp_alloc(ctx, pp_sz, pp_ctx, flags);
}

inline uint32_t get_pp_index(pp_handle* pp)
{
    if (pp) {
        return pp->index;
    }
    return 0;
}

#define devx_free_pp mlx5dv_pp_free

#define IS_ERR(ptr) (nullptr == ptr)

#if !defined(EVENT_CHANNEL)
typedef struct {
    int fd;
} event_channel;
#define EVENT_CHANNEL
#endif

typedef struct ibv_comp_channel comp_channel;
typedef ibv_event_type devx_event_type;
typedef void* LPOVERLAPPED;

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif /* UNUSED */

inline void aligned_free(void* mem_ptr)
{
    ::free(mem_ptr);
    return;
}

#endif /* SRC_DCMD_LINUX_DEF_H_ */
