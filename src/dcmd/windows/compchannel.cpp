/*
 * Copyright (c) 2020-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include "stdafx.h"
#include "src/dcmd/dcmd.h"
#include "compchannel.h"

using namespace dcmd;

compchannel::compchannel(ctx_handle ctx)
    : m_ctx(ctx)
    , m_handle(INVALID_HANDLE_VALUE)
    , m_binded(false)
{
    if (INVALID_HANDLE_VALUE == m_handle) {
        int err = devx_overlapped_file_open(m_ctx, &m_handle);
        if (err) {
            log_error("overlapped_file_open failed ret=0x%x\n", err);
            throw DCMD_ENOTSUP;
        }
    }
    log_trace("overlapped file handle %p\n", m_handle);
}

int compchannel::bind(obj_handle src_obj, bool solicited)
{
    UNUSED(solicited);
    if (src_obj) {
        m_cq_obj = src_obj;
    } else {
        log_error("event src obj is missing\n");
        return DCMD_EINVAL;
    }

    int err = devx_overlapped_io_enable(m_cq_obj, DEVX_EVENT_TYPE_CQE);
    if (err) {
        log_error("io_enable ret = %d\n", err);
        return DCMD_EIO;
    }
    log_trace("overlapped_io_enable ret = %d\n", err);
    m_binded = true;
    return err;
}

int compchannel::unbind()
{
    // After flush completions will not be generated till next park (query)
    flush(0);
    m_binded = false;
    return DCMD_EOK;
}

int compchannel::get_comp_channel(event_channel*& ch)
{
    ch = &m_handle;
    return DCMD_EOK;
}

int compchannel::request(compchannel_ctx& cc_ctx)
{
    uint32_t num_eqe = 0;
    int err = devx_overlapped_io_park(m_handle, m_cq_obj, DEVX_EVENT_TYPE_CQE, cc_ctx.overlapped,
                                      &num_eqe);
#if defined(DPCP_DEBUG)
    log_trace("m_handle %p err %d num_eqe %d ovl %p\n", m_handle, err, num_eqe, cc_ctx.overlapped);
#endif
    if (err) {
        if (err != -ECANCELED) {
            return DCMD_EIO;
        }

        m_adapter_shutdown = true;
        return DCMD_ENOTSUP;
    }
    cc_ctx.eqe_nums = num_eqe;
    return DCMD_EOK;
}

void compchannel::flush(uint32_t unused)
{
    UNUSED(unused);
    if (m_binded) {
        devx_overlapped_io_flush(m_cq_obj, DEVX_EVENT_TYPE_CQE);
    }
}

compchannel::~compchannel()
{
    flush(0);
    CloseHandle(m_handle);
}
