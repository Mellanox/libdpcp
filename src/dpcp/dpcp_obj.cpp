/*
 * Copyright (c) 2019-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include "utils/os.h"
#include "dcmd/dcmd.h"
#include "api/dpcp.h"

namespace dpcp {

obj::obj(dcmd::ctx* ctx)
    : m_id(0LL)
    , m_obj_handle(nullptr)
    , m_ctx(ctx)
    , m_last_status(0)
    , m_last_syndrome(0)
{
}

status obj::destroy()
{
    status ret = DPCP_OK;
    int err = 0;
    errno = 0;
    if (m_obj_handle) {
        err = m_obj_handle->destroy();
        if (err) {
            ret = DPCP_OK;
        }
    }
    log_trace("dpcp_obj::destroy %p dcmd_obj %p id=0x%x ret=%d\n", this, m_obj_handle, m_id, err);
    return ret;
}

obj::~obj()
{
    delete m_obj_handle;
    m_obj_handle = nullptr;
    m_id = 0LL;
    m_ctx = nullptr;
}

status obj::get_id(uint32_t& id)
{
    if (m_obj_handle) {
        id = m_id;
        return DPCP_OK;
    }
    return DPCP_ERR_INVALID_ID;
}

status obj::get_handle(uintptr_t& handle) const
{
    if (m_obj_handle) {
        handle = m_obj_handle->get_handle();
        return DPCP_OK;
    }
    return DPCP_ERR_CREATE;
}

status obj::create(void* in, size_t inlen, void* out, size_t& outlen)
{
    if (!m_ctx)
        return DPCP_ERR_NO_CONTEXT;

    if ((nullptr == in) || (nullptr == out) || (inlen < 16) || (outlen < 16))
        return DPCP_ERR_INVALID_PARAM;

    struct dcmd::obj_desc obj_desc = {in, inlen, out, outlen};

    log_trace("create in: %p inlen: %zu out: %p outlen: %zu\n", obj_desc.in, obj_desc.inlen,
              obj_desc.out, obj_desc.outlen);

    m_obj_handle = m_ctx->create_obj(&obj_desc);

    m_last_status = DEVX_GET(status_out, out, status);
    m_last_syndrome = DEVX_GET(status_out, out, syndrome);
    m_id = DEVX_GET(status_out, out, id);
    log_trace("obj_handle: %p status: %u syndrome: %x id: 0x%x\n", m_obj_handle, m_last_status,
              m_last_syndrome, m_id);

    if ((nullptr == m_obj_handle) || (0 != m_last_status))
        return DPCP_ERR_CREATE;

    return DPCP_OK;
}

status obj::modify(void* in, size_t inlen, void* out, size_t& outlen)
{
    if (!m_ctx)
        return DPCP_ERR_NO_CONTEXT;

    if ((nullptr == in) || (nullptr == out) || (inlen < 16) || (outlen < 16))
        return DPCP_ERR_INVALID_PARAM;

    struct dcmd::obj_desc obj_desc = {in, inlen, out, outlen};

    log_trace("modify in: %p inlen: %zu out: %p outlen: %zu\n", obj_desc.in, obj_desc.inlen,
              obj_desc.out, obj_desc.outlen);

    int ret = m_obj_handle->modify(&obj_desc);

    if (DCMD_EOK != ret) {
        m_last_status = DEVX_GET(status_out, out, status);
        m_last_syndrome = DEVX_GET(status_out, out, syndrome);
        log_error("modify returns: %d\n", ret);
        log_trace("modify status: %u syndrome: %x\n", m_last_status, m_last_syndrome);
        return DPCP_ERR_MODIFY;
    }

    m_last_status = DEVX_GET(status_out, out, status);
    m_last_syndrome = DEVX_GET(status_out, out, syndrome);
    log_trace("modify status: %u syndrome: %x\n", m_last_status, m_last_syndrome);

    if (0 != m_last_status)
        return DPCP_ERR_MODIFY;
    return DPCP_OK;
}

status obj::query(void* in, size_t inlen, void* out, size_t& outlen)
{
    if (!m_ctx)
        return DPCP_ERR_NO_CONTEXT;

    if ((nullptr == in) || (nullptr == out) || (inlen < 16) || (outlen < 16))
        return DPCP_ERR_INVALID_PARAM;

    struct dcmd::obj_desc obj_desc = {in, inlen, out, outlen};

    log_trace("query in: %p inlen: %zu out: %p outlen: %zu\n", obj_desc.in, obj_desc.inlen,
              obj_desc.out, obj_desc.outlen);

    int ret = m_obj_handle->query(&obj_desc);

    m_last_status = DEVX_GET(status_out, out, status);
    m_last_syndrome = DEVX_GET(status_out, out, syndrome);
    log_trace("query status: %u syndrome: %x\n", m_last_status, m_last_syndrome);

    if ((DCMD_EOK != ret) || (0 != m_last_status)) {
        log_error("query returns: %d\n", ret);
        return DPCP_ERR_QUERY;
    }
    return DPCP_OK;
}
} // namespace dpcp
