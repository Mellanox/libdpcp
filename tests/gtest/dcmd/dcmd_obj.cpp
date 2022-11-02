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

class dcmd_obj : public dcmd_base {
};

/**
 * @test dcmd_obj.ti_1
 * @brief
 *    Check obj component
 * @details
 */
TEST_F(dcmd_obj, ti_1)
{
    dcmd::provider* provider;
    provider = provider->get_instance();
    ASSERT_NE(nullptr, provider);
    dcmd::device** device_list = NULL;
    size_t device_count = 0;
    device_list = provider->get_device_list(device_count);
    ASSERT_NE(nullptr, device_list);
    ASSERT_LT(0, (int)device_count);

    dcmd::device* dev_ptr = device_list[0];
    dcmd::ctx* ctx_ptr = dev_ptr->create_ctx();
    ASSERT_NE(nullptr, ctx_ptr);

    uint32_t in[DEVX_ST_SZ_DW(alloc_pd_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(alloc_pd_out)] = {0};
    struct dcmd::obj_desc desc = {0};
    int pd = 0;

    DEVX_SET(alloc_pd_in, in, opcode, MLX5_CMD_OP_ALLOC_PD);
    DCMD_OBJ_SET(&desc, in, out);
    dcmd::obj* obj_ptr = ctx_ptr->create_obj(&desc);
    EXPECT_NE(nullptr, obj_ptr);
    DEVX_GET(alloc_pd_out, out, pd);
    EXPECT_LE(0, pd);
    delete obj_ptr;

    delete ctx_ptr;
}
