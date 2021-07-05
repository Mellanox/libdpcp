/*
Copyright (C) Mellanox Technologies, Ltd. 2001-2020. ALL RIGHTS RESERVED.

This software product is a proprietary product of Mellanox Technologies, Ltd.
(the "Company") and all right, title, and interest in and to the software
product, including all associated intellectual property rights, are and shall
remain exclusively with the Company.  All rights in or to the software product
are licensed, not sold.  All rights not licensed are reserved.

This software product is governed by the End User License Agreement provided
with the software product.
*/

#include <string>
#include "utils/os.h"
#include "dcmd/dcmd.h"

using namespace dcmd;

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
