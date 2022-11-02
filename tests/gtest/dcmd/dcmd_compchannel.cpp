/*
 * Copyright (c) 2020-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
#include "compchannel.h"

using namespace dcmd;
class dcmd_compchannel : public dcmd_base {
public:
    ctx* openDevice(void);
};

ctx* dcmd_compchannel::openDevice()
{
    provider* provider = provider->get_instance();
    if (!provider) {
        return nullptr;
    }

    device** device_list = NULL;
    size_t device_count = 0;
    device_list = provider->get_device_list(device_count);

    if (device_list != nullptr) {
        device* dev_ptr = device_list[0];
        return dev_ptr->create_ctx();
    }
    return nullptr;
}
/**
 * @test dcmd_compchannel.ti_01_ctr
 * @brief
 *    Check ctx component
 * @details
 */
TEST_F(dcmd_compchannel, ti_01_ctr)
{
    ctx* ctx = openDevice();
    EXPECT_NE(nullptr, ctx);

    compchannel* cc = nullptr;
    try {
        cc = new compchannel((ctx_handle)ctx->get_context());
    } catch (...) {
        log_error("Can't create compchannel for ctx %p\n", ctx);
    }
    EXPECT_NE(nullptr, cc);

    delete ctx;
}
/**
 * @test dcmd_eventqueue.ti_02_bind
 * @brief
 *    Check ctx component
 * @details
 */
TEST_F(dcmd_compchannel, ti_02_bind)
{
    ctx* ctx = openDevice();
    EXPECT_NE(nullptr, ctx);

    compchannel* cc = nullptr;
    try {
        cc = new compchannel((ctx_handle)ctx->get_context());
    } catch (...) {
        log_error("Can't create eventqueue for ctx %p\n", ctx);
    }
    EXPECT_NE(nullptr, cc);

    int ret = cc->bind(nullptr, false);
    EXPECT_EQ(DCMD_EINVAL, ret);

    delete ctx;
}
/**
 * @test dcmd_compchannel.ti_03_unbind
 * @brief
 *    Check ctx component
 * @details
 */
TEST_F(dcmd_compchannel, ti_03_unbind)
{
    ctx* ctx = openDevice();
    EXPECT_NE(nullptr, ctx);

    compchannel* cc = nullptr;
    try {
        cc = new compchannel((ctx_handle)ctx->get_context());
    } catch (...) {
        log_error("Can't create eventqueue for ctx %p\n", ctx);
    }
    EXPECT_NE(nullptr, cc);

    int ret = cc->bind(nullptr, false);
    EXPECT_EQ(DCMD_EINVAL, ret);

    ret = cc->unbind();
    EXPECT_EQ(DCMD_EOK, ret);

    delete ctx;
}
/**
 * @test dcmd_compchannel.ti_04_flush
 * @brief
 *    Check ctx component
 * @details
 */
TEST_F(dcmd_compchannel, ti_04_flush)
{
    ctx* ctx = openDevice();
    EXPECT_NE(nullptr, ctx);

    compchannel* cc = nullptr;
    try {
        cc = new compchannel((ctx_handle)ctx->get_context());
    } catch (...) {
        log_error("Can't create eventqueue for ctx %p\n", ctx);
    }
    EXPECT_NE(nullptr, cc);

    int ret = cc->bind(nullptr, false);
    EXPECT_EQ(DCMD_EINVAL, ret);

    ret = cc->unbind();
    EXPECT_EQ(DCMD_EOK, ret);

    cc->flush(0);

    delete ctx;
}
