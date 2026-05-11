/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <cinttypes>
#include <memory>

#include "common/def.h"
#include "common/log.h"
#include "common/sys.h"
#include "common/base.h"

#include "dcmd_base.h"

/**
 * @brief Early-return from the current test if the HCA lacks MEMIC support.
 *        Detected via a 4 KiB @ref ibv_alloc_dm probe in @ref SetUp.
 */
#define SKIP_IF_DEV_MEM_UNSUPPORTED()                                                              \
    do {                                                                                           \
        if (!m_dev_mem_supported)                                                                  \
            return;                                                                                \
    } while (0)

/**
 * @brief Deleter for ibv_pd - releases the PD via @ref ibv_dealloc_pd.
 */
struct ibv_pd_deleter {
    void operator()(ibv_pd* pd) const noexcept
    {
        if (pd) {
            ibv_dealloc_pd(pd);
        }
    }
};

class dcmd_dev_mem : public dcmd_base {
protected:
    std::unique_ptr<dcmd::ctx> m_ctx_ptr;
    std::unique_ptr<ibv_pd, ibv_pd_deleter> m_pd;
    ibv_context* m_ibv_ctx = nullptr;
    bool m_dev_mem_supported = false;

    void SetUp() override
    {
        dcmd_base::SetUp();

        dcmd::provider* provider = dcmd::provider::get_instance();
        ASSERT_NE(nullptr, provider);

        size_t device_count = 0;
        dcmd::device** device_list = provider->get_device_list(device_count);
        ASSERT_NE(nullptr, device_list);
        ASSERT_LT(0U, device_count);

        /* Honor -a <adapter_name> from the gtest CLI when present; otherwise
         * fall back to device_list[0] so unfiltered runs stay device-agnostic. */
        dcmd::device* selected = nullptr;
        if (gtest_conf.adapter[0] != '\0') {
            for (size_t i = 0; i < device_count; ++i) {
                if (device_list[i]->get_name() == gtest_conf.adapter) {
                    selected = device_list[i];
                    break;
                }
            }
            ASSERT_NE(nullptr, selected)
                << "adapter '" << gtest_conf.adapter << "' not found";
        } else {
            selected = device_list[0];
        }

        m_ctx_ptr.reset(selected->create_ctx());
        ASSERT_NE(nullptr, m_ctx_ptr);

        m_ibv_ctx = static_cast<ibv_context*>(m_ctx_ptr->get_context());
        ASSERT_NE(nullptr, m_ibv_ctx);

        m_pd.reset(ibv_alloc_pd(m_ibv_ctx));
        ASSERT_NE(nullptr, m_pd);

        ibv_alloc_dm_attr probe = {};
        probe.length = 4096;
        probe.log_align_req = dcmd::dev_mem::LOG_ALIGN_REQ;
        std::unique_ptr<ibv_dm, dcmd::ibv_dm_deleter> probe_dm(ibv_alloc_dm(m_ibv_ctx, &probe));
        if (probe_dm) {
            m_dev_mem_supported = true;
        }
    }
};

/**
 * @test dcmd_dev_mem.ti_01_alloc_dm_then_reg_dm_mr
 * @brief
 *    Construct dev_mem with a non-aligned size and check the allocation round-up.
 * @details
 *    A size of 4097 must round up to 4160 (next 64-byte multiple); lkey must be non-zero.
 */
TEST_F(dcmd_dev_mem, ti_01_alloc_dm_then_reg_dm_mr)
{
    SKIP_IF_DEV_MEM_UNSUPPORTED();

    dcmd::dev_mem dm(m_ibv_ctx, m_pd.get(), 4097);

    EXPECT_NE(0U, dm.get_lkey());
    EXPECT_EQ(static_cast<size_t>(4160), dm.get_size());
}

/*
 * EXPECT_ANY_THROW (vs EXPECT_THROW(_, T)) below: dev_mem throws the
 * anonymous-enum DCMD_EINVAL across the libdpcp.so boundary; its typeinfo
 * does not match a named catch type, so type-pinned matchers fail to catch it.
 */

/**
 * @test dcmd_dev_mem.ti_02_ctor_throws_on_null_ctx
 * @brief
 *    Constructor rejects a null ibv_context and throws DCMD_EINVAL.
 */
TEST_F(dcmd_dev_mem, ti_02_ctor_throws_on_null_ctx)
{
    EXPECT_ANY_THROW({ dcmd::dev_mem dm(nullptr, m_pd.get(), 4096); });
}

/**
 * @test dcmd_dev_mem.ti_03_ctor_throws_on_null_pd
 * @brief
 *    Constructor rejects a null ibv_pd and throws DCMD_EINVAL.
 */
TEST_F(dcmd_dev_mem, ti_03_ctor_throws_on_null_pd)
{
    EXPECT_ANY_THROW({ dcmd::dev_mem dm(m_ibv_ctx, nullptr, 4096); });
}

/**
 * @test dcmd_dev_mem.ti_04_ctor_throws_on_zero_size
 * @brief
 *    Constructor rejects a zero size and throws DCMD_EINVAL.
 */
TEST_F(dcmd_dev_mem, ti_04_ctor_throws_on_zero_size)
{
    EXPECT_ANY_THROW({ dcmd::dev_mem dm(m_ibv_ctx, m_pd.get(), 0); });
}

/**
 * @test dcmd_dev_mem.ti_05_static_get_max_dm_size
 * @brief
 *    Static helper returns the same value as ibv_query_device_ex.max_dm_size.
 */
