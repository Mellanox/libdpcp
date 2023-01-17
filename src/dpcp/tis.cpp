/*
 * Copyright (c) 2020-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
#include "dpcp/internal.h"

namespace dpcp {

tis::tis(dcmd::ctx* ctx)
    : obj(ctx)
    , m_tisn(0)
{
    memset(&m_attr, 0, sizeof(m_attr));
}

tis::~tis()
{
}

status tis::create(const tis::attr& tis_attr)
{
    uint32_t in[DEVX_ST_SZ_DW(create_tis_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(create_tis_out)] = {};
    size_t outlen = sizeof(out);
    void* tis_ctx;
    uintptr_t handle;

    if (DPCP_OK == get_handle(handle)) {
        log_error("TIS already exists\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    DEVX_SET(create_tis_in, in, opcode, MLX5_CMD_OP_CREATE_TIS);
    tis_ctx = DEVX_ADDR_OF(create_tis_in, in, ctx);

    if (tis_attr.flags & TIS_ATTR_TRANSPORT_DOMAIN) {
        DEVX_SET(tisc, tis_ctx, transport_domain, tis_attr.transport_domain);
    }

    if (tis_attr.flags & TIS_ATTR_TLS) {
        DEVX_SET(tisc, tis_ctx, tls_en, tis_attr.tls_en);
    }

    if (tis_attr.flags & TIS_ATTR_PD) {
        DEVX_SET(tisc, tis_ctx, pd, tis_attr.pd);
    }

    status ret = obj::create(in, sizeof(in), out, outlen);
    if (DPCP_OK == ret) {
        ret = obj::get_id(m_tisn);
        if (DPCP_OK == ret) {
            ret = query(m_attr);
            log_trace("TIS tisn: 0x%x created\n", m_tisn);
        }
    }

    return ret;
}

status tis::query(tis::attr& tis_attr)
{
    status ret = DPCP_OK;
    uint32_t in[DEVX_ST_SZ_DW(query_tis_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(query_tis_out)] = {0};
    size_t outlen = sizeof(out);
    void* tis_ctx;
    uintptr_t handle;

    if (DPCP_OK != get_handle(handle)) {
        log_error("TIS is invalid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    /* This check is optimization
     * During first query() call flags field should be zero
     * Once actual devx query is done all properties are cached and
     * next time query() returns data from cache
     */
    if (m_attr.flags) {
        goto out;
    }

    memset(&tis_attr, 0, sizeof(tis_attr));

    DEVX_SET(query_tis_in, in, opcode, MLX5_CMD_OP_QUERY_TIS);
    DEVX_SET(query_tis_in, in, tisn, m_tisn);
    tis_ctx = DEVX_ADDR_OF(query_tis_out, out, tis_context);

    ret = obj::query(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        log_error("TIS query() tirn=0x%x ret=%d\n", m_tisn, ret);
        return ret;
    }

    m_attr.flags |= TIS_ATTR_TRANSPORT_DOMAIN;
    m_attr.transport_domain = DEVX_GET(tisc, tis_ctx, transport_domain);
    m_attr.flags |= TIS_ATTR_TLS;
    m_attr.tls_en = DEVX_GET(tisc, tis_ctx, tls_en);
    m_attr.flags |= TIS_ATTR_PD;
    m_attr.pd = DEVX_GET(tisc, tis_ctx, pd);

out:
    memcpy(&tis_attr, &m_attr, sizeof(m_attr));
    log_trace("TIS attr: flags=0x%x\n", m_attr.flags);
    log_trace("          transport_domain=0x%x\n", m_attr.transport_domain);
    log_trace("          tls_en=0x%x\n", m_attr.tls_en);
    log_trace("          pd=0x%x\n", m_attr.pd);

    return DPCP_OK;
}

} // namespace dpcp
