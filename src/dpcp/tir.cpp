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

status tir::create(uint32_t td_id, uint32_t rqn)
{
    if (0 == td_id) {
        log_error("Transport Domain is not set\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (0 == rqn) {
        log_error("ReceiveQueue is not set\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    uint32_t in[DEVX_ST_SZ_DW(create_tir_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(create_tir_out)] = {};
    size_t outlen = sizeof(out);
    //
    // Set fields in tir_context
    void* tirc = DEVX_ADDR_OF(create_tir_in, in, tir_context);
    DEVX_SET(tirc, tirc, disp_type, MLX5_TIRC_DISP_TYPE_DIRECT);

    DEVX_SET(tirc, tirc, inline_rqn, rqn);
    DEVX_SET(tirc, tirc, transport_domain, td_id);

    DEVX_SET(create_tir_in, in, opcode, MLX5_CMD_OP_CREATE_TIR);
    status ret = obj::create(in, sizeof(in), out, outlen);

    if (DPCP_OK == ret) {
        m_tirn = DEVX_GET(create_tir_out, out, tirn);
    }
    return DPCP_OK;
}

status tir::create(tir::attr& tir_attr)
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

status tir::modify(tir::attr& tir_attr)
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
