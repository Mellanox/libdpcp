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
