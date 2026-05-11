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

#ifndef SRC_DCMD_BASE_DEV_MEM_H_
#define SRC_DCMD_BASE_DEV_MEM_H_

#include <cstddef>
#include <cstdint>

namespace dcmd {

/**
 * @brief Abstract base for platform-specific device-memory wrappers.
 *
 * Defines the cross-platform surface every concrete implementation must provide.
 * Platform-specific construction and capability queries live on the concrete subclass.
 *
 * The region is registered as a zero-based MR and is addressed by its lkey
 * (@ref get_lkey) plus a byte offset. The host reaches it through
 * @ref copy_to_dev_mem / @ref copy_from_dev_mem; the device reaches it through
 * DMA WQE entries built from the lkey and offset.
 */
class base_dev_mem {
protected:
    base_dev_mem() = default;

public:
    /** Device-memory allocation alignment in bytes. */
    static constexpr size_t ALLOCATION_ALIGNMENT = 64;
    /** log2(ALLOCATION_ALIGNMENT) */
    static constexpr uint32_t LOG_ALIGN_REQ = 6;

    virtual ~base_dev_mem() = default;

    base_dev_mem(const base_dev_mem&) = delete;
    base_dev_mem& operator=(const base_dev_mem&) = delete;
    base_dev_mem(base_dev_mem&&) = delete;
    base_dev_mem& operator=(base_dev_mem&&) = delete;

    /**
     * @brief Returns the local key for HW SGL entries targeting this device-memory region.
     *
     * @return Local key (lkey) value.
     */
    virtual uint32_t get_lkey() const = 0;
    /**
     * @brief Returns the allocated size of the device-memory region.
     *
     * @return Size in bytes.
     */
    virtual size_t get_size() const = 0;
    /**
     * @brief Copies bytes from a host buffer into device memory at the given offset.
     *
     * @param [in] dst_offset  Byte offset into the device-memory region to write at.
     * @param [in] src         Source host buffer (must be non-null when length > 0).
     * @param [in] length      Number of bytes to copy.
     *
     * @return 0 on success, EINVAL on bad inputs or out-of-bounds, otherwise the verb errno.
     */
    virtual int copy_to_dev_mem(size_t dst_offset, const void* src, size_t length) noexcept = 0;
    /**
     * @brief Copies bytes from device memory at the given offset into a host buffer.
     *
     * @param [out] dst        Destination host buffer (must be non-null when length > 0).
     * @param [in]  src_offset Byte offset into the device-memory region to read from.
     * @param [in]  length     Number of bytes to copy.
     *
     * @return 0 on success, EINVAL on bad inputs or out-of-bounds, otherwise the verb errno.
     */
    virtual int copy_from_dev_mem(void* dst, size_t src_offset, size_t length) noexcept = 0;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_BASE_DEV_MEM_H_ */
