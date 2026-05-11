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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cerrno>

#include "utils/os.h"
#include "dpcp/internal.h"

namespace dpcp {

namespace {

bool try_get_caps(const adapter* ad, adapter_hca_capabilities& caps)
{
    return ad && ad->get_hca_capabilities(caps) == DPCP_OK;
}

} // namespace

dev_mem::dev_mem(adapter* ad, size_t size, status& s_out)
    : obj(ad ? ad->get_ctx() : nullptr)
    , m_adapter(ad)
    , m_dev_mem(nullptr)
{
    s_out = DPCP_OK;

    if (!ad) {
        s_out = DPCP_ERR_NO_CONTEXT;
        log_error("dpcp::dev_mem: adapter is null\n");
        return;
    }

    void* ibv_pd_v = nullptr;
    status ret = ad->get_ibv_pd(ibv_pd_v);
    if (ret != DPCP_OK || !ibv_pd_v) {
        s_out = DPCP_ERR_NO_CONTEXT;
        log_error("dpcp::dev_mem: failed to get ibv_pd\n");
        return;
    }

    ibv_context* ibv_ctx = static_cast<ibv_context*>(ad->get_ctx()->get_context());
    if (!ibv_ctx) {
        s_out = DPCP_ERR_NO_CONTEXT;
        log_error("dpcp::dev_mem: failed to get ibv_ctx\n");
        return;
    }

    try {
        m_dev_mem.reset(new dcmd::dev_mem(ibv_ctx, reinterpret_cast<ibv_pd*>(ibv_pd_v), size));
    } catch (int dcmd_err) {
        log_error("dpcp::dev_mem: dcmd ctor failed dcmd_err=%d size=%zu\n", dcmd_err, size);
        s_out = DPCP_ERR_DEV_MEM;
    } catch (const std::exception& e) {
        log_error("dpcp::dev_mem: dcmd ctor threw unexpected exception: %s\n", e.what());
        s_out = DPCP_ERR_DEV_MEM;
    } catch (...) {
        log_error("dpcp::dev_mem: dcmd ctor threw non-int exception size=%zu\n", size);
        s_out = DPCP_ERR_DEV_MEM;
    }
}

dev_mem::~dev_mem() = default;

uint32_t dev_mem::get_lkey() const
{
    return m_dev_mem->get_lkey();
}

size_t dev_mem::get_size() const
{
    return m_dev_mem ? m_dev_mem->get_size() : 0;
}

int dev_mem::copy_to_dev_mem(size_t dst_offset, const void* src, size_t length) noexcept
{
    if (!m_dev_mem) {
        log_error("dpcp::dev_mem: dev_mem is null\n");
        return EINVAL;
    }
    return m_dev_mem->copy_to_dev_mem(dst_offset, src, length);
}

int dev_mem::copy_from_dev_mem(void* dst, size_t src_offset, size_t length) noexcept
{
    if (!m_dev_mem) {
        log_error("dpcp::dev_mem: dev_mem is null\n");
        return EINVAL;
    }
    return m_dev_mem->copy_from_dev_mem(dst, src_offset, length);
}

size_t dev_mem::get_max_device_memory_size(const adapter* ad)
{
    adapter_hca_capabilities caps {};
    auto ret = try_get_caps(ad, caps);
    if (!ret) {
        log_error("dpcp::dev_mem: failed to get adapter capabilities\n");
        return 0;
    }
    return caps.memic_max_size;
}

status dev_mem::is_supported(const adapter* ad)
{
    adapter_hca_capabilities caps {};
    auto ret = try_get_caps(ad, caps);
    if (!ret) {
        log_error("dpcp::dev_mem: failed to get adapter capabilities\n");
        return DPCP_ERR_QUERY;
    }
    if (!caps.memic_supported) {
        return DPCP_ERR_NO_SUPPORT;
    }
    return DPCP_OK;
}

status dev_mem::is_supported(const adapter* ad, size_t length)
{
    adapter_hca_capabilities caps {};
    auto ret = try_get_caps(ad, caps);
    if (!ret) {
        log_error("dpcp::dev_mem: failed to get adapter capabilities\n");
        return DPCP_ERR_QUERY;
    }
    if (!caps.memic_supported || caps.memic_max_size == 0) {
        return DPCP_ERR_NO_SUPPORT;
    }
    if (length > caps.memic_max_size) {
        return DPCP_ERR_NO_MEMORY;
    }
    return DPCP_OK;
}

} // namespace dpcp
