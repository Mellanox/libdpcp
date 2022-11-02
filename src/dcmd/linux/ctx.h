/*
 * Copyright (c) 2019-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef SRC_DCMD_LINUX_CTX_H_
#define SRC_DCMD_LINUX_CTX_H_

#include "dcmd/base/base_ctx.h"

namespace dcmd {

class ctx : public base_ctx {
public:
    ctx()
    {
        m_handle = nullptr;
    }
    ctx(dev_handle handle);
    virtual ~ctx();

    void* get_context();
    int exec_cmd(const void* in, size_t inlen, void* out, size_t outlen);
    obj* create_obj(struct obj_desc* desc);
    uar* create_uar(struct uar_desc* desc);
    umem* create_umem(struct umem_desc* desc);
    ibv_mr* ibv_reg_mem_reg_iova(struct ibv_pd* verbs_pd, void* addr, size_t length, uint64_t iova,
                                 unsigned int access);
    ibv_mr* ibv_reg_mem_reg(struct ibv_pd* verbs_pd, void* addr, size_t length,
                            unsigned int access);
    int ibv_dereg_mem_reg(struct ibv_mr* umem);
    flow* create_flow(struct flow_desc* desc);
    int query_eqn(uint32_t cpu_num, uint32_t& eqn);
    int hca_iseg_mapping();
    uint64_t get_real_time();
    int create_ibv_pd(void* ibv_pd, uint32_t& pdn);
    inline int ibv_get_access_flags()
    {
        return IBV_ACCESS_LOCAL_WRITE;
    }

private:
    ctx_handle m_handle;
    struct mlx5dv_context* m_dv_context;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_LINUX_CTX_H_ */
