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

class dpcp_tis : public dpcp_base {
};

/**
 * @test dpcp_tis.ti_01_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_tis, ti_01_Constructor)
{
    status ret = DPCP_OK;
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    tis tis_obj(adapter_obj->get_ctx());

    uintptr_t handle = 0;
    ret = tis_obj.get_handle(handle);
    ASSERT_EQ(DPCP_ERR_CREATE, ret);
    ASSERT_EQ(0, handle);

    uint32_t id = 0;
    ret = tis_obj.get_id(id);
    ASSERT_EQ(DPCP_ERR_INVALID_ID, ret);
    ASSERT_EQ(0, id);

    delete adapter_obj;
}

/**
 * @test dpcp_tis.ti_02_create
 * @brief
 *    Check tis::create method
 * @details
 *
 */
TEST_F(dpcp_tis, ti_02_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = ad->get_td();
    ASSERT_NE(0, tdn);

    log_trace("tdn: 0x%x\n", tdn);

    tis tis_obj(ad->get_ctx());

    struct tis::attr tis_attr;
    memset(&tis_attr, 0, sizeof(tis_attr));
    tis_attr.flags = TIS_ATTR_TRANSPORT_DOMAIN;
    tis_attr.transport_domain = tdn;
    ret = tis_obj.create(tis_attr);
    ASSERT_EQ(DPCP_OK, ret);

    delete ad;
}

/**
 * @test dpcp_tis.ti_03_create_tls_enabled
 * @brief
 *    Check tis::create method with TLS enabled
 * @details
 *
 */
TEST_F(dpcp_tis, ti_03_create_tls_enabled)
{
    adapter* ad = OpenAdapter(DevPartIdConnectX6DX);
    if (!ad) {
        log_warn("Adapter with PCI DevID %x not found, test is not run!\n", DevPartIdConnectX6DX);
        return;
    }

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    adapter_hca_capabilities caps;
    ret = ad->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    bool is_tls_supported = (caps.tls_tx || caps.tls_rx)
        && caps.general_object_types_encryption_key
        && caps.log_max_dek && caps.tls_1_2_aes_gcm_128;
    if (!is_tls_supported) {
        log_trace("TLS is not supported\n");
        delete ad;
        return;
    }

    uint32_t tdn = ad->get_td();
    ASSERT_NE(0, tdn);
    log_trace("tdn: 0x%x\n", tdn);

    uint32_t pdn = ad->get_pd();
    ASSERT_NE(0, pdn);
    log_trace("pdn: 0x%x\n", pdn);

    tis tis_obj(ad->get_ctx());

    struct tis::attr tis_attr;
    memset(&tis_attr, 0, sizeof(tis_attr));
    tis_attr.flags = TIS_ATTR_TRANSPORT_DOMAIN | TIS_ATTR_TLS | TIS_ATTR_PD;
    tis_attr.transport_domain = tdn;
    tis_attr.tls_en = is_tls_supported;
    tis_attr.pd = pdn;
    ret = tis_obj.create(tis_attr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tisn = 0;
    ret = tis_obj.get_tisn(tisn);
    ASSERT_EQ(DPCP_OK, ret);
    log_trace("tisn: 0x%x\n", tisn);

    delete ad;
}

/**
 * @test dpcp_tis.ti_04_destroy
 * @brief
 *    Check tis::destroy method
 * @details
 *
 */
TEST_F(dpcp_tis, ti_04_destroy)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = ad->get_td();
    ASSERT_NE(0, tdn);
    log_trace("tdn: 0x%x\n", tdn);

    uint32_t pdn = ad->get_pd();
    ASSERT_NE(0, pdn);
    log_trace("pdn: 0x%x\n", pdn);

    tis tis_obj(ad->get_ctx());

    struct tis::attr tis_attr;
    memset(&tis_attr, 0, sizeof(tis_attr));
    tis_attr.flags = TIS_ATTR_TRANSPORT_DOMAIN;
    tis_attr.transport_domain = tdn;
    ret = tis_obj.create(tis_attr);
    ASSERT_EQ(DPCP_OK, ret);

    ret = tis_obj.destroy();
    ASSERT_EQ(DPCP_OK, ret);

    delete ad;
}

/**
 * @test dpcp_tis.ti_05_destroy_tls_enabled
 * @brief
 *    Check tis::destroy method
 * @details
 *
 */
TEST_F(dpcp_tis, ti_05_destroy_tls_enabled)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    adapter_hca_capabilities caps;
    ret = ad->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    bool is_tls_supported = (caps.tls_tx || caps.tls_rx)
        && caps.general_object_types_encryption_key
        && caps.log_max_dek && caps.tls_1_2_aes_gcm_128;
    if (!is_tls_supported) {
        log_trace("TLS is not supported\n");
        delete ad;
        return;
    }

    uint32_t tdn = ad->get_td();
    ASSERT_NE(0, tdn);

    uint32_t pdn = ad->get_pd();
    ASSERT_NE(0, pdn);
    log_trace("pdn: 0x%x\n", pdn);

    tis tis_obj(ad->get_ctx());

    struct tis::attr tis_attr;
    memset(&tis_attr, 0, sizeof(tis_attr));
    tis_attr.flags = TIS_ATTR_TRANSPORT_DOMAIN | TIS_ATTR_TLS | TIS_ATTR_PD;
    tis_attr.transport_domain = tdn;
    tis_attr.tls_en = is_tls_supported;
    tis_attr.pd = pdn;
    ret = tis_obj.create(tis_attr);

    ret = tis_obj.destroy();
    ASSERT_EQ(DPCP_OK, ret);

    delete ad;
}

/**
 * @test dpcp_tis.ti_06_get_tisn
 * @brief
 *    Check tis::get_tisn method
 * @details
 *
 */
TEST_F(dpcp_tis, ti_06_get_tisn)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = ad->get_td();
    ASSERT_NE(0, tdn);

    log_trace("tdn: 0x%x\n", tdn);

    tis tis_obj(ad->get_ctx());

    struct tis::attr tis_attr;
    memset(&tis_attr, 0, sizeof(tis_attr));
    tis_attr.flags = TIS_ATTR_TRANSPORT_DOMAIN;
    tis_attr.transport_domain = tdn;
    ret = tis_obj.create(tis_attr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tisn = 0;
    ret = tis_obj.get_tisn(tisn);
    ASSERT_EQ(DPCP_OK, ret);

    log_trace("tisn: 0x%x\n", tisn);

    delete ad;
}
