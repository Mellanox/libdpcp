/*
Copyright (C) Mellanox Technologies, Ltd. 2020. ALL RIGHTS RESERVED.

This software product is a proprietary product of Mellanox Technologies, Ltd.
(the "Company") and all right, title, and interest in and to the software
product, including all associated intellectual property rights, are and shall
remain exclusively with the Company. All rights in or to the software product
are licensed, not sold. All rights not licensed are reserved.

This software product is governed by the End User License Agreement provided
with the software product.
*/

#ifndef SRC_DCMD_WINDOWS_DEVICE_H_
#define SRC_DCMD_WINDOWS_DEVICE_H_

#include "dcmd/base/base_device.h"

namespace dcmd {

class device : public base_device {
public:
    device()
        : m_handle(nullptr)
        , m_dev_info()
        , m_vendor_id(0)
        , m_vendor_part_id(0)
    {
    }

    device(dev_handle handle);

    virtual ~device()
    {
    }

    std::string get_name();

    ctx* create_ctx();

    uint32_t get_vendor_id()
    {
        return m_vendor_id;
    }

    uint32_t get_vendor_part_id()
    {
        return m_vendor_part_id;
    }

private:
    dev_handle m_handle;
    devx_device m_dev_info;
    uint32_t m_vendor_id;
    uint32_t m_vendor_part_id;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_WINDOWS_DEVICE_H_ */
