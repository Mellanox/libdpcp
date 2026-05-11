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

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>
#include <vector>

#include "common/def.h"
#include "common/log.h"
#include "common/sys.h"
#include "common/base.h"

#include "dpcp_base.h"

#include "utils/os.h"

using namespace dpcp;

/*
 * ---------------------------------------------------------------------------
 *  Shared fixture state.
 *
 *  Adapter and cap probe are populated in ti_01 and torn down in ti_14,
 *  mirroring the dpcp_dma_mmo_qp test convention. Tests in between consume
 *  s_ad / s_memic_max.
 * ---------------------------------------------------------------------------
 */
static adapter* s_ad = nullptr;
static size_t s_memic_max = 0;

/**
 * @brief Early-return if the shared adapter was not opened (e.g. running a
 *        single test alone via --gtest_filter without ti_01).
 */
#define SKIP_IF_NO_ADAPTER()                                                                       \
    do {                                                                                           \
        if (!s_ad) {                                                                               \
            return;                                                                                \
        }                                                                                          \
    } while (0)

/**
 * @brief Early-return from the current test if the HCA does not support
 *        MEMIC (on-chip device memory). Also guards against a missing shared
 *        adapter (e.g. standalone --gtest_filter runs).
 */
#define SKIP_IF_NO_MEMIC()                                                                         \
    do {                                                                                           \
        if (!s_ad || s_memic_max == 0) {                                                           \
            return;                                                                                \
        }                                                                                          \
    } while (0)

/**
 * @brief class dpcp_dev_mem - Test fixture for dpcp::dev_mem.
 */
class dpcp_dev_mem : public dpcp_base {
protected:
    void SetUp() override
    {
        dpcp_base::SetUp();
        if (errno) {
            log_trace("dpcp_dev_mem::SetUp errno=%d\n", errno);
            errno = EOK;
        }
    }
};

/**
 * @test dpcp_dev_mem.ti_01_open_adapter_and_probe_caps
 * @brief
 *    Open the adapter and populate the shared cap (s_memic_max).
 * @details
 *    All subsequent test cases consume the shared fixture; the matching
 *    teardown lives in ti_14. When the HCA does not advertise MEMIC the
 *    cap is left at 0 and downstream tests early-return.
 */
TEST_F(dpcp_dev_mem, ti_01_open_adapter_and_probe_caps)
{
    s_ad = OpenAdapter();
    ASSERT_NE(nullptr, s_ad);
    ASSERT_EQ(DPCP_OK, s_ad->open());

    s_memic_max = dev_mem::get_max_device_memory_size(s_ad);
    log_info("dpcp_dev_mem::ti_01 memic_max_size=%zu\n", s_memic_max);
}

/**
 * @test dpcp_dev_mem.ti_02_cap_struct_consistency
 * @brief
 *    The cached caps struct and the static helper agree on the MEMIC values.
 */
TEST_F(dpcp_dev_mem, ti_02_cap_struct_consistency)
{
    SKIP_IF_NO_MEMIC();

    adapter_hca_capabilities caps {};
    ASSERT_EQ(DPCP_OK, s_ad->get_hca_capabilities(caps));
    EXPECT_EQ(caps.memic_max_size, dev_mem::get_max_device_memory_size(s_ad));
    EXPECT_EQ(caps.memic_supported, caps.memic_max_size != 0);
}

/**
 * @test dpcp_dev_mem.ti_03_ctor_happy_path
 * @brief
 *    Construct a dev_mem with a non-aligned size, validate round-up and lkey.
 * @details
 *    A size of 4097 must round up to 4160 (next 64-byte multiple); lkey must
 *    be non-zero.
 */
TEST_F(dpcp_dev_mem, ti_03_ctor_happy_path)
{
    SKIP_IF_NO_MEMIC();

    constexpr size_t ALIGN = dcmd::base_dev_mem::ALLOCATION_ALIGNMENT;
    constexpr size_t REQ_SIZE = 4097;
    constexpr size_t EXPECTED_SIZE = ((REQ_SIZE + ALIGN - 1) / ALIGN) * ALIGN;

    dev_mem* raw = nullptr;
    ASSERT_EQ(DPCP_OK, s_ad->create_dev_mem(REQ_SIZE, raw));
    ASSERT_NE(nullptr, raw);
    std::unique_ptr<dev_mem> dm(raw);
    EXPECT_NE(0U, dm->get_lkey());
    EXPECT_EQ(EXPECTED_SIZE, dm->get_size());
}

/**
 * @test dpcp_dev_mem.ti_04_ctor_size_zero_invalid
 * @brief
 *    Zero-size allocation is rejected with DPCP_ERR_INVALID_PARAM.
 */
