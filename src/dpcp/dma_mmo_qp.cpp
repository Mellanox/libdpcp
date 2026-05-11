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

#include "utils/os.h"
#include "dcmd/dcmd.h"
#include "dpcp/internal.h"

namespace dpcp {

static qp_attr set_dma_mmo_attr(const dma_mmo_qp_attr& attr)
{
    qp_attr dma_mmo_attr = attr;
    dma_mmo_attr.st = QPST_RC;
    dma_mmo_attr.rq_wqe_num = 0;
    dma_mmo_attr.rq_wqe_sz = 0;
    dma_mmo_attr.cqn_rcv = 0;
    return dma_mmo_attr;
}

dma_mmo_qp::dma_mmo_qp(adapter* ad, const dma_mmo_qp_attr& attr)
    : qp(ad, set_dma_mmo_attr(attr))
{
}

status dma_mmo_qp::on_build_create(void* p_in, void* /*p_qpc*/, void* p_qpc_ext)
{
    adapter_hca_capabilities caps = {};
    status ret = m_adapter->get_hca_capabilities(caps);
    if (ret != DPCP_OK) {
        log_error("dma_mmo_qp: get_hca_capabilities failed ret=%d\n", ret);
        return ret;
    }
    if (!caps.qpc_extension_supported) {
        log_error("dma_mmo_qp: QPC extension not supported by device\n");
        return DPCP_ERR_NO_SUPPORT;
    }
    if (!caps.qp_mmo_type_supported) {
        log_error("dma_mmo_qp: QP MMO type not supported by device\n");
        return DPCP_ERR_NO_SUPPORT;
    }

    DEVX_SET(create_qp_in, p_in, qpc_ext, 1);
    DEVX_SET(qp_context_extension, p_qpc_ext, mmo, 1);
    DEVX_SET(qp_context_extension, p_qpc_ext, mmo_type, MLX5_QP_CONTEXT_EXTENSION_MMO_TYPE_DMA_MMO);

    return DPCP_OK;
}

status dma_mmo_qp::on_build_init2rtr(void* p_qpc)
{
    status ret = qp::on_build_init2rtr(p_qpc);
    if (ret != DPCP_OK) {
        log_error("dma_mmo_qp: on_build_init2rtr failed ret=%d\n", ret);
        return ret;
    }
    DEVX_SET(qpc, p_qpc, primary_address_path.fl, 1);
    DEVX_SET(qpc, p_qpc, remote_qpn, m_qpn);
    return DPCP_OK;
}

} // namespace dpcp
