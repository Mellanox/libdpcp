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

#include <string>

#include "dcmd/dcmd.h"
#include "utils/os.h"

using namespace dcmd;

compchannel::compchannel(ctx_handle ctx)
    : m_ctx(ctx)
    , m_cq_obj(nullptr)
    , m_binded(false)
    , m_solicited(false)
{
    // use ibv_completion_channel interface for this case
    comp_channel* cch = ibv_create_comp_channel(m_ctx);

    if (nullptr == cch) {
        log_error("create_comp_channel failed errno=0x%x\n", errno);
        throw DCMD_ENOTSUP;
    }
    m_event_channel = *cch;
}

int compchannel::bind(cq_handle cq_obj, bool solicited_only)
{
    if (cq_obj) {
        m_cq_obj = cq_obj;
        m_solicited = solicited_only;
    } else {
        return DCMD_EINVAL;
    }
    int err = ibv_req_notify_cq(m_cq_obj, m_solicited);
    if (err) {
        log_error("bind req_notify_cq ret= %d errno=%d\n", err, errno);
        return DCMD_EIO;
    }
    m_binded = true;
    return err;
}

int compchannel::unbind()
{
    m_binded = false;
    return DCMD_EOK;
}

int compchannel::get_comp_channel(::event_channel*& ch)
{
    ch = (::event_channel*)&m_event_channel;
    return DCMD_EOK;
}

int compchannel::request(compchannel_ctx& cc_ctx)
{
    UNUSED(cc_ctx);

    int err = ibv_req_notify_cq(m_cq_obj, m_solicited);
    if (err) {
        log_error("bind req_notify_cq ret= %d errno=%d\n", err, errno);
        return DCMD_EIO;
    }
    return DCMD_EOK;
}

int compchannel::query(void*& ctx)
{
    cq_handle event_cq = nullptr;
    void* cq_ctx = nullptr;
    int err = ibv_get_cq_event(&m_event_channel, &event_cq, &cq_ctx);

    if (err) {
        log_error("query get_cq_event ret= %d errno=%d\n", err, errno);
        return DCMD_EIO;
    }
    if (m_cq_obj != event_cq) {
        log_error("complitions for cq=%p, binded cq=%p\n", event_cq, m_cq_obj);
        return DCMD_EIO;
    }
    ctx = cq_ctx;
    return DCMD_EOK;
}

void compchannel::flush(uint32_t nevents)
{
    if (m_cq_obj && nevents) {
        ibv_ack_cq_events(m_cq_obj, nevents);
        log_trace("flush() compchannel OK\n");
    } else {
        log_warn("flush() compchannel nothing to do\n");
    }
}

compchannel::~compchannel()
{
    int err = ibv_destroy_comp_channel(&m_event_channel);
    if (err) {
        log_error("DTR compchannel ret = %d\n", err);
    } else {
        log_trace("DTR compchannel OK\n");
    }
}
