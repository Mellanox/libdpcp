/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2019-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <utils/os.h>
#include "dcmd/dcmd.h"

using namespace dcmd;

#define HCA_CORE_CLOCK_TO_REAL_TIME_CLOCK_OFFSET 64
#define HCA_CORE_CLOCK_TO_REAL_TIME_CLOCK(hca_core_clock)                                          \
    htobe64(*((uint64_t*)((char*)(hca_core_clock) + HCA_CORE_CLOCK_TO_REAL_TIME_CLOCK_OFFSET)))

ctx::ctx(dev_handle handle)
{
    struct mlx5dv_context_attr dv_attr;
    struct ibv_context* ibv_ctx;

    memset(&dv_attr, 0, sizeof(dv_attr));
    m_dv_context = new (std::nothrow) mlx5dv_context();
    if (nullptr == m_dv_context) {
        log_error("m_dv_context is not initialized");
        throw DCMD_ENOTSUP;
    }
    dv_attr.flags |= MLX5DV_CONTEXT_FLAGS_DEVX;
    ibv_ctx = mlx5dv_open_device(handle, &dv_attr);
    if (NULL == ibv_ctx) {
        throw DCMD_ENOTSUP;
    }
    m_handle = ibv_ctx;
}

ctx::~ctx()
{
    if (m_handle) {
        ibv_close_device(m_handle);
        m_handle = nullptr;
    }
    delete m_dv_context;
}

void* ctx::get_context()
{
    return m_handle;
}

int ctx::exec_cmd(const void* in, size_t inlen, void* out, size_t outlen)
{
    int ret = mlx5dv_devx_general_cmd(m_handle, in, inlen, out, outlen);
    return (ret ? DCMD_EIO : DCMD_EOK);
}

obj* ctx::create_obj(struct obj_desc* desc)
{

    obj* obj_ptr = nullptr;

    try {
        obj_ptr = new obj(m_handle, desc);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}

uar* ctx::create_uar(struct uar_desc* desc)
{

    uar* obj_ptr = nullptr;

    try {
        obj_ptr = new uar(m_handle, desc);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}

umem* ctx::create_umem(struct umem_desc* desc)
{

    umem* obj_ptr = nullptr;

    try {
        obj_ptr = new umem(m_handle, desc);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}

flow* ctx::create_flow(struct flow_desc* desc)
{

    flow* obj_ptr = nullptr;

    try {
        obj_ptr = new flow(m_handle, desc);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}

int ctx::query_eqn(uint32_t cpu_num, uint32_t& eqn)
{
    int ret = mlx5dv_devx_query_eqn(m_handle, cpu_num, &eqn);
    log_trace("query_eqn: cpuNum: %x eqn: %x ret: %d\n", cpu_num, eqn, ret);
    return (ret ? DCMD_EIO : DCMD_EOK);
}

int ctx::hca_iseg_mapping()
{
    int ret = 0;
    m_dv_context->comp_mask |= MLX5DV_CONTEXT_MASK_HCA_CORE_CLOCK;
    ret = mlx5dv_query_device(m_handle, m_dv_context);

    return (ret ? DCMD_EIO : DCMD_EOK);
}

uint64_t ctx::get_real_time()
{
    return HCA_CORE_CLOCK_TO_REAL_TIME_CLOCK(m_dv_context->hca_core_clock);
}

ibv_mr* ctx::ibv_reg_mem_reg_iova(struct ibv_pd* verbs_pd, void* addr, size_t length, uint64_t iova,
                                  unsigned int access)
{
    return ibv_reg_mr_iova((ibv_pd*)verbs_pd, addr, length, iova, access);
}

ibv_mr* ctx::ibv_reg_mem_reg(struct ibv_pd* verbs_pd, void* addr, size_t length,
                             unsigned int access)
{
    return ibv_reg_mr((ibv_pd*)verbs_pd, addr, length, access);
}

int ctx::ibv_dereg_mem_reg(struct ibv_mr* ibv_mem)
{
    return ibv_dereg_mr(ibv_mem);
}

int ctx::create_ibv_pd(void* pd, uint32_t& pdn)
{
    mlx5dv_obj mlx5_obj;
    mlx5_obj.pd.in = (ibv_pd*)pd;
    mlx5dv_pd out_pd;
    mlx5_obj.pd.out = &out_pd;

    int ret = mlx5dv_init_obj(&mlx5_obj, MLX5DV_OBJ_PD);
    if (ret) {
        return DCMD_EINVAL;
    }
    pdn = out_pd.pdn;
    return DCMD_EOK;
}