TEST_F(dpcp_dev_mem, ti_04_ctor_size_zero_invalid)
{
    SKIP_IF_NO_ADAPTER();

    dev_mem* dm = nullptr;
    EXPECT_EQ(DPCP_ERR_INVALID_PARAM, s_ad->create_dev_mem(0, dm));
    EXPECT_EQ(nullptr, dm);
}

/**
 * @test dpcp_dev_mem.ti_05_ctor_size_too_large
 * @brief
 *    Allocation larger than the device limit is rejected with DPCP_ERR_NO_MEMORY.
 */
TEST_F(dpcp_dev_mem, ti_05_ctor_size_too_large)
{
    SKIP_IF_NO_MEMIC();

    if (s_memic_max == SIZE_MAX) {
        return;
    }

    dev_mem* dm = nullptr;
    EXPECT_EQ(DPCP_ERR_NO_MEMORY, s_ad->create_dev_mem(s_memic_max + 1, dm));
    EXPECT_EQ(nullptr, dm);
}

/**
 * @test dpcp_dev_mem.ti_06_ctor_allocation_at_max
 * @brief
 *    Allocation at exactly the device limit either succeeds or fails with
 *    DPCP_ERR_DEV_MEM / DPCP_ERR_NO_MEMORY. No leak across runs.
 */
TEST_F(dpcp_dev_mem, ti_06_ctor_allocation_at_max)
{
    SKIP_IF_NO_MEMIC();

    dev_mem* raw = nullptr;
    const status s = s_ad->create_dev_mem(s_memic_max, raw);
    EXPECT_TRUE(s == DPCP_OK || s == DPCP_ERR_DEV_MEM || s == DPCP_ERR_NO_MEMORY);
    if (s == DPCP_OK) {
        ASSERT_NE(nullptr, raw);
        std::unique_ptr<dev_mem> dm(raw);
        EXPECT_NE(0U, dm->get_lkey());
    } else {
        EXPECT_EQ(nullptr, raw);
    }
}

/**
 * @test dpcp_dev_mem.ti_07_getters_stable_across_lifetime
 * @brief
 *    Repeated getter reads return stable values across the object lifetime.
 */
TEST_F(dpcp_dev_mem, ti_07_getters_stable_across_lifetime)
{
    SKIP_IF_NO_MEMIC();

    dev_mem* raw = nullptr;
    ASSERT_EQ(DPCP_OK, s_ad->create_dev_mem(4096, raw));
    ASSERT_NE(nullptr, raw);
    std::unique_ptr<dev_mem> dm(raw);

    const uint32_t lkey = dm->get_lkey();
    const size_t size = dm->get_size();

    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(lkey, dm->get_lkey());
        EXPECT_EQ(size, dm->get_size());
    }
}

/**
 * @test dpcp_dev_mem.ti_08_destruction_no_crash_no_leak
 * @brief
 *    Repeated alloc-then-delete cycles complete without crashes or leaks.
 * @details
 *    Exercises the RAII teardown of @ref dev_mem (which composes the
 *    underlying dcmd::dev_mem unique_ptr deleters for the MR and DM).
 *    Any double-free, use-after-free or leak surfaces as a crash or as a
 *    subsequent allocation failure.
 */
TEST_F(dpcp_dev_mem, ti_08_destruction_no_crash_no_leak)
{
    SKIP_IF_NO_MEMIC();

    /* Allocate slightly more than half the available MEMIC each iteration so
     * that a leaked previous allocation would prevent the next alloc from
     * succeeding - the test thus actually catches a missing free. */
    const size_t alloc_size = (s_memic_max / 2) + dcmd::base_dev_mem::ALLOCATION_ALIGNMENT;
    if (alloc_size > s_memic_max) {
        return;
    }
    for (int i = 0; i < 10; ++i) {
        dev_mem* raw = nullptr;
        ASSERT_EQ(DPCP_OK, s_ad->create_dev_mem(alloc_size, raw)) << "iteration " << i;
        ASSERT_NE(nullptr, raw);
        std::unique_ptr<dev_mem> dm(raw);
        EXPECT_NE(0U, dm->get_lkey());
    }
}

/**
 * @test dpcp_dev_mem.ti_09_multiple_instances_coexist
 * @brief
 *    Three concurrent dev_mem instances have distinct lkeys; LIFO free works.
 */
