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
#include "stdafx.h"
#include "provider.h"

using namespace dcmd;

provider* provider::pinstance = 0;

device** provider::get_device_list(size_t& size)
{
    if (nullptr == m_dev_array) {
        dev_handle device_list = nullptr;
        size_t num_devices = 0;
        m_dev_array_size = 0;
        // get device list
        int err = devx_get_device_list(&num_devices, &device_list);
        if (err) {
            goto exit;
        }
        log_trace("");
        m_dev_array = new (std::nothrow) device*[num_devices];
        if (m_dev_array) {
            /* search for the given device in the device list */
            for (int i = 0; i < (int)num_devices; ++i) {
                device* dv = create_device(device_list + i);
                if (dv) {
                    m_dev_array[m_dev_array_size++] = dv;
                }
            }
        }

        devx_free_device_list(device_list);
    }

exit:
    size = m_dev_array_size;

    return m_dev_array;
}

device* provider::create_device(dev_handle handle)
{
    device* obj_ptr = nullptr;
    bool can_be_open = false;

    try {
        obj_ptr = new device(handle);
        // test is device can be opened
        ctx* ctx_obj = obj_ptr->create_ctx();
        if (ctx_obj) {
            can_be_open = true;
            delete ctx_obj;
        }
    } catch (...) {
        if (obj_ptr) {
            delete obj_ptr;
        }
        return nullptr;
    }

    if (can_be_open) {
        return obj_ptr;
    }
    return nullptr;
}
