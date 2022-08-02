/*
 * Copyright © 2001-2022 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Nvidia Corporation and its affiliates
 * (the "Company") and all right, title, and interest in and to the software
 * product, including all associated intellectual property rights, are and
 * shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
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
