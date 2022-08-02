/*
 * Copyright © 2020-2022 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Nvidia Corporation and its affiliates
 * (the "Company") and all right, title, and interest in and to the software
 * product, including all associated intellectual property rights, are and
 * shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 */

#ifndef SRC_DCMD_LINUX_DEVICE_H_
#define SRC_DCMD_LINUX_DEVICE_H_

#include "dcmd/base/base_device.h"

namespace dcmd {

class device : public base_device {

public:
    device()
    {
        m_handle = nullptr;
    }

    device(dev_handle handle);

    virtual ~device()
    {
    }

    std::string get_name();

    ctx* create_ctx();

    inline uint32_t get_vendor_id()
    {
        return m_device_attr.vendor_id;
    }

    uint32_t get_vendor_part_id()
    {
        return m_device_attr.vendor_part_id;
    }

    ibv_device_attr* get_ibv_device_attr();

private:
    ctx* m_ctx;
    dev_handle m_handle;
    ibv_device_attr m_device_attr;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_LINUX_DEVICE_H_ */
