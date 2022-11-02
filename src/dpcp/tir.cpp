/*
 * Copyright (c) 2019-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

tir::tir(dcmd::ctx* ctx)
    : forwardable_obj(ctx)
    , m_tirn(0)
{
    memset(&m_attr, 0, sizeof(m_attr));
}

tir::~tir()
{
}

status tir::create(const tir::attr& tir_attr)
{
    status ret = DPCP_OK;
    uint32_t in[DEVX_ST_SZ_DW(create_tir_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(create_tir_out)] = {0};
    size_t outlen = sizeof(out);
    void* tir_ctx;
    uintptr_t handle;

    if (DPCP_OK == get_handle(handle)) {
        log_error("TIR already exists\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    DEVX_SET(create_tir_in, in, opcode, MLX5_CMD_OP_CREATE_TIR);
    tir_ctx = DEVX_ADDR_OF(create_tir_in, in, tir_context);

    if (tir_attr.flags & TIR_ATTR_LRO) {
        DEVX_SET(tirc, tir_ctx, lro_timeout_period_usecs, tir_attr.lro.timeout_period_usecs);
        DEVX_SET(tirc, tir_ctx, lro_enable_mask, tir_attr.lro.enable_mask);
        DEVX_SET(tirc, tir_ctx, lro_max_ip_payload_size, tir_attr.lro.max_msg_sz);
    }

    if (tir_attr.flags & TIR_ATTR_TLS) {
        DEVX_SET(tirc, tir_ctx, self_lb_block, 3); // A must by PRM.
        DEVX_SET(tirc, tir_ctx, tls_en, tir_attr.tls_en);
    }

    if (tir_attr.flags & TIR_ATTR_INLINE_RQN) {
        DEVX_SET(tirc, tir_ctx, inline_rqn, tir_attr.inline_rqn);
    }

    if (tir_attr.flags & TIR_ATTR_TRANSPORT_DOMAIN) {
        DEVX_SET(tirc, tir_ctx, transport_domain, tir_attr.transport_domain);
    }

    ret = obj::create(in, sizeof(in), out, outlen);
    if (DPCP_OK == ret) {
        ret = obj::get_id(m_tirn);
        if (DPCP_OK == ret) {
            ret = query(m_attr);
            log_trace("TIR tirn: 0x%x created\n", m_tirn);
        }
    }

    return ret;
}

status tir::modify(const tir::attr& tir_attr)
{
    status ret = DPCP_OK;
    uint32_t in[DEVX_ST_SZ_DW(modify_tir_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(modify_tir_out)] = {0};
    size_t outlen = sizeof(out);
    void* tir_ctx;
    uintptr_t handle;

    if (DPCP_OK != get_handle(handle)) {
        log_error("TIR is invalid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    DEVX_SET(modify_tir_in, in, opcode, MLX5_CMD_OP_MODIFY_TIR);
    DEVX_SET(modify_tir_in, in, tirn, m_tirn);
    tir_ctx = DEVX_ADDR_OF(modify_tir_in, in, tir_context);

    if (tir_attr.flags & TIR_ATTR_LRO) {
        DEVX_SET(modify_tir_in, in, bitmask.lro, 1);
        DEVX_SET(tirc, tir_ctx, lro_timeout_period_usecs, tir_attr.lro.timeout_period_usecs);
        DEVX_SET(tirc, tir_ctx, lro_enable_mask, tir_attr.lro.enable_mask);
        DEVX_SET(tirc, tir_ctx, lro_max_ip_payload_size, tir_attr.lro.max_msg_sz);
    }

    ret = obj::modify(in, sizeof(in), out, outlen);
    if (DPCP_OK == ret) {
        log_trace("TIR tirn: 0x%x modified\n", m_tirn);

        if (tir_attr.flags & TIR_ATTR_LRO) {
            memcpy(&m_attr.lro, &tir_attr.lro, sizeof(m_attr.lro));
        }
    }

    return ret;
}

status tir::query(tir::attr& tir_attr)
{
    status ret = DPCP_OK;
    uint32_t in[DEVX_ST_SZ_DW(query_tir_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(query_tir_out)] = {0};
    size_t outlen = sizeof(out);
    void* tir_ctx;
    uintptr_t handle;

    if (DPCP_OK != get_handle(handle)) {
        log_error("TIR is invalid\n");
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

    memset(&tir_attr, 0, sizeof(tir_attr));

    DEVX_SET(query_tir_in, in, opcode, MLX5_CMD_OP_QUERY_TIR);
    DEVX_SET(query_tir_in, in, tirn, m_tirn);
    tir_ctx = DEVX_ADDR_OF(query_tir_out, out, tir_context);

    ret = obj::query(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        log_error("TIR query() tirn=0x%x ret=%d\n", m_tirn, ret);
        return ret;
    }

    m_attr.flags |= TIR_ATTR_LRO;
    m_attr.lro.timeout_period_usecs = DEVX_GET(tirc, tir_ctx, lro_timeout_period_usecs);
    m_attr.lro.enable_mask = DEVX_GET(tirc, tir_ctx, lro_enable_mask);
    m_attr.lro.max_msg_sz = DEVX_GET(tirc, tir_ctx, lro_max_ip_payload_size);
    m_attr.flags |= TIR_ATTR_TLS;
    m_attr.tls_en = DEVX_GET(tirc, tir_ctx, tls_en);
    m_attr.flags |= TIR_ATTR_INLINE_RQN;
    m_attr.inline_rqn = DEVX_GET(tirc, tir_ctx, inline_rqn);
    m_attr.flags |= TIR_ATTR_TRANSPORT_DOMAIN;
    m_attr.transport_domain = DEVX_GET(tirc, tir_ctx, transport_domain);

out:
    memcpy(&tir_attr, &m_attr, sizeof(m_attr));
    log_trace("TIR attr: flags=0x%x\n", m_attr.flags);
    log_trace("          lro.timeout_period_usecs=0x%x\n", m_attr.lro.timeout_period_usecs);
    log_trace("          lro.enable_mask=0x%x\n", m_attr.lro.enable_mask);
    log_trace("          lro.max_msg_sz=0x%x\n", m_attr.lro.max_msg_sz);
    log_trace("          tls_en=0x%x\n", m_attr.tls_en);
    log_trace("          inline_rqn=0x%x\n", m_attr.inline_rqn);
    log_trace("          transport_domain=0x%x\n", m_attr.transport_domain);

    return DPCP_OK;
}

int tir::get_fwd_type() const
{
    return MLX5_FLOW_DESTINATION_TYPE_TIR;
}

} // namespace dpcp
