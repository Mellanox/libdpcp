/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2020-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef SRC_DCMD_BASE_ACTION_H_
#define SRC_DCMD_BASE_ACTION_H_

#include <vector>

namespace dcmd {

// Forward declarations
struct flow_desc;
struct fwd_dst_desc;

/**
 * @brief base_action - Action interface class.
 *
 * @note: All Action types should inherit this interface.
 */
class base_action {
public:
    base_action() = default;
    virtual ~base_action() = default;
    /**
     * @brief apply - Apply the Action to the Flow description structure.
     *
     * @param [out] desc: Flow description.
     *
     * @retval Returns DCMD_EOK for success.
     */
    virtual int apply(struct flow_desc& desc) const = 0;
};

/**
 * @brief base_action_fwd - Action forward abstract class.
 *
 * @note: All Action forward ,OS concrete, should inherit from this class.
 */
class base_action_fwd : public base_action {
public:
    base_action_fwd(const std::vector<fwd_dst_desc>& dests)
        : m_dests(dests)
    {
    }
    virtual ~base_action_fwd() = default;
    virtual int apply(struct flow_desc& desc) const = 0;

protected:
    std::vector<fwd_dst_desc> m_dests;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_BASE_ACTION_H_ */
