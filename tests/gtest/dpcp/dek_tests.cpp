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

class dpcp_dek : public dpcp_base {
};

/**
 * @test dpcp_dek.ti_01_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_dek, ti_01_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    dek _dek(ad->get_ctx());
    uint32_t id = 0;
    status ret = _dek.get_id(id);

    log_trace("ret: %d id: 0x%x\n", ret, id);
    ASSERT_EQ(DPCP_ERR_INVALID_ID, ret);
    ASSERT_EQ(0, id);

    delete ad;
}

/**
 * @test dpcp_dek.ti_02_create
 * @brief
 *    Check dek::create method
 * @details
 *
 */
TEST_F(dpcp_dek, ti_02_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    adapter_hca_capabilities caps;
    ret = ad->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    bool is_dek_supported = caps.general_object_types_encryption_key
        && caps.log_max_dek;
    if (!is_dek_supported) {
        log_trace("DEK is not supported \n");
        delete ad;
        return;
    }

    dek _dek(ad->get_ctx());

    const uint32_t tls_cipher_aes_gcm_128_key_size = 16;
    uint32_t key_size_bytes = tls_cipher_aes_gcm_128_key_size;
    void* key = new char[key_size_bytes];

    memcpy(key, "a6a7ee7abec9c4ce", key_size_bytes);  // Random key for the test.
    key_size_bytes = key_size_bytes;

    ret = _dek.create(ad->get_pd(), key, key_size_bytes);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek.get_key_id();
    log_trace("key_id: 0x%x\n", key_id);

    delete ad;
}

/**
 * @test dpcp_dek.ti_03_destroy
 * @brief
 *    Check dek::destroy method
 * @details
 *
 */
TEST_F(dpcp_dek, ti_03_destroy)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    adapter_hca_capabilities caps;
    ret = ad->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    bool is_dek_supported = caps.general_object_types_encryption_key
        && caps.log_max_dek;
    if (!is_dek_supported) {
        log_trace("DEK is not supported \n");
        delete ad;
        return;
    }

    dek _dek(ad->get_ctx());

    const uint32_t tls_cipher_aes_gcm_128_key_size = 16;
    uint32_t key_size_bytes = tls_cipher_aes_gcm_128_key_size;
    void* key = new char[key_size_bytes];

    memcpy(key, "a6a7ee7abec9c4ce", key_size_bytes);  // Random key for the test.
    key_size_bytes = key_size_bytes;

    ret = _dek.create(ad->get_pd(), key, key_size_bytes);
    ASSERT_EQ(DPCP_OK, ret);

    ret = _dek.destroy();
    ASSERT_EQ(DPCP_OK, ret);

    delete ad;
}
