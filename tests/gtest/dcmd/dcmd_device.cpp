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
