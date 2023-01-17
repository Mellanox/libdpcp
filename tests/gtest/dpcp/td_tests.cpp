/*
 * Copyright (c) 2020-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include "dpcp_base.h"

using namespace dpcp;

class dpcp_td : public dpcp_base {
protected:
    //	SetUp() {}
};

/**
 * @test dpcp_td.ti_01_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_td, ti_01_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    td td1(ad->get_ctx());
    uint32_t id = td1.get_td_id();
    ASSERT_EQ(0, id);

    delete ad;
}

/**
 * @test dpcp_pd.ti_02_create
 * @brief
 *    Check pd::create method
 * @details
 *
 */
TEST_F(dpcp_td, ti_02_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    td td1(ad->get_ctx());

    status ret = td1.create();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t td_id = td1.get_td_id();
    EXPECT_LE((uint32_t)0, td_id);

    log_trace("td_id: %u\n", td_id);

    delete ad;
}
