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

#include <string>
#include "utils/os.h"
#include "dcmd/dcmd.h"

using namespace dcmd;

device::device(dev_handle handle)
    : m_ctx(nullptr)
    , m_handle(handle)
{
    m_id = std::string(handle->name);
    m_name = std::string(handle->name);
    memset(&m_device_attr, 0, sizeof(m_device_attr));
}

std::string device::get_name()
{
    return m_name;
}

ctx* device::create_ctx()
{

    m_ctx = nullptr;

    try {
        m_ctx = new ctx(m_handle);
    } catch (...) {
        return nullptr;
    }

    return m_ctx;
}

ibv_device_attr* device::get_ibv_device_attr()
{
    int err = ibv_query_device((ibv_context*)m_ctx->get_context(), &m_device_attr);
    if (err) {
        log_warn("query device failed! errno=%d\n", errno);
        return nullptr;
    }
    log_trace("FW ver. %s HW ver 0x%x Ports %d\n", m_device_attr.fw_ver, m_device_attr.hw_ver,
              m_device_attr.phys_port_cnt);
    return &m_device_attr;
}
