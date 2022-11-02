/*
 * Copyright (c) 2019-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