TEST_F(dpcp_dev_mem, ti_09_multiple_instances_coexist)
{
    SKIP_IF_NO_MEMIC();

    constexpr size_t NUM_INSTANCES = 3;
    constexpr size_t INSTANCE_SIZE = 16 * 1024;
    if (s_memic_max < NUM_INSTANCES * INSTANCE_SIZE) {
        return;
    }

    std::unique_ptr<dev_mem> dm[NUM_INSTANCES];
    for (size_t i = 0; i < NUM_INSTANCES; ++i) {
        dev_mem* raw = nullptr;
        ASSERT_EQ(DPCP_OK, s_ad->create_dev_mem(INSTANCE_SIZE, raw)) << "instance " << i;
        ASSERT_NE(nullptr, raw);
        dm[i].reset(raw);
        EXPECT_NE(0U, dm[i]->get_lkey());
    }
    for (size_t i = 0; i < NUM_INSTANCES; ++i) {
        for (size_t j = i + 1; j < NUM_INSTANCES; ++j) {
            EXPECT_NE(dm[i]->get_lkey(), dm[j]->get_lkey())
                << "instances " << i << " and " << j << " share lkey";
        }
    }
    /* unique_ptr array destructors run in reverse order at scope exit. */
}

/**
 * @test dpcp_dev_mem.ti_10_static_helpers_on_null_adapter
 * @brief
 *    The static helpers handle a null adapter without crashing and report
 *    "unsupported" for every query. Always runs.
 */
TEST_F(dpcp_dev_mem, ti_10_static_helpers_on_null_adapter)
{
    EXPECT_EQ(0U, dev_mem::get_max_device_memory_size(nullptr));
    EXPECT_EQ(DPCP_ERR_QUERY, dev_mem::is_supported(nullptr));
    EXPECT_EQ(DPCP_ERR_QUERY, dev_mem::is_supported(nullptr, 0));
    EXPECT_EQ(DPCP_ERR_QUERY, dev_mem::is_supported(nullptr, 1));
    EXPECT_EQ(DPCP_ERR_QUERY, dev_mem::is_supported(nullptr, SIZE_MAX));
}

/**
 * @test dpcp_dev_mem.ti_11_copy_to_dev_mem_boundaries
 * @brief
 *    copy_to_dev_mem accepts in-bounds writes and rejects out-of-bounds,
 *    null source, and offset-at-end with non-zero length. Zero-length is a
 *    legal no-op.
 */
TEST_F(dpcp_dev_mem, ti_11_copy_to_dev_mem_boundaries)
{
    SKIP_IF_NO_MEMIC();

    dev_mem* raw = nullptr;
    ASSERT_EQ(DPCP_OK, s_ad->create_dev_mem(4096, raw));
    ASSERT_NE(nullptr, raw);
    std::unique_ptr<dev_mem> dm(raw);
    const size_t size = dm->get_size();
    std::vector<uint8_t> src(size, 0xAB);

    EXPECT_EQ(0, dm->copy_to_dev_mem(64, src.data(), 256));
    EXPECT_EQ(0, dm->copy_to_dev_mem(0, src.data(), size));
    EXPECT_EQ(0, dm->copy_to_dev_mem(size, src.data(), 0));

    EXPECT_EQ(EINVAL, dm->copy_to_dev_mem(size, src.data(), 1));
    EXPECT_EQ(EINVAL, dm->copy_to_dev_mem(1, src.data(), size));
    EXPECT_EQ(EINVAL, dm->copy_to_dev_mem(0, nullptr, 1));

    /* offset + length integer overflow must be rejected (the dcmd bounds
     * check is `offset + length > m_size`; without explicit overflow
     * handling a huge offset + huge length could wrap to a small value and
     * pass the check). */
    EXPECT_EQ(EINVAL, dm->copy_to_dev_mem(SIZE_MAX, src.data(), SIZE_MAX));
}

/**
 * @test dpcp_dev_mem.ti_12_sequential_alloc_free_repeatability
 * @brief
 *    Allocate and free the same size repeatedly; catches state corruption
 *    or device-memory leaks in the underlying allocator.
 */
TEST_F(dpcp_dev_mem, ti_12_sequential_alloc_free_repeatability)
{
    SKIP_IF_NO_MEMIC();

    /* Size to slightly more than half the device memory so a leaked allocation would
     * prevent the next iteration's alloc - the test then actually catches a
     * missing free. */
    const size_t test_size = (s_memic_max / 2) + dcmd::base_dev_mem::ALLOCATION_ALIGNMENT;
    if (test_size > s_memic_max) {
        return;
    }
    for (int i = 0; i < 50; ++i) {
        dev_mem* raw = nullptr;
        ASSERT_EQ(DPCP_OK, s_ad->create_dev_mem(test_size, raw)) << "iteration " << i;
        ASSERT_NE(nullptr, raw);
        std::unique_ptr<dev_mem> dm(raw);
        EXPECT_NE(0U, dm->get_lkey());
    }
}

/**
 * @test dpcp_dev_mem.ti_13_host_to_dev_to_host_pattern_roundtrip
 * @brief
 *    Write a deterministic pattern from host to MEMIC and read it back via
 *    copy_from_dev_mem; verify byte-exact match and that adjacent guard
 *    bands are not over-written.
 */
