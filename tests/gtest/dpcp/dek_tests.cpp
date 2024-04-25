/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2021-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

////////////////////////////////////////////////////////////////////////
// tls dek tests.                                                     //
////////////////////////////////////////////////////////////////////////

class dpcp_tls_dek : public dpcp_base {
};

/**
 * @test dpcp_dek.ti_01_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_tls_dek, ti_01_Constructor)
{
    std::unique_ptr<adapter> ad(OpenAdapter());
    ASSERT_NE(nullptr, ad);

    tls_dek _dek(ad->get_ctx());
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
TEST_F(dpcp_tls_dek, ti_02_create)
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

    tls_dek _dek(ad->get_ctx());

    const uint32_t tls_cipher_aes_gcm_128_key_size = 16;
    uint32_t key_size_bytes = tls_cipher_aes_gcm_128_key_size;
    void* key = new char[key_size_bytes];

    memcpy(key, "a6a7ee7abec9c4ce", key_size_bytes);  // Random key for the test.

    dek_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.key_blob = key;
    attr.key_blob_size = key_size_bytes;
    attr.pd_id = ad->get_pd();
    attr.key_size = key_size_bytes;

    ret = _dek.create(attr);
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
TEST_F(dpcp_tls_dek, ti_03_destroy)
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

    tls_dek _dek(ad->get_ctx());

    const uint32_t tls_cipher_aes_gcm_128_key_size = 16;
    uint32_t key_size_bytes = tls_cipher_aes_gcm_128_key_size;
    std::unique_ptr<char[]> key(new char[key_size_bytes]);

    memcpy(key.get(), "a6a7ee7abec9c4ce", key_size_bytes);  // Random key for the test.

    struct dek_attr attr;
    memset(&attr, 0, sizeof(dek_attr));
    attr.key_blob = key.get();
    attr.key_blob_size = key_size_bytes;
    attr.pd_id = ad->get_pd();
    attr.key_size = key_size_bytes;

    ret = _dek.create(attr);
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
TEST_F(dpcp_tls_dek, ti_04_create)
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

    tls_dek _dek(ad->get_ctx());

    const uint32_t tls_cipher_aes_gcm_128_key_size = 16;
    uint32_t key_size_bytes = tls_cipher_aes_gcm_128_key_size;
    void* key = new char[key_size_bytes];

    memcpy(key, "a6a7ee7abec9c4ce", key_size_bytes);  // Random key for the test.

    struct dek_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.key_blob = key;
    attr.key_blob_size = key_size_bytes;
    attr.key_size = key_size_bytes;
    attr.pd_id = ad->get_pd();

    ret = _dek.create(attr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek.get_key_id();
    log_trace("key_id: 0x%x\n", key_id);

    memset(&attr, 0, sizeof(attr));
    ret = _dek.query(attr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(ad->get_pd(), attr.pd_id);
    ASSERT_EQ(key_size_bytes, attr.key_size);
}

/**
 * @test dpcp_dek.ti_05_modify
 * @brief
 *    Check dek::modify method
 * @details
 *
 */
TEST_F(dpcp_tls_dek, ti_05_modify)
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

    tls_dek* _dek_ptr = nullptr;
    std::unique_ptr<tls_dek> _dek;
    struct dek_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.key_blob = key.get();
    attr.key_blob_size = key_size_128;
    attr.key_size = key_size_128;
    attr.pd_id = ad->get_pd();
    ret = ad->create_tls_dek(attr, _dek_ptr);
    _dek.reset(_dek_ptr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek->get_key_id();
    log_trace("key_id: 0x%x\n", key_id);

    // Modify to a key with the same size.
    dek_attr modify_attr;
    modify_attr.key_blob = key2.get();
    modify_attr.key_blob_size = key_size_128;
    modify_attr.key_size = key_size_128;
    modify_attr.pd_id = ad->get_pd();
    ret = _dek->modify(modify_attr);
    ASSERT_EQ(DPCP_OK, ret);

    ret = ad->sync_crypto_tls();
    ASSERT_EQ(DPCP_OK, ret);

    const uint32_t key_size_256 = 32;
    std::unique_ptr<char[]> key3(new char[key_size_256]);
    memcpy(key3.get(), "a6a7ee7abec9c4cba6a7ee7abec9c4cb", key_size_256);

    // Modify to a key with a different size.
    modify_attr.key_blob = key3.get();
    modify_attr.key_blob_size = key_size_256;
    modify_attr.key_size = key_size_256;
    ret = _dek->modify(modify_attr);
    ASSERT_EQ(DPCP_OK, ret);

    ret = ad->sync_crypto_tls();
    ASSERT_EQ(DPCP_OK, ret);
}

/**
 * @test dpcp_dek.ti_06_modify_query
 * @brief
 *    Check dek::query method
 * @details
 *
 */
