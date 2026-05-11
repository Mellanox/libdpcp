/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef SRC_DCMD_LINUX_DEV_MEM_H_
#define SRC_DCMD_LINUX_DEV_MEM_H_

#include <cstddef>
#include <cstdint>
#include <memory>

#include <infiniband/verbs.h>

#include "dcmd/base/base_dev_mem.h"

namespace dcmd {

/**
 * @brief Deleter for ibv_dm - frees the on-chip device-memory allocation via @ref ibv_free_dm.
 */
struct ibv_dm_deleter {
    void operator()(ibv_dm* dm) const noexcept;
};

/**
 * @brief Deleter for ibv_mr - deregisters the device-memory MKey via @ref ibv_dereg_mr.
 */
struct ibv_mr_deleter {
    void operator()(ibv_mr* mr) const noexcept;
};

/**
 * @brief Linux device-memory wrapper.
 */
class dev_mem : public base_dev_mem {
public:
    /**
     * @brief Allocate and register a device-memory region of the given size.
     *
     * @param [in] ctx    IB context (must be non-null).
     * @param [in] pd     Protection domain for the MR (must be non-null).
     * @param [in] size   Requested bytes; rounded up to @ref base_dev_mem::ALLOCATION_ALIGNMENT.
     *
     * @throws DCMD_EINVAL if ctx or pd is null, or size is zero.
     * @throws DCMD_EIO on @ref ibv_alloc_dm or @ref ibv_reg_dm_mr failure.
     */
    dev_mem(ibv_context* ctx, ibv_pd* pd, size_t size);

    ~dev_mem() override;

    uint32_t get_lkey() const override
    {
        return m_lkey;
    }
    size_t get_size() const override
    {
        return m_size;
    }
    int copy_to_dev_mem(size_t dst_offset, const void* src, size_t length) noexcept override;
    int copy_from_dev_mem(void* dst, size_t src_offset, size_t length) noexcept override;

    /**
     * @brief Query the kernel-cached device-memory capacity for a context.
     *
     * @param [in] ctx  IB context to query.
     *
     * @return Maximum device-memory size in bytes, or 0 on null ctx or query failure.
     */
    static size_t get_max_device_memory_size(ibv_context* ctx);
    /**
     * @brief Check whether device memory of the given length is supported on this context.
     *
     * @param [in] ctx     IB context to query.
     * @param [in] length  Requested allocation size in bytes.
     *
     * @return true if @ref get_max_device_memory_size > 0 and @p length fits within it.
     */
    static bool is_supported(ibv_context* ctx, size_t length);

private:
    std::unique_ptr<ibv_dm, ibv_dm_deleter> m_dm;
    std::unique_ptr<ibv_mr, ibv_mr_deleter> m_mr;
    size_t m_size = 0;
    uint32_t m_lkey = 0;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_LINUX_DEV_MEM_H_ */
