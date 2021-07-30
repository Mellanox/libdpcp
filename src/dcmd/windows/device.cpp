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
