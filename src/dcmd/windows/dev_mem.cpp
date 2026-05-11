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

#include "stdafx.h"
#include "dev_mem.h"
#include "utils/os.h"

using namespace dcmd;

void ibv_dm_deleter::operator()(ibv_dm* dm) const noexcept
{
    if (dm) {
        int ret = ibv_free_dm(dm);
        if (ret) {
            log_trace("~dev_mem: ibv_free_dm ret=%d errno=%d\n", ret, errno);
        }
    }
}

void ibv_mr_deleter::operator()(ibv_mr* mr) const noexcept
{
    if (mr) {
        int ret = ibv_dereg_mr(mr);
        if (ret) {
            log_trace("~dev_mem: ibv_dereg_mr ret=%d errno=%d\n", ret, errno);
        }
    }
}

dev_mem::dev_mem(ibv_context* ctx, ibv_pd* pd, size_t size)
{
    if (!ctx || !pd || size == 0) {
        log_error("dev_mem: invalid argument ctx=%p pd=%p size=%zu\n", ctx, pd, size);
        throw DCMD_EINVAL;
    }

    const size_t aligned_size = align<ALLOCATION_ALIGNMENT>(size);

    ibv_alloc_dm_attr dm_attr = {};
    dm_attr.length = aligned_size;
    dm_attr.log_align_req = LOG_ALIGN_REQ;
    dm_attr.comp_mask = 0;

    ibv_dm* dm = ibv_alloc_dm(ctx, &dm_attr);
    if (!dm) {
        log_error("dev_mem: ibv_alloc_dm size=%zu errno=%d\n", aligned_size, errno);
        throw DCMD_EIO;
    }
    m_dm.reset(dm);

    ibv_mr* mr =
        ibv_reg_dm_mr(pd, dm, 0, aligned_size, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_ZERO_BASED);
    if (!mr) {
        log_error("dev_mem: ibv_reg_dm_mr size=%zu errno=%d\n", aligned_size, errno);
        throw DCMD_EIO;
    }
    m_mr.reset(mr);
    m_lkey = mr->lkey;
    m_size = mr->length;

    log_trace("dev_mem: alloc OK lkey=0x%x size=%zu\n", m_lkey, m_size);
}

dev_mem::~dev_mem() = default;

int dev_mem::copy_to_dev_mem(size_t dst_offset, const void* src, size_t length) noexcept
{
    if (!src || !m_dm || dst_offset > m_size || length > (m_size - dst_offset)) {
        log_error("dev_mem: copy_to_dev_mem invalid argument src=%p offset=%zu length=%zu\n", src,
                  dst_offset, length);
        return EINVAL;
    }
    if (length == 0) {
        return 0;
    }
    int rc = ibv_memcpy_to_dm(m_dm.get(), dst_offset, const_cast<void*>(src), length);
    if (rc) {
        log_error("dev_mem: ibv_memcpy_to_dm offset=%zu len=%zu rc=%d\n", dst_offset, length, rc);
    }
    return rc;
}

int dev_mem::copy_from_dev_mem(void* dst, size_t src_offset, size_t length) noexcept
{
    if (!dst || !m_dm || src_offset > m_size || length > (m_size - src_offset)) {
        log_error("dev_mem: copy_from_dev_mem invalid argument dst=%p offset=%zu length=%zu\n", dst,
                  src_offset, length);
        return EINVAL;
    }
    if (length == 0) {
        return 0;
    }
    int rc = ibv_memcpy_from_dm(dst, m_dm.get(), src_offset, length);
    if (rc) {
        log_error("dev_mem: ibv_memcpy_from_dm offset=%zu len=%zu rc=%d\n", src_offset, length, rc);
    }
    return rc;
}

size_t dev_mem::get_max_device_memory_size(ibv_context* ctx)
{
    if (!ctx) {
        return 0;
    }
    ibv_device_attr_ex attr_ex = {};
    /* WinOF2 validates cb_size on input; Linux rdma-core has no such field. */
    attr_ex.cb_size = sizeof(attr_ex);
    int ret = ibv_query_device_ex(ctx, nullptr, &attr_ex);
    if (ret) {
        log_warn("dev_mem: ibv_query_device_ex failed errno=%d\n", errno);
        return 0;
    }
    return attr_ex.max_dm_size;
}

bool dev_mem::is_supported(ibv_context* ctx, size_t length)
{
    const size_t max_size = get_max_device_memory_size(ctx);
    const size_t aligned_length = align<ALLOCATION_ALIGNMENT>(length);
    return (length > 0) && (max_size > 0) && (aligned_length <= max_size);
}
