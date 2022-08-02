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

#ifndef SRC_DCMD_BASE_DEVICE_H_
#define SRC_DCMD_BASE_DEVICE_H_

#include <string>

namespace dcmd {

class ctx;

class base_device {
public:
    base_device()
    {
    }
    virtual ~base_device()
    {
    }

    virtual ctx* create_ctx() = 0;

    inline std::string get_id()
    {
        return m_id;
    }
    inline std::string get_name()
    {
        return m_name;
    }

    virtual uint32_t get_vendor_id() = 0;
    virtual uint32_t get_vendor_part_id() = 0;

protected:
    std::string m_id;
    std::string m_name;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_BASE_DEVICE_H_ */
