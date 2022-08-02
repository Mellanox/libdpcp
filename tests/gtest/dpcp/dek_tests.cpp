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
    std::unique_ptr<adapter> ad(OpenAdapter());
    ASSERT_NE(nullptr, ad);

    dek _dek(ad->get_ctx());
    uint32_t id = 0;
    status ret = _dek.get_id(id);

    log_trace("ret: %d id: 0x%x\n", ret, id);
    ASSERT_EQ(DPCP_ERR_INVALID_ID, ret);
    ASSERT_EQ(0, id);
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
    std::unique_ptr<adapter> ad(OpenAdapter());
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
    std::unique_ptr<adapter> ad(OpenAdapter());
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
        return;
    }

    dek _dek(ad->get_ctx());

    const uint32_t tls_cipher_aes_gcm_128_key_size = 16;
    uint32_t key_size_bytes = tls_cipher_aes_gcm_128_key_size;
    std::unique_ptr<char[]> key(new char[key_size_bytes]);

    memcpy(key.get(), "a6a7ee7abec9c4ce", key_size_bytes);  // Random key for the test.
    key_size_bytes = key_size_bytes;

    ret = _dek.create(ad->get_pd(), key.get(), key_size_bytes);
    ASSERT_EQ(DPCP_OK, ret);

    ret = _dek.destroy();
    ASSERT_EQ(DPCP_OK, ret);
}

/**
 * @test dpcp_dek.ti_04_create
 * @brief
 *    Check dek::create method
 * @details
 *
 */
TEST_F(dpcp_dek, ti_04_create)
{
    std::unique_ptr<adapter> ad(OpenAdapter());
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
        return;
    }

    dek _dek(ad->get_ctx());

    const uint32_t tls_cipher_aes_gcm_128_key_size = 16;
    uint32_t key_size_bytes = tls_cipher_aes_gcm_128_key_size;
    void* key = new char[key_size_bytes];

    memcpy(key, "a6a7ee7abec9c4ce", key_size_bytes);  // Random key for the test.
    key_size_bytes = key_size_bytes;

    dek::attr attrs;
    attrs.key = key;
    attrs.key_size_bytes = key_size_bytes;
    attrs.pd_id = ad->get_pd();
    ret = _dek.create(attrs);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek.get_key_id();
    log_trace("key_id: 0x%x\n", key_id);
}

/**
 * @test dpcp_dek.ti_05_modify
 * @brief
 *    Check dek::modify method
 * @details
 *
 */
TEST_F(dpcp_dek, ti_05_modify)
{
    std::unique_ptr<adapter> ad(OpenAdapter());
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    adapter_hca_capabilities caps;
    ret = ad->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    bool is_dek_supported = caps.general_object_types_encryption_key
        && caps.log_max_dek && caps.log_max_num_deks && caps.synchronize_dek;
    if (!is_dek_supported) {
        log_trace("DEK modify is not supported \n");
        return;
    }

    const uint32_t key_size_128 = 16;
    std::unique_ptr<char[]> key(new char[key_size_128]);
    std::unique_ptr<char[]> key2(new char[key_size_128]);

    memcpy(key.get(), "a6a7ee7abec9c4ce", key_size_128);  // Random key for the test.
    memcpy(key2.get(), "a6a7ee7abec9c4cb", key_size_128);  // Random key2 for the test.

    dek* _dek_ptr = nullptr;
    std::unique_ptr<dek> _dek;
    ret = ad->create_dek(ENCRYPTION_KEY_TYPE_TLS, key.get(), key_size_128, _dek_ptr);
    _dek.reset(_dek_ptr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek->get_key_id();
    log_trace("key_id: 0x%x\n", key_id);

    // Modify to a key with the same size.
    dek::attr attrs;
    attrs.key = key2.get();
    attrs.key_size_bytes = key_size_128;
    ret = _dek->modify(attrs);
    ASSERT_EQ(DPCP_OK, ret);

    ret = ad->sync_crypto_tls();
    ASSERT_EQ(DPCP_OK, ret);

    const uint32_t key_size_256 = 32;
    std::unique_ptr<char[]> key3(new char[key_size_256]);
    memcpy(key3.get(), "a6a7ee7abec9c4cba6a7ee7abec9c4cb", key_size_256);

    // Modify to a key with a different size.
    attrs.key = key3.get();
    attrs.key_size_bytes = key_size_256;
    ret = _dek->modify(attrs);
    ASSERT_EQ(DPCP_OK, ret);

    ret = ad->sync_crypto_tls();
    ASSERT_EQ(DPCP_OK, ret);
}

/**
 * @test dpcp_dek.ti_06_query
 * @brief
 *    Check dek::query method
 * @details
 *
 */
TEST_F(dpcp_dek, ti_06_query)
{
    std::unique_ptr<adapter> ad(OpenAdapter());
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    adapter_hca_capabilities caps;
    ret = ad->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    bool is_dek_supported = caps.general_object_types_encryption_key
        && caps.log_max_dek && caps.log_max_num_deks;
    if (!is_dek_supported) {
        log_trace("DEK query is not supported \n");
        return;
    }

    const uint32_t key_size_128 = 16;
    std::unique_ptr<char[]> key(new char[key_size_128]);
    memcpy(key.get(), "a6a7ee7abec9c4ce", key_size_128);  // Random key for the test.

    dek* _dek_ptr = nullptr;
    std::unique_ptr<dek> _dek;
    ret = ad->create_dek(ENCRYPTION_KEY_TYPE_TLS, key.get(), key_size_128, _dek_ptr);
    _dek.reset(_dek_ptr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek->get_key_id();
    log_trace("key_id: 0x%x\n", key_id);

    // Modify to a key with the same size.
    dek::attr attrs;
    ret = _dek->query(attrs);
    ASSERT_EQ(DPCP_OK, ret);

    std::unique_ptr<char[]> key_out(new char[attrs.key_size_bytes]);
    memset(key_out.get(), 0, attrs.key_size_bytes);

    attrs.key = key_out.get();
    ret = _dek->query(attrs);
    ASSERT_EQ(DPCP_OK, ret);

    ASSERT_EQ(0, memcmp(key_out.get(), key.get(), MIN(attrs.key_size_bytes, key_size_128)));
}
