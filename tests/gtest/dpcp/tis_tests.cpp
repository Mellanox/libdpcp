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
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    uint64_t flags = tis_flags::TIS_NONE;
    tis _tis(ad->get_ctx(), flags);
    uint32_t id = 0;
    status ret = _tis.get_id(id);

    log_trace("ret: %d id: 0x%x\n", ret, id);
    ASSERT_EQ(DPCP_ERR_INVALID_ID, ret);
    ASSERT_EQ(0, id);

    delete ad;
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

    uint64_t flags = tis_flags::TIS_NONE;
    tis _tis(ad->get_ctx(), flags);

    ret = _tis.create(tdn);
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

    uint64_t flags = tis_flags::TIS_TLS_EN;
    tis _tis(ad->get_ctx(), flags);
    ret = _tis.create(tdn, pdn);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tisn = 0;
    ret = _tis.get_tisn(tisn);
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

    uint64_t flags = tis_flags::TIS_NONE;
    tis _tis(ad->get_ctx(), flags);

    ret = _tis.create(tdn, pdn);
    ASSERT_EQ(DPCP_OK, ret);

    ret = _tis.destroy();
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

    uint64_t flags = tis_flags::TIS_TLS_EN;
    tis _tis(ad->get_ctx(), flags);
    ret = _tis.create(tdn, pdn);
    ASSERT_EQ(DPCP_OK, ret);

    ret = _tis.destroy();
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

    uint64_t flags = tis_flags::TIS_NONE;
    tis _tis(ad->get_ctx(), flags);
    ret = _tis.create(tdn);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tisn = 0;
    ret = _tis.get_tisn(tisn);
    ASSERT_EQ(DPCP_OK, ret);

    log_trace("tisn: 0x%x\n", tisn);

    delete ad;
}
