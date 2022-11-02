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

#include "dpcp_base.h"

using namespace dpcp;

class dpcp_uar : public dpcp_base {
protected:
    //	SetUp() {}
};

/**
 * @test dpcp_uar.ti_01_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_uar, ti_01_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    uar_collection* uac = new (std::nothrow) uar_collection(ad->get_ctx());
    ASSERT_NE(nullptr, uac);

    size_t num = uac->num_uars();
    ASSERT_EQ(0, num);

    uar u = {};
    u = uac->get_uar((const rq*)nullptr);
    ASSERT_EQ(nullptr, u);

    delete ad;
}
/**
 * @test dpcp_uar.ti_02_get_uar
 * @brief
 *    Check uar_collection::get_uar method
 * @details
 *
 */
TEST_F(dpcp_uar, ti_02_get_uar_shared)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    uar_collection* uac = new (std::nothrow) uar_collection(ad->get_ctx());
    ASSERT_NE(nullptr, uac);

    size_t num = uac->num_uars();
    ASSERT_EQ(0, num);

    uar u1 = {};
    u1 = uac->get_uar((const void*)&u1);
    ASSERT_NE(nullptr, u1);

    num = uac->num_uars();
    ASSERT_EQ(1, num);
    // Same rq_num
    uar u2 = {};
    u2 = uac->get_uar((const void*)&u1);
    ASSERT_NE(nullptr, u2);
    ASSERT_EQ(u1, u2);

    num = uac->num_uars();
    ASSERT_EQ(1, num);

    num = uac->num_shared();
    ASSERT_EQ(1, num);

    // New rq_num
    u2 = uac->get_uar((const void*)&u2);
    ASSERT_NE(nullptr, u2);
    ASSERT_EQ(u1, u2);

    num = uac->num_uars();
    ASSERT_EQ(1, num);

    num = uac->num_shared();
    ASSERT_EQ(2, num);

    delete uac;
    delete ad;
}
/**
 * @test dpcp_uar.ti_03_release_uar
 * @brief
 *    Check uar_collection::release_uar method
 * @details
 *
 */
TEST_F(dpcp_uar, ti_03_release_uar)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    uar_collection* uac = new (std::nothrow) uar_collection(ad->get_ctx());
    ASSERT_NE(nullptr, uac);

    size_t num = uac->num_uars();
    ASSERT_EQ(0, num);

    uar u1 = {};
    u1 = uac->get_uar((void*)&u1);
    ASSERT_NE(nullptr, u1);

    num = uac->num_uars();
    ASSERT_EQ(1, num);
    // New rq_num
    uar u2 = {};
    u2 = uac->get_uar((void*)&u2);
    ASSERT_NE(nullptr, u2);
    ASSERT_EQ(u1, u2);

    num = uac->num_uars();
    ASSERT_EQ(1, num);

    num = uac->num_shared();
    ASSERT_EQ(2, num);

    // Remove 1st
    status ret = uac->release_uar((const void*)&u1);
    ASSERT_EQ(DPCP_OK, ret);

    num = uac->num_uars();
    ASSERT_EQ(1, num);

    num = uac->num_shared();
    ASSERT_EQ(1, num);

    // Remove 1st one more time
    ret = uac->release_uar((const void*)&u1);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    // Remove 2nd
    ret = uac->release_uar((const void*)&u2);
    ASSERT_EQ(DPCP_OK, ret);

    num = uac->num_uars();
    ASSERT_EQ(1, num);

    num = uac->num_shared();
    ASSERT_EQ(0, num);

    delete uac;
    delete ad;
}

/**
 * @test dpcp_uar.ti_04_get_uar_page
 * @brief
 *    Check uar_collection::get_uar_page method
 * @details
 *
 */
TEST_F(dpcp_uar, ti_04_get_uar_page)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    uar_collection* uac = new (std::nothrow) uar_collection(ad->get_ctx());
    ASSERT_NE(nullptr, uac);

    uar u1 = {};
    u1 = uac->get_uar((void*)&u1);
    ASSERT_NE(nullptr, u1);

    size_t num = uac->num_uars();
    ASSERT_EQ(1, num);

    uar_t ut1 = {};
    status ret = uac->get_uar_page(u1, ut1);
    log_trace("Id1: %u Page1: 0x%p BFReg2: 0x%p\n", ut1.m_page_id, ut1.m_page, ut1.m_bf_reg);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0, ut1.m_page_id);
    ASSERT_NE(nullptr, (void*)ut1.m_page);
    ASSERT_NE(nullptr, (void*)ut1.m_bf_reg);

    uar_t ut2 = {};
    void* bf2 = nullptr;
    ret = uac->get_uar_page(u1, ut2);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0, ut2.m_page_id);
    ASSERT_NE(nullptr, (void*)ut2.m_page);
    ASSERT_NE(nullptr, (void*)ut2.m_bf_reg);
    ASSERT_EQ(ut1, ut2);

    uar u2 = nullptr;
    ret = uac->get_uar_page(u2, ut2);
    log_trace("Id2: %u Page2: 0x%p BFReg2: 0x%p\n", ut2.m_page_id, ut2.m_page, ut2.m_bf_reg);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);
    ASSERT_NE(0, ut2.m_page_id);
    ASSERT_NE(nullptr, (void*)ut2.m_page);
    ASSERT_NE(nullptr, (void*)ut2.m_bf_reg);
    ASSERT_EQ(ut1, ut2);

    delete uac;
    delete ad;
}