TEST_F(dpcp_dev_mem, ti_13_host_to_dev_to_host_pattern_roundtrip)
{
    SKIP_IF_NO_MEMIC();

    constexpr size_t ALLOC_SIZE = 64 * 1024;
    constexpr size_t PAYLOAD_SIZE = 4 * 1024;
    constexpr size_t GUARD_SIZE = 64;
    constexpr size_t WRITE_OFFSET = 256;
    if (s_memic_max < ALLOC_SIZE) {
        return;
    }

    dev_mem* raw = nullptr;
    ASSERT_EQ(DPCP_OK, s_ad->create_dev_mem(ALLOC_SIZE, raw));
    ASSERT_NE(nullptr, raw);
    std::unique_ptr<dev_mem> dm(raw);

    std::vector<uint8_t> src(PAYLOAD_SIZE);
    for (size_t i = 0; i < PAYLOAD_SIZE; ++i) {
        src[i] = static_cast<uint8_t>((i * 137 + 41) & 0xFF);
    }

    std::vector<uint8_t> guard_pre(GUARD_SIZE, 0x5A);
    std::vector<uint8_t> guard_post(GUARD_SIZE, 0xA5);
    ASSERT_EQ(0, dm->copy_to_dev_mem(WRITE_OFFSET - GUARD_SIZE, guard_pre.data(), GUARD_SIZE));
    ASSERT_EQ(0, dm->copy_to_dev_mem(WRITE_OFFSET + PAYLOAD_SIZE, guard_post.data(), GUARD_SIZE));

    ASSERT_EQ(0, dm->copy_to_dev_mem(WRITE_OFFSET, src.data(), PAYLOAD_SIZE));

    /* Pre-fill destination buffers with a sentinel different from anything
     * that could plausibly come from the device, so a no-op readback (which
     * would leave the sentinel intact) fails the memcmp. */
    std::vector<uint8_t> dst(PAYLOAD_SIZE, 0x99);
    ASSERT_EQ(0, dm->copy_from_dev_mem(dst.data(), WRITE_OFFSET, PAYLOAD_SIZE));
    EXPECT_EQ(0, memcmp(src.data(), dst.data(), PAYLOAD_SIZE));

    std::vector<uint8_t> got_pre(GUARD_SIZE, 0x99);
    std::vector<uint8_t> got_post(GUARD_SIZE, 0x99);
    ASSERT_EQ(0, dm->copy_from_dev_mem(got_pre.data(), WRITE_OFFSET - GUARD_SIZE, GUARD_SIZE));
    ASSERT_EQ(0, dm->copy_from_dev_mem(got_post.data(), WRITE_OFFSET + PAYLOAD_SIZE, GUARD_SIZE));
    EXPECT_EQ(0, memcmp(guard_pre.data(), got_pre.data(), GUARD_SIZE));
    EXPECT_EQ(0, memcmp(guard_post.data(), got_post.data(), GUARD_SIZE));
}

/**
 * @test dpcp_dev_mem.ti_14_copy_from_dev_mem_boundaries
 * @brief
 *    Mirrors ti_11 for the readback direction.
 * @details
 *    Also tears down the shared adapter at the end of the suite.
 */
TEST_F(dpcp_dev_mem, ti_14_copy_from_dev_mem_boundaries)
{
    /* Adapter teardown is unconditional via RAII at scope exit; do NOT
     * insert any early return into this body - the s_ad cleanup must run
     * even when the boundary block below early-returns or asserts. */
    std::unique_ptr<adapter> ad_guard(std::exchange(s_ad, nullptr));
    if (!ad_guard) {
        return; /* standalone-filter run without ti_01; nothing to do. */
    }

    if (s_memic_max != 0) {
        dev_mem* raw = nullptr;
        ASSERT_EQ(DPCP_OK, ad_guard->create_dev_mem(4096, raw));
        ASSERT_NE(nullptr, raw);
        std::unique_ptr<dev_mem> dm(raw);
        const size_t size = dm->get_size();
        std::vector<uint8_t> dst(size, 0);

        EXPECT_EQ(0, dm->copy_from_dev_mem(dst.data(), 64, 256));
        EXPECT_EQ(0, dm->copy_from_dev_mem(dst.data(), 0, size));
        EXPECT_EQ(0, dm->copy_from_dev_mem(dst.data(), size, 0));

        EXPECT_EQ(EINVAL, dm->copy_from_dev_mem(dst.data(), size, 1));
        EXPECT_EQ(EINVAL, dm->copy_from_dev_mem(dst.data(), 1, size));
        EXPECT_EQ(EINVAL, dm->copy_from_dev_mem(nullptr, 0, 1));
    }

    s_memic_max = 0;
}