TEST_F(dpcp_tls_dek, ti_06_modify_query)
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

    uint64_t opaque = 0x1234567887654321;
    const uint32_t key_size_128 = 16;
    std::unique_ptr<char[]> key(new char[key_size_128]);
    memcpy(key.get(), "a6a7ee7abec9c4ce", key_size_128);  // Random key for the test.

    tls_dek* _dek_ptr = nullptr;
    std::unique_ptr<tls_dek> _dek;
    dek_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.key_blob = key.get();
    attr.key_blob_size = key_size_128;
    attr.key_size = key_size_128;
    attr.pd_id = ad->get_pd();
    attr.opaque = opaque;
    ret = ad->create_tls_dek(attr, _dek_ptr);
    _dek.reset(_dek_ptr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek->get_key_id();
    log_trace("key_id: 0x%x\n", key_id);

    // Verify create params
    dek_attr query_attr;
    ret = _dek->query(query_attr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(ad->get_pd(), query_attr.pd_id);
    ASSERT_EQ(key_size_128, query_attr.key_size);
    ASSERT_EQ(query_attr.opaque, opaque);

    // Modify
    const uint32_t modify_key_size = 32;
    std::unique_ptr<char[]> modify_key(new char[modify_key_size]);
    memcpy(modify_key.get(), "a6a7ee7abec9c4cea6a7ee7abec9c4ce", modify_key_size);  // Random key for the test.
    attr.key_blob_size = modify_key_size;
    attr.key_size = attr.key_blob_size;
    attr.opaque = opaque = 0x876543210abcdef;
    attr.key_blob = modify_key.get();
    ret = _dek->modify(attr);
    ASSERT_EQ(DPCP_OK, ret);

    // Verify modify params
    ret = _dek->query(query_attr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(ad->get_pd(), query_attr.pd_id);
    ASSERT_EQ(modify_key_size, query_attr.key_size);
    ASSERT_EQ(query_attr.opaque, opaque);
}

////////////////////////////////////////////////////////////////////////
// aes xts dek tests.                                                 //
////////////////////////////////////////////////////////////////////////

class dpcp_aes_xts_dek : public dpcp_base {
};

/**
 * @test dpcp_aes_xts_dek.ti_01_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_aes_xts_dek, ti_01_Constructor)
{
    std::unique_ptr<adapter> ad(OpenAdapter());
    ASSERT_NE(nullptr, ad);

    aes_xts_dek _dek(ad->get_ctx());
    uint32_t id = 0;
    status ret = _dek.get_id(id);

    log_trace("ret: %d id: 0x%x\n", ret, id);
    ASSERT_EQ(DPCP_ERR_INVALID_ID, ret);
    ASSERT_EQ(0, id);
}

/**
 * @test dpcp_aes_xts_dek.ti_02_create_128_bit
 * @brief
 *    Check dek::create method with 128 bit key
 * @details
 *
 */
TEST_F(dpcp_aes_xts_dek, ti_02_create_128_bit)
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

    aes_xts_dek _dek(ad->get_ctx());

    const uint32_t tls_cipher_aes_gcm_128_key_size = 16;
    uint32_t key_blob_size_bytes = tls_cipher_aes_gcm_128_key_size * 2;
    void* key = new char[key_blob_size_bytes];

    memcpy(key, "a6a7ee7abec9c4cea6a7ee7abec9c4cea", key_blob_size_bytes);  // Random key for the test.

    dek_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.key_blob = key;
    attr.key_blob_size = key_blob_size_bytes;
    attr.pd_id = ad->get_pd();
    attr.key_size = tls_cipher_aes_gcm_128_key_size;

    ret = _dek.create(attr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek.get_key_id();
    log_trace("key_id: 0x%x\n", key_id);
}

/**
 * @test dpcp_aes_xts_dek.ti_03_create_256_bit
 * @brief
 *    Check dek::create method with 256 bit key
 * @details
 *
 */
TEST_F(dpcp_aes_xts_dek, ti_03_create_256_bit)
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

    aes_xts_dek _dek(ad->get_ctx());

    const uint32_t tls_cipher_aes_gcm_128_key_size = 32;
    uint32_t key_blob_size_bytes = tls_cipher_aes_gcm_128_key_size * 2;
    void* key = new char[key_blob_size_bytes];

    memcpy(key, "a6a7ee7abec9c4cea6a7ee7abec9c4ceaa6a7ee7abec9c4cea6a7ee7abec9c4cea",
           key_blob_size_bytes);  // Random key for the test.

    dek_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.key_blob = key;
    attr.key_blob_size = key_blob_size_bytes;
    attr.pd_id = ad->get_pd();
    attr.key_size = tls_cipher_aes_gcm_128_key_size;

    ret = _dek.create(attr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek.get_key_id();
    log_trace("key_id: 0x%x\n", key_id);
}

/**
 * @test dpcp_aes_xts_dek.ti_04_create_128_bit_and_keytag
 * @brief
 *    Check dek::create method with 128 bit key and keytag
 * @details
 *
 */
TEST_F(dpcp_aes_xts_dek, ti_04_create_128_bit_and_keytag)
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

    aes_xts_dek _dek(ad->get_ctx());

    const uint32_t tls_cipher_aes_gcm_128_key_size = 16;
    uint32_t key_blob_size_bytes = tls_cipher_aes_gcm_128_key_size * 2 + 8;
    void* key = new char[key_blob_size_bytes];

    memcpy(key, "a6a7ee7abec9c4cea6a7ee7abec9c4ceaaaaaaaaa",
           key_blob_size_bytes);  // Random key for the test.

    dek_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.key_blob = key;
    attr.key_blob_size = key_blob_size_bytes;
    attr.pd_id = ad->get_pd();
    attr.key_size = tls_cipher_aes_gcm_128_key_size;

    ret = _dek.create(attr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek.get_key_id();
    log_trace("key_id: 0x%x\n", key_id);
}

/**
 * @test dpcp_aes_xts_dek.ti_05_create_256_bit_and_keytag
 * @brief
 *    Check dek::create method with 256 bit key and keytag
 * @details
 *
 */
TEST_F(dpcp_aes_xts_dek, ti_05_create_256_bit_and_keytag)
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

    aes_xts_dek _dek(ad->get_ctx());

    const uint32_t tls_cipher_aes_gcm_128_key_size = 32;
    uint32_t key_blob_size_bytes = tls_cipher_aes_gcm_128_key_size * 2 + 8;
    void* key = new char[key_blob_size_bytes];

    memcpy(key, "a6a7ee7abec9c4cea6a7ee7abec9c4ceaa6a7ee7abec9c4cea6a7ee7abec9c4ceaaaaaaaaa",
           key_blob_size_bytes);  // Random key for the test.

    dek_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.key_blob = key;
    attr.key_blob_size = key_blob_size_bytes;
    attr.pd_id = ad->get_pd();
    attr.key_size = tls_cipher_aes_gcm_128_key_size;

    ret = _dek.create(attr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek.get_key_id();
    log_trace("key_id: 0x%x\n", key_id);
}

/**
 * @test dpcp_aes_xts_dek.ti_06_destroy
 * @brief
 *    Check dek::destroy method
 * @details
 *
 */
TEST_F(dpcp_aes_xts_dek, ti_06_destroy)
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

    aes_xts_dek _dek(ad->get_ctx());

    const uint32_t aes_txs_cipher_128_key_size = 16;
    uint32_t key_blob_size_bytes = aes_txs_cipher_128_key_size * 2;
    std::unique_ptr<char[]> key_blob(new char[key_blob_size_bytes]);

    memcpy(key_blob.get(), "a6a7ee7abec9c4cea6a7ee7abec9c4ce", key_blob_size_bytes);  // Random key for the test.

    struct dek_attr attr;
    memset(&attr, 0, sizeof(dek_attr));
    attr.key_blob = key_blob.get();
    attr.key_blob_size = key_blob_size_bytes;
    attr.key_size = aes_txs_cipher_128_key_size;
    attr.pd_id = ad->get_pd();

    ret = _dek.create(attr);
    ASSERT_EQ(DPCP_OK, ret);

    ret = _dek.destroy();
    ASSERT_EQ(DPCP_OK, ret);
}

/**
 * @test dpcp_aes_xts_dek.ti_07_create
 * @brief
 *    Check dek::create method
 * @details
 *
 */
TEST_F(dpcp_aes_xts_dek, ti_07_create)
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

    aes_xts_dek _dek(ad->get_ctx());

    const uint32_t aes_xts_cipher_128_key_size = 16;
    uint32_t key_blob_size_bytes = aes_xts_cipher_128_key_size * 2 + 8;
    std::unique_ptr<char[]> key_blob(new char[key_blob_size_bytes]);

    memcpy(key_blob.get(), "a6a7ee7abec9c4cea6a7ee7abec9c4ceaaaaaaaa", key_blob_size_bytes);  // Random key for the test.

    struct dek_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.key_blob = key_blob.get();
    attr.key_blob_size = key_blob_size_bytes;
    attr.key_size = aes_xts_cipher_128_key_size;
    attr.pd_id = ad->get_pd();

    ret = _dek.create(attr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek.get_key_id();
    log_trace("key_id: 0x%x\n", key_id);

    memset(&attr, 0, sizeof(attr));
    ret = _dek.query(attr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(ad->get_pd(), attr.pd_id);
    ASSERT_EQ(aes_xts_cipher_128_key_size, attr.key_size);
}

/**
 * @test dpcp_aes_xts_dek.ti_08_modify
 * @brief
 *    Check dek::modify method
 * @details
 *
 */
TEST_F(dpcp_aes_xts_dek, ti_08_modify)
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
    const uint32_t key_blob_size = key_size_128 * 2;
    std::unique_ptr<char[]> key(new char[key_blob_size]);
    std::unique_ptr<char[]> key2(new char[key_blob_size]);

    memcpy(key.get(), "a6a7ee7abec9c4cea6a7ee7abec9c4ce", key_blob_size);  // Random key for the test.
    memcpy(key2.get(), "a6a7ee7abec9c4cba6a7ee7abec9c4cb", key_blob_size);  // Random key2 for the test.

    aes_xts_dek* _dek_ptr = nullptr;
    std::unique_ptr<aes_xts_dek> _dek;
    struct dek_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.key_blob = key.get();
    attr.key_blob_size = key_blob_size;
    attr.key_size = key_size_128;
    attr.pd_id = ad->get_pd();
    ret = ad->create_aes_txs_dek(attr, _dek_ptr);
    _dek.reset(_dek_ptr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek->get_key_id();
    log_trace("key_id: 0x%x\n", key_id);

    // Modify to a key with the same size.
    dek_attr modify_attr;
    modify_attr.key_blob = key2.get();
    modify_attr.key_blob_size = key_blob_size;
    modify_attr.key_size = key_size_128;
    modify_attr.pd_id = ad->get_pd();
    ret = _dek->modify(modify_attr);
    ASSERT_EQ(DPCP_OK, ret);

    ret = ad->sync_crypto_tls();
    ASSERT_EQ(DPCP_OK, ret);

    const uint32_t key_size_256 = 32;
    const uint32_t key_blob_size_256 = key_size_256 *2;
    std::unique_ptr<char[]> key3(new char[key_blob_size_256]);
    memcpy(key3.get(), "a6a7ee7abec9c4cba6a7ee7abec9c4cba6a7ee7abec9c4cba6a7ee7abec9c4cb", key_blob_size_256);

    // Modify to a key with a different size.
    modify_attr.key_blob = key3.get();
    modify_attr.key_blob_size = key_blob_size_256;
    modify_attr.key_size = key_size_256;
    ret = _dek->modify(modify_attr);
    ASSERT_EQ(DPCP_OK, ret);

    ret = ad->sync_crypto_tls();
    ASSERT_EQ(DPCP_OK, ret);
}

/**
 * @test dpcp_aes_xts_dek.ti_09_modify_query
 * @brief
 *    Check dek::query method
 * @details
 *
 */
TEST_F(dpcp_aes_xts_dek, ti_09_modify_query)
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

    uint64_t opaque = 0x1234567887654321;
    const uint32_t key_size_128 = 16;
    const uint32_t key_blob_size = 2 * key_size_128;
    std::unique_ptr<char[]> key(new char[key_blob_size]);
    memcpy(key.get(), "a6a7ee7abec9c4cea6a7ee7abec9c4ce", key_blob_size);  // Random key for the test.

    aes_xts_dek* _dek_ptr = nullptr;
    std::unique_ptr<aes_xts_dek> _dek;
    dek_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.key_blob = key.get();
    attr.key_blob_size = key_blob_size;
    attr.key_size = key_size_128;
    attr.pd_id = ad->get_pd();
    attr.opaque = opaque;
    ret = ad->create_aes_txs_dek(attr, _dek_ptr);
    _dek.reset(_dek_ptr);
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t key_id = _dek->get_key_id();
    log_trace("key_id: 0x%x\n", key_id);

    // Verify create params
    dek_attr query_attr;
    ret = _dek->query(query_attr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(ad->get_pd(), query_attr.pd_id);
    ASSERT_EQ(key_size_128, query_attr.key_size);
    ASSERT_EQ(query_attr.opaque, opaque);

    // Modify
    const uint32_t modify_key_size = 32;
    attr.key_blob_size = modify_key_size * 2;
    std::unique_ptr<char[]> modify_key(new char[key_blob_size]);
    memcpy(modify_key.get(),
           "a6a7ee7abec9c4cea6a7ee7abec9c4cea6a7ee7abec9c4cea6a7ee7abec9c4ce",
           key_blob_size);
    attr.key_size = modify_key_size;
    attr.opaque = opaque = 0x876543210abcdef;
    attr.key_blob = modify_key.get();
    ret = _dek->modify(attr);
    ASSERT_EQ(DPCP_OK, ret);

    // Verify modify params
    ret = _dek->query(query_attr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(ad->get_pd(), query_attr.pd_id);
    ASSERT_EQ(modify_key_size, query_attr.key_size);
    ASSERT_EQ(query_attr.opaque, opaque);
}
