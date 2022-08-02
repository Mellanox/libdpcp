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

tis::tis(dcmd::ctx* ctx, const uint64_t flags)
    : obj(ctx)
    , m_flags(flags)
    , m_tisn(0)
{
}

tis::~tis()
{
}

status tis::create(const uint32_t td_id, const uint32_t pd_id)
{
    if (td_id == 0) {
        log_error("Transport Domain is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    if ((m_flags & tis_flags::TIS_TLS_EN) && pd_id == 0) {
        log_error("Protection Domain is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    uint32_t in[DEVX_ST_SZ_DW(create_tis_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(create_tis_out)] = {};
    size_t outlen = sizeof(out);

    // Set fields in TIS context:
    void* tisc = DEVX_ADDR_OF(create_tis_in, in, ctx);

    DEVX_SET(tisc, tisc, transport_domain, td_id);

    if (m_flags & tis_flags::TIS_TLS_EN) {
        DEVX_SET(tisc, tisc, tls_en, 1);
        DEVX_SET(tisc, tisc, pd, pd_id);
    }

    DEVX_SET(create_tis_in, in, opcode, MLX5_CMD_OP_CREATE_TIS);
    status ret = obj::create(in, sizeof(in), out, outlen);

    if (ret == DPCP_OK) {
        m_tisn = DEVX_GET(create_tis_out, out, tisn);
        return DPCP_OK;
    } else {
        return DPCP_ERR_CREATE;
    }
}

} // namespace dpcp
