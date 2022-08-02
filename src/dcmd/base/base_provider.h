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

#ifndef SRC_DCMD_BASE_PROVIDER_H_
#define SRC_DCMD_BASE_PROVIDER_H_

namespace dcmd {

class device;

class base_provider {
public:
    base_provider()
    {
        m_dev_array = nullptr;
        m_dev_array_size = 0;
    }
    virtual ~base_provider()
    {
        for (size_t i = 0; i < m_dev_array_size; i++) {
            delete m_dev_array[i];
        }
        delete[] m_dev_array;
    }

    virtual device** get_device_list(size_t& size) = 0;

protected:
    device** m_dev_array;
    size_t m_dev_array_size;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_BASE_PROVIDER_H_ */
