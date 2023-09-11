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
