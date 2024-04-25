/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2020-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include "dpcp/internal.h"
#include "utils/os.h"

namespace dpcp {

comp_channel::comp_channel(adapter* ad)
    : eq(ad->get_ctx())
{
    try {
        m_cc = new dcmd::compchannel((ctx_handle)(eq::get_ctx())->get_context());
    } catch (...) {
        log_error("Can't create compchannel\n");
    }
}

comp_channel::~comp_channel()
{
    delete m_cc;
}

status comp_channel::bind(cq& in_cq)
{
    uintptr_t obj_h;
    status ret = in_cq.get_handle(obj_h);
    if (ret) {
        return ret;
    }
    int err = m_cc->bind((cq_handle)obj_h, false);
    if (err) {
        return DPCP_ERR_NO_DEVICES;
    }
    return DPCP_OK;
}

status comp_channel::unbind(cq& to_unbind)
{
    UNUSED(to_unbind); // TODO: add processing
    if (m_cc->unbind()) {
        return DPCP_ERR_NO_CONTEXT;
    }
    return DPCP_OK;
}

status comp_channel::get_comp_channel(event_channel*& ch)
{
    int ret = m_cc->get_comp_channel(ch);
    if (ret) {
        return DPCP_ERR_NO_CONTEXT;
    }
    return DPCP_OK;
}

status comp_channel::request(cq& for_cq, eq_context& eq_ctx)
{
    UNUSED(for_cq); // TODO: add processing
    dcmd::compchannel_ctx cc_ctx {eq_ctx.p_overlapped, 0};
    int ret = m_cc->request(cc_ctx);
    if (ret) {
        return DPCP_ERR_NO_CONTEXT;
    }
    eq_ctx.num_eqe = cc_ctx.eqe_nums;
    return DPCP_OK;
}

status comp_channel::flush(cq& for_cq)
{
    UNUSED(for_cq); // TODO: add processing
    m_cc->flush(0);
    return DPCP_OK;
}

} // namespace dpcp
