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
#ifndef SRC_DCMD_LINUX_PROVIDER_H_
#define SRC_DCMD_LINUX_PROVIDER_H_

#include "device.h"
#include "dcmd/base/base_provider.h"

namespace dcmd {

class provider : public base_provider {
public:
    static provider* get_instance()
    {
        static provider self;

        pinstance = &self;
        return pinstance;
    }

public:
    virtual ~provider()
    {
    }

    device** get_device_list(size_t& size);

private:
    provider()
    {
    }
    device* create_device(dev_handle handle);

    /* C++ 03
     * Don't forget to declare these two. You want to make sure they
     * are unacceptable otherwise you may accidentally get copies of
     * your singleton appearing.
     */
    provider(provider const&);
    void operator=(provider const&);

    static provider* pinstance;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_LINUX_PROVIDER_H_ */
