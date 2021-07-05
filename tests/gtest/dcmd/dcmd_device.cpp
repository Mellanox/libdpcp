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

#include "common/def.h"
#include "common/log.h"
#include "common/sys.h"
#include "common/base.h"

#include "dcmd_base.h"


class dcmd_device : public dcmd_base {};

/**
 * @test dcmd_device.ti_1
 * @brief
 *    Check device component
 * @details
 */
TEST_F(dcmd_device, ti_1)
{
    dcmd::provider *provider;
    provider = provider->get_instance();
    ASSERT_TRUE(provider != nullptr);

    dcmd::device **device_list = NULL;
    size_t device_count = 0;
    device_list = provider->get_device_list(device_count);
    ASSERT_TRUE(device_list != nullptr);
    ASSERT_LT(0, (int)device_count);

    for (int i = 0; i < (int)device_count; i++) {
        EXPECT_TRUE(device_list[i] != nullptr);
        log_trace("Device id: %s name: %s\n",
        device_list[i]->get_id().c_str(),
        device_list[i]->get_name().c_str());
    }
}

/**
 * @test dcmd_device.ti_2
 * @brief
 *    Check create_ctx()
 * @details
 */
TEST_F(dcmd_device, ti_2)
{
    dcmd::provider *provider;
    provider = provider->get_instance();
    ASSERT_TRUE(provider != nullptr);

    dcmd::device **device_list = NULL;
    size_t device_count = 0;
    device_list = provider->get_device_list(device_count);
    ASSERT_TRUE(device_list != nullptr);
    ASSERT_LT(0, (int)device_count);

    dcmd::device *dev_ptr = device_list[0];
    dcmd::ctx *ctx_ptr = dev_ptr->create_ctx();
    EXPECT_TRUE(ctx_ptr != nullptr);
    delete ctx_ptr;
}
