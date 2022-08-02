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

class dcmd_ctx : public dcmd_base {
};

/**
 * @test dcmd_ctx.ti_1
 * @brief
 *    Check ctx component
 * @details
 */
TEST_F(dcmd_ctx, ti_1)
{
    dcmd::provider* provider;
    provider = provider->get_instance();
    ASSERT_TRUE(provider != nullptr);

    dcmd::device** device_list = NULL;
    size_t device_count = 0;
    device_list = provider->get_device_list(device_count);
    ASSERT_TRUE(device_list != nullptr);
    ASSERT_LT(0, (int)device_count);

    dcmd::device* dev_ptr = device_list[0];
    dcmd::ctx* ctx_ptr = dev_ptr->create_ctx();
    EXPECT_TRUE(ctx_ptr != nullptr);

    delete ctx_ptr;
}
