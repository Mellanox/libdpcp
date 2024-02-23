/*
 * Copyright (c) 2020-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <sstream>

#include "common/def.h"
#include "common/log.h"
#include "common/sys.h"
#include "common/base.h"

#include "dpcp_base.h"

using namespace dpcp;

class dpcp_provider : public dpcp_base {
};

/**
 * @test dpcp_provider.ti_1
 * @brief
 *    Check singleton
 * @details
 */
TEST_F(dpcp_provider, ti_1)
{
    provider* provider1;
    status ret = provider1->get_instance(provider1);
    ASSERT_EQ(DPCP_OK, ret);

    dpcp::provider* provider2;
    ret = provider2->get_instance(provider2);
    ASSERT_EQ(DPCP_OK, ret);

    ASSERT_EQ((uintptr_t)provider1, (uintptr_t)provider2);
}

/**
 * @test dpcp_provider.ti_2
 * @brief
 *    Check get_adapter_info_lst
 * @details
 *
 */
TEST_F(dpcp_provider, ti_2)
{
    provider* p;
    status ret = p->get_instance(p);
    ASSERT_EQ(DPCP_OK, ret);

    size_t num = 0;
    ret = p->get_adapter_info_lst(nullptr, num);

    ASSERT_EQ(DPCP_ERR_OUT_OF_RANGE, ret);
    log_trace("Adapters: %zd\n", num);

    ret = p->get_adapter_info_lst(nullptr, num);

    ASSERT_EQ(DPCP_ERR_OUT_OF_RANGE, ret);
    adapter_info* ai = new (std::nothrow) adapter_info[num];

    ret = p->get_adapter_info_lst(ai, num);
    ASSERT_EQ(DPCP_OK, ret);

    for (unsigned i = 0; i < num; i++) {
        log_trace("[%u] id: %s name: %s\n", i, ai->id.c_str(), ai->name.c_str());
        ai++;
    }
}

/**
 * @test dpcp_provider.ti_3
 * @brief
 *    Check create_adapter
 * @details
 *
 */
TEST_F(dpcp_provider, ti_3)
{
    provider* p;
    status ret = p->get_instance(p);
    ASSERT_EQ(DPCP_OK, ret);

    adapter* ad1 = nullptr;
    ret = p->open_adapter("", ad1);
    ASSERT_EQ(DPCP_ERR_INVALID_ID, ret);
    ASSERT_EQ(nullptr, ad1);

    size_t num = 0;
    ret = p->get_adapter_info_lst(nullptr, num);
    adapter_info* temp_p_ainfo = new (std::nothrow) adapter_info[num];
    ret = p->get_adapter_info_lst(temp_p_ainfo, num);
    ASSERT_EQ(DPCP_OK, ret);

    adapter_info* ai = temp_p_ainfo;

    ret = p->open_adapter(ai->id, ad1);
    ASSERT_EQ(DPCP_OK, ret);
    log_trace("id: %s name: %s adapter: %p\n", ai->id.c_str(), ai->name.c_str(), ad1);

    if (num > 1) {
        ai = temp_p_ainfo + 1;
        adapter* ad2 = nullptr;
        ret = p->open_adapter(ai->id, ad2);
        ASSERT_EQ(DPCP_OK, ret);
        ASSERT_NE(ad1, ad2);
        log_trace("id: %s name: %s adapter: %p\n", ai->id.c_str(), ai->name.c_str(), ad2);
    }
}

/**
 * @test dpcp_provider.ti_4
 * @brief
 *    Check get_instance version checks
 * @details
 */
TEST_F(dpcp_provider, ti_4)
{
    provider* test_provider;

    status ret = test_provider->get_instance(test_provider, nullptr);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    int current_major = 0;
    int current_minor = 0;
    int current_patch = 0;
    const int scanned_count =
        sscanf(dpcp_version, "%d.%d.%d", &current_major, &current_minor, &current_patch);
    ASSERT_EQ(3, scanned_count);

    ret = test_provider->get_instance(test_provider, dpcp_version);
    ASSERT_EQ(DPCP_OK, ret);

    std::ostringstream bigger_major_version;
    bigger_major_version << (current_major + 1) << "." << current_minor << "." << current_patch;
    ret = test_provider->get_instance(test_provider, bigger_major_version.str().c_str());
    ASSERT_EQ(DPCP_ERR_NO_SUPPORT, ret);

    std::ostringstream smaller_major_version;
    smaller_major_version << (current_major - 1) << "." << current_minor << "." << current_patch;
    ret = test_provider->get_instance(test_provider, smaller_major_version.str().c_str());
    ASSERT_EQ(DPCP_ERR_NO_SUPPORT, ret);

    std::ostringstream bigger_minor_version;
    bigger_minor_version << current_major << "." << (current_minor + 1) << "." << current_patch;
    ret = test_provider->get_instance(test_provider, bigger_minor_version.str().c_str());
    ASSERT_EQ(DPCP_ERR_NO_SUPPORT, ret);

    if (current_minor > 0) {
        std::ostringstream smaller_minor_version;
        smaller_minor_version << current_major << "." << (current_minor - 1) << "."
                              << current_patch;
        ret = test_provider->get_instance(test_provider, smaller_minor_version.str().c_str());
        ASSERT_EQ(DPCP_OK, ret);
    }

    const char* invalid_version_format_01 = "1";
    ret = test_provider->get_instance(test_provider, invalid_version_format_01);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    const char* invalid_version_format_02 = "1.1.35bogus";
    ret = test_provider->get_instance(test_provider, invalid_version_format_02);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);
}
