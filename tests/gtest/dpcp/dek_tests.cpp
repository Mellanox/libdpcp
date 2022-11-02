/*
 * Copyright (c) 2021-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

    struct dek::attr dek_attr;
    memset(&dek_attr, 0, sizeof(dek_attr));
    dek_attr.flags = DEK_ATTR_TLS;
    dek_attr.key = key;
    dek_attr.key_size_bytes = key_size_bytes;
    dek_attr.pd_id = ad->get_pd();

    ret = _dek.create(dek_attr);
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

    struct dek::attr dek_attr;
    memset(&dek_attr, 0, sizeof(dek_attr));
    dek_attr.flags = DEK_ATTR_TLS;
    dek_attr.key = key.get();
    dek_attr.key_size_bytes = key_size_bytes;
    dek_attr.pd_id = ad->get_pd();

    ret = _dek.create(dek_attr);
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

    struct dek::attr dek_attr;
    memset(&dek_attr, 0, sizeof(dek_attr));
    dek_attr.flags = DEK_ATTR_TLS;
    dek_attr.key = key;
    dek_attr.key_size_bytes = key_size_bytes;
    dek_attr.pd_id = ad->get_pd();

    ret = _dek.create(dek_attr);
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
    struct dek::attr dek_attr;
    memset(&dek_attr, 0, sizeof(dek_attr));
    dek_attr.flags = DEK_ATTR_TLS;
    dek_attr.key = key.get();
    dek_attr.key_size_bytes = key_size_128;
    dek_attr.pd_id = ad->get_td();
    ret = ad->create_dek(dek_attr, _dek_ptr);
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
    struct dek::attr dek_attr;
    memset(&dek_attr, 0, sizeof(dek_attr));
    dek_attr.flags = DEK_ATTR_TLS;
    dek_attr.key = key.get();
    dek_attr.key_size_bytes = key_size_128;
    dek_attr.pd_id = ad->get_td();
    ret = ad->create_dek(dek_attr, _dek_ptr);
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
