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
#include "utils/os.h"
#include "dcmd/dcmd.h"

using namespace dcmd;

static void parse_version(size_t ms_offset, uint64_t ver, uint16_t& major, uint16_t& minor,
                          uint16_t& revision)
{
    major = (ver >> ms_offset) & 0xffff;
    minor = (ver >> (ms_offset - 16)) & 0xffff;
    revision = (ver >> (ms_offset - 32)) & 0xffff;
}

static void parse_version_driver(uint64_t ver, uint16_t& major, uint16_t& minor, uint16_t& revision)
{
    parse_version(48, ver, major, minor, revision);
}

static void parse_version_fw(uint64_t ver, uint16_t& major, uint16_t& minor, uint16_t& revision)
{
    parse_version(32, ver, major, minor, revision);
}

device::device(dev_handle handle)
{
    devx_query_device(handle, &m_dev_info);
    // as handle is just pointer to BDF which can be destroyed
    // we repointing it to m_dev_info.bdf to make safe
    m_handle = &m_dev_info.bdf;
    char buf[_MAX_U64TOSTR_BASE16_COUNT];
    m_id = _ui64toa(m_dev_info.net_luid, buf, 16);
    // PnP ID format example: "PCI\VEN_15B3&DEV_1014&SUBSYS_000815B3&REV_80\6&c1a15f4&0&2"
    uint32_t ven_id = 0;
    uint32_t ven_part_id = 0;
    int err = sscanf_s(m_dev_info.dev_pnp_id, "PCI\\VEN_%x&DEV_%x&", &ven_id, &ven_part_id);
    if ((err != 2) || !ven_id || !ven_part_id) {
        log_error("%s %x %x %d\n", m_dev_info.dev_pnp_id, ven_id, ven_part_id, err);
    } else {
        m_vendor_id = ven_id;
        m_vendor_part_id = ven_part_id;
    }
    // Provider and driver versions
    uint16_t major = 0;
    uint16_t minor = 0;
    uint16_t revision = 0;
    parse_version_driver(m_dev_info.provider_ver, major, minor, revision);
    log_trace("Provider version %d.%ld.%d\n", major, minor, revision);
    parse_version_driver(m_dev_info.driver_ver, major, minor, revision);
    log_trace("Driver version %d.%ld.%d\n", major, minor, revision);
    // FW version
    parse_version_fw(m_dev_info.fw_ver, major, minor, revision);
    log_trace("FW version %d.%ld.%d\n", major, minor, revision);
    log_trace("FT rules 0x%x NUMA 0x%x link %lld bps state %d\n", m_dev_info.supported_fs_rules,
              m_dev_info.preferred_numa_node, m_dev_info.link_speed, m_dev_info.link_state);
}

std::string device::get_name()
{
    return m_dev_info.name;
}

ctx* device::create_ctx()
{
    ctx* obj_ptr = nullptr;
    try {
        obj_ptr = new ctx(m_handle);
    } catch (...) {
        delete obj_ptr;
        return nullptr;
    }
    return obj_ptr;
}
