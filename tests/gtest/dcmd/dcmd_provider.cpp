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

#include "common/def.h"
#include "common/log.h"
#include "common/sys.h"
#include "common/base.h"

#include "dcmd_base.h"

class dcmd_provider : public dcmd_base {
};

/**
 * @test dcmd_provider.ti_1
 * @brief
 *    Check singleton
 * @details
 */
TEST_F(dcmd_provider, ti_1)
{
    dcmd::provider* provider1;
    provider1 = provider1->get_instance();
    ASSERT_TRUE(provider1 != nullptr);

    dcmd::provider* provider2;
    provider2 = provider2->get_instance();
    ASSERT_TRUE(provider2 != nullptr);

    ASSERT_EQ((uintptr_t)provider1, (uintptr_t)provider2);
}

/**
 * @test dcmd_provider.ti_2
 * @brief
 *    Check get_device_list()
 * @details
 */
TEST_F(dcmd_provider, ti_2)
{
    dcmd::provider* provider;
    provider = provider->get_instance();
    ASSERT_NE(nullptr, provider);

    dcmd::device** device_list = NULL;
    size_t device_count = 0;
    device_list = provider->get_device_list(device_count);
    ASSERT_NE(nullptr, device_list);
    ASSERT_LT(0, (int)device_count);
    log_trace("Number of devices: %zd\n", device_count);
}

/**
 * @test dcmd_provider.ti_3
 * @brief
 *    Check get_device_list() returns the same list
 *    during sequence of calls
 * @details
 */
TEST_F(dcmd_provider, ti_3)
{
    dcmd::provider* provider;
    provider = provider->get_instance();
    ASSERT_NE(nullptr, provider);

    dcmd::device** device_list = NULL;
    size_t device_count = 0;
    device_list = provider->get_device_list(device_count);
    ASSERT_NE(nullptr, device_list);
    ASSERT_LT(0, (int)device_count);

    for (int i = 0; i < 10; i++) {
        dcmd::device** cur_device_list = NULL;
        size_t cur_device_count = 0;
        cur_device_list = provider->get_device_list(cur_device_count);
        EXPECT_EQ((uintptr_t)device_list, (uintptr_t)cur_device_list);
        EXPECT_EQ(device_count, cur_device_count);
    }
}
