/*
 * Copyright © 2019-2022 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
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
