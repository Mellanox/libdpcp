/*
 Copyright (C) Mellanox Technologies, Ltd. 2020. ALL RIGHTS RESERVED.

 This software product is a proprietary product of Mellanox Technologies, Ltd.
 (the "Company") and all right, title, and interest in and to the software
 product, including all associated intellectual property rights, are and shall
 remain exclusively with the Company. All rights in or to the software product
 are licensed, not sold. All rights not licensed are reserved.

 This software product is governed by the End User License Agreement provided
 with the software product.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "utils/os.h"
#include "dpcp/internal.h"

namespace dpcp {

tir::tir(dcmd::ctx* ctx)
    : obj(ctx)
    , m_rq(nullptr)
    , m_td(nullptr)
    , m_tirn(0)
    , m_uc_self_loopback(false)
    , m_mc_self_loopback(false)
{
}

tir::~tir()
{
}

status tir::create(uint32_t td_id, uint32_t rqn)
{
    if (0 == td_id) {
        log_error("Transport Domain is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (0 == rqn) {
        log_error("ReceiveQueue is not set");
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

status tir::modify()
{
    return DPCP_OK;
}

status tir::query()
{
    return DPCP_OK;
}
} // namespace dpcp
