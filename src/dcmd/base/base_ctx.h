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

#ifndef SRC_DCMD_BASE_CTX_H_
#define SRC_DCMD_BASE_CTX_H_

#include <vector>

#include "action.h"
#include "dcmd/dcmd.h"

namespace dcmd {

class obj;
class uar;
class umem;
class flow;
class action_fwd;

class base_ctx {
public:
    base_ctx()
    {
    }
    virtual ~base_ctx()
    {
    }

    virtual int exec_cmd(const void* in, size_t inlen, void* out, size_t outlen) = 0;
    virtual obj* create_obj(struct obj_desc* desc) = 0;
    virtual uar* create_uar(struct uar_desc* desc) = 0;
    virtual umem* create_umem(struct umem_desc* desc) = 0;
    virtual flow* create_flow(struct flow_desc* desc) = 0;
    std::unique_ptr<action_fwd> create_action_fwd(const std::vector<fwd_dst_desc>& dests);
};

inline std::unique_ptr<action_fwd> base_ctx::create_action_fwd(
    const std::vector<fwd_dst_desc>& dests)
{
    std::unique_ptr<action_fwd> action_ptr;

    try {
        action_ptr.reset(new action_fwd(dests));
    } catch (...) {
        return std::unique_ptr<action_fwd>(nullptr);
    }

    return action_ptr;
}

} /* namespace dcmd */

#endif /* SRC_DCMD_BASE_CTX_H_ */