TEST_F(dcmd_dev_mem, ti_05_static_get_max_dm_size)
{
    ibv_device_attr_ex attr_ex = {};
#if !defined(__linux__)
    /* WinOF2 validates cb_size on input; Linux rdma-core has no such field. */
    attr_ex.cb_size = sizeof(attr_ex);
#endif
    int ret = ibv_query_device_ex(m_ibv_ctx, nullptr, &attr_ex);
    ASSERT_EQ(0, ret);

    log_info("dcmd_dev_mem::ti_05 ibv_query_device_ex.max_dm_size = %" PRIu64 "\n",
             static_cast<uint64_t>(attr_ex.max_dm_size));

    EXPECT_EQ(attr_ex.max_dm_size, dcmd::dev_mem::get_max_device_memory_size(m_ibv_ctx));
}

/**
 * @test dcmd_dev_mem.ti_06_copy_to_dev_mem
 * @brief
 *    Smoke-test copy_to_dev_mem plus null-source and out-of-bounds rejections.
 */
TEST_F(dcmd_dev_mem, ti_06_copy_to_dev_mem)
{
    SKIP_IF_DEV_MEM_UNSUPPORTED();

    dcmd::dev_mem dm(m_ibv_ctx, m_pd.get(), 4096);
    uint8_t payload[128] = {};
    for (size_t i = 0; i < sizeof(payload); ++i) {
        payload[i] = static_cast<uint8_t>(i);
    }

    EXPECT_EQ(0, dm.copy_to_dev_mem(0, payload, sizeof(payload)));
    EXPECT_EQ(0, dm.copy_to_dev_mem(dm.get_size() - sizeof(payload), payload, sizeof(payload)));

    EXPECT_EQ(EINVAL, dm.copy_to_dev_mem(0, nullptr, sizeof(payload)));
    EXPECT_EQ(EINVAL, dm.copy_to_dev_mem(dm.get_size(), payload, sizeof(payload)));
    EXPECT_EQ(EINVAL,
              dm.copy_to_dev_mem(0, payload, dm.get_size() + 1));
}

/**
 * @test dcmd_dev_mem.ti_07_copy_from_dev_mem
 * @brief
 *    Smoke-test copy_from_dev_mem plus null-destination and out-of-bounds rejections.
 */
TEST_F(dcmd_dev_mem, ti_07_copy_from_dev_mem)
{
    SKIP_IF_DEV_MEM_UNSUPPORTED();

    dcmd::dev_mem dm(m_ibv_ctx, m_pd.get(), 4096);
    uint8_t buf[128] = {};

    EXPECT_EQ(0, dm.copy_from_dev_mem(buf, 0, sizeof(buf)));
    EXPECT_EQ(EINVAL, dm.copy_from_dev_mem(nullptr, 0, sizeof(buf)));
    EXPECT_EQ(EINVAL, dm.copy_from_dev_mem(buf, dm.get_size(), sizeof(buf)));
    EXPECT_EQ(EINVAL, dm.copy_from_dev_mem(buf, 0, dm.get_size() + 1));
}

/**
 * @test dcmd_dev_mem.ti_08_size_alignment_table
 * @brief
 *    Exercise the 64-byte round-up at low, mid, and aligned boundaries.
 */
TEST_F(dcmd_dev_mem, ti_08_size_alignment_table)
{
    SKIP_IF_DEV_MEM_UNSUPPORTED();

    const struct {
        size_t requested;
        size_t expected;
    } cases[] = {
        {1023, 1024}, {1024, 1024}, {1025, 1088},
        {2048, 2048}, {4096, 4096}, {4097, 4160},
    };

    for (const auto& tc : cases) {
        dcmd::dev_mem dm(m_ibv_ctx, m_pd.get(), tc.requested);
        EXPECT_EQ(tc.expected, dm.get_size()) << "requested=" << tc.requested;
    }
}

/**
 * @test dcmd_dev_mem.ti_09_large_allocation
 * @brief
 *    Allocate the maximum device-memory size reported by the helper.
 */
TEST_F(dcmd_dev_mem, ti_09_large_allocation)
{
    SKIP_IF_DEV_MEM_UNSUPPORTED();

    const size_t max = dcmd::dev_mem::get_max_device_memory_size(m_ibv_ctx);
    if (max == 0) {
        log_info("SKIPPING dcmd_dev_mem::ti_09 - get_max_device_memory_size returned 0\n");
        return;
    }

    dcmd::dev_mem dm(m_ibv_ctx, m_pd.get(), max);
    EXPECT_GE(dm.get_size(), max);
    EXPECT_NE(0U, dm.get_lkey());
}

/**
 * @test dcmd_dev_mem.ti_10_is_supported
 * @brief
 *    Static is_supported returns true within reported capacity and false for
 *    null ctx or over-capacity requests.
 */
TEST_F(dcmd_dev_mem, ti_10_is_supported)
{
    SKIP_IF_DEV_MEM_UNSUPPORTED();

    const size_t max = dcmd::dev_mem::get_max_device_memory_size(m_ibv_ctx);
    ASSERT_GT(max, 0U);

    EXPECT_TRUE(dcmd::dev_mem::is_supported(m_ibv_ctx, 1));
    EXPECT_TRUE(dcmd::dev_mem::is_supported(m_ibv_ctx, max));
    EXPECT_FALSE(dcmd::dev_mem::is_supported(m_ibv_ctx, max + 1));
    EXPECT_FALSE(dcmd::dev_mem::is_supported(nullptr, max));
}
