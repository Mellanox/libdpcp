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
