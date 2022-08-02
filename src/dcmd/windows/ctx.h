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

#ifndef SRC_DCMD_WINDOWS_CTX_H_
#define SRC_DCMD_WINDOWS_CTX_H_

#include "dcmd/base/base_ctx.h"
#include "mlx5verbs.h"

namespace dcmd {

class ctx : public base_ctx {
public:
    ctx()
        : m_handle(nullptr)
        , m_pv_iseg(nullptr)
    {
    }
    ctx(dev_handle handle);
    virtual ~ctx();

    void* get_context();
    int exec_cmd(const void* in, size_t inlen, void* out, size_t outlen);
    obj* create_obj(struct obj_desc* desc);
    uar* create_uar(struct uar_desc* desc);
    umem* create_umem(struct umem_desc* desc);
    flow* create_flow(struct flow_desc* desc);
    int query_eqn(uint32_t cpu_num, uint32_t& eqn);
    int hca_iseg_mapping();
    uint64_t get_real_time();
    ibv_mr* ibv_reg_mem_reg_iova(struct ibv_pd* verbs_pd, void* addr, size_t length, uint64_t iova,
                                 unsigned int access);
    ibv_mr* ibv_reg_mem_reg(struct ibv_pd* verbs_pd, void* addr, size_t length,
                            unsigned int access);
    int ibv_dereg_mem_reg(struct ibv_mr* ibv_mem);
    int create_ibv_pd(void* ibv_pd, uint32_t& pdn);
    inline int ibv_get_access_flags()
    {
        return WIN_IBV_ACCESS_LOCAL_WRITE;
    }

private:
    ctx_handle m_handle;
    void* m_pv_iseg;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_WINDOWS_CTX_H_ */
