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
