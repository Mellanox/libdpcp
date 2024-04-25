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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "utils/os.h"
#include "dpcp/internal.h"

namespace dpcp {

static constexpr uint32_t KEY_128_BIT_SIZE_IN_BYTES = 16;
static constexpr uint32_t KEY_256_BIT_SIZE_IN_BYTES = 32;

////////////////////////////////////////////////////////////////////////
// dek implementation.                                                //
////////////////////////////////////////////////////////////////////////

dek::dek(dcmd::ctx* ctx)
    : obj(ctx)
    , m_key_id(0)
{
}

status dek::create(const dek_attr& attr)
{
    status ret = verify_attr(attr);
    if (DPCP_OK != ret) {
        log_error("DEK failed to verify attributes");
        return ret;
    }

    key_params params;
    ret = get_key_params(attr.key_blob_size, attr.key_size, params);
    if (DPCP_OK != ret) {
        log_error("DEK failed to get key params, status %d", ret);
        return ret;
    }

    uint32_t in[DEVX_ST_SZ_DW(create_encryption_key_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(general_obj_out_cmd_hdr)] = {0};
    size_t outlen = sizeof(out);

    void* _obj = DEVX_ADDR_OF(create_encryption_key_in, in, encryption_key_object);
    void* key_p = DEVX_ADDR_OF(encryption_key_obj, _obj, key);
    key_p = static_cast<char*>(key_p) + params.offset;

    memcpy(key_p, attr.key_blob, attr.key_blob_size);
    DEVX_SET(encryption_key_obj, _obj, key_size, params.size);
    DEVX_SET(encryption_key_obj, _obj, has_keytag, params.has_keytag);
    DEVX_SET(encryption_key_obj, _obj, key_type, params.type);
    DEVX_SET(general_obj_in_cmd_hdr, in, opcode, MLX5_CMD_OP_CREATE_GENERAL_OBJECT);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_type, MLX5_GENERAL_OBJECT_TYPES_ENCRYPTION_KEY);
    DEVX_SET(encryption_key_obj, _obj, pd, attr.pd_id);
    DEVX_SET64(encryption_key_obj, _obj, opaque, attr.opaque);

    ret = obj::create(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        log_error("DEK failed to create HW object, status %d", ret);
        return ret;
    }

    m_key_id = DEVX_GET(general_obj_out_cmd_hdr, out, obj_id);
    log_trace("DEK key_id: 0x%x created\n", m_key_id);
    return DPCP_OK;
}

status dek::modify(const dek_attr& attr)
{
    status ret = verify_attr(attr);
    if (DPCP_OK != ret) {
        log_error("DEK failed to verify attributes");
        return ret;
    }

    uintptr_t handle;
    if (DPCP_OK != get_handle(handle)) {
        log_error("DEK is invalid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    key_params params;
    ret = get_key_params(attr.key_blob_size, attr.key_size, params);
    if (DPCP_OK != ret) {
        log_error("DEK failed to get key params, status %d", ret);
        return ret;
    }

    uint32_t in[DEVX_ST_SZ_DW(create_encryption_key_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(general_obj_out_cmd_hdr)] = {0};
    size_t outlen = sizeof(out);

    void* _obj = DEVX_ADDR_OF(create_encryption_key_in, in, encryption_key_object);
    void* key_p = DEVX_ADDR_OF(encryption_key_obj, _obj, key);
    key_p = static_cast<char*>(key_p) + params.offset;

    uint64_t modify_select = 0x1;
    memcpy(key_p, attr.key_blob, attr.key_blob_size);
    DEVX_SET64(encryption_key_obj, _obj, modify_field_select, modify_select);
    DEVX_SET(encryption_key_obj, _obj, key_size, params.size);
    DEVX_SET(encryption_key_obj, _obj, key_type, params.type); // From PRM - can't be changed
    DEVX_SET(general_obj_in_cmd_hdr, in, opcode, MLX5_CMD_OP_MODIFY_GENERAL_OBJECT);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_type, MLX5_GENERAL_OBJECT_TYPES_ENCRYPTION_KEY);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_id, m_key_id);
    DEVX_SET(encryption_key_obj, _obj, pd, attr.pd_id); // From PRM - can't be changed
    DEVX_SET64(encryption_key_obj, _obj, opaque, attr.opaque);

    ret = obj::modify(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        log_error("DEK failed to modify HW object, status %d", ret);
        return ret;
    }

    log_trace("DEK key_id: 0x%x modified\n", m_key_id);
    return DPCP_OK;
}

status dek::query(dek_attr& attr)
{
    status ret = DPCP_OK;

    memset(&attr, 0, sizeof(attr));

    uintptr_t handle;
    if (DPCP_OK != get_handle(handle)) {
        log_error("DEK is invalid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    uint32_t in[DEVX_ST_SZ_DW(general_obj_in_cmd_hdr)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(query_encryption_key_out)] = {0};
    size_t outlen = sizeof(out);

    DEVX_SET(general_obj_in_cmd_hdr, in, opcode, MLX5_CMD_OP_QUERY_GENERAL_OBJECT);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_type, MLX5_GENERAL_OBJECT_TYPES_ENCRYPTION_KEY);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_id, m_key_id);

    ret = obj::query(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        log_warn("DEK query failed, status %d", ret);
        return DPCP_ERR_QUERY;
    }

    void* dek_ctx = DEVX_ADDR_OF(query_encryption_key_out, out, encryption_key_object);
    attr.key_size = key_size_flag_to_bytes_size(DEVX_GET(encryption_key_obj, dek_ctx, key_size));
    attr.pd_id = DEVX_GET(encryption_key_obj, dek_ctx, pd);
    attr.opaque = DEVX_GET64(encryption_key_obj, dek_ctx, opaque);
    const uint8_t key_type = DEVX_GET(encryption_key_obj, dek_ctx, key_type);
    const uint8_t has_keytag = DEVX_GET(encryption_key_obj, dek_ctx, has_keytag);
    // attr.key returned is 0 by FW since it is encrypted

    log_trace("DEK attr:\n");
    log_trace("          key_size=0x%x\n", attr.key_size);
    log_trace("          pd=0x%x\n", attr.pd_id);
    log_trace("          key id=0x%x\n", m_key_id);
    log_trace("          key_type=0x%x\n", key_type);
    log_trace("          has_keytag=0x%x\n", has_keytag);
    return DPCP_OK;
}

status dek::get_key_params(const uint32_t key_blob_size, const uint32_t key_size,
                           dek::key_params& params) const
{
    params.has_keytag = false;
    params.type = get_key_type();

    if (key_blob_size != key_size) {
        log_error("DEK key blob size should be equal to key size, key type %d", params.type);
        return DPCP_ERR_INVALID_PARAM;
    }

    switch (key_size) {
    case KEY_128_BIT_SIZE_IN_BYTES:
        params.size = MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_128;
        params.offset = key_size;
        break;
    case KEY_256_BIT_SIZE_IN_BYTES:
        params.size = MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_256;
        params.offset = 0;
        break;
    default:
        log_error("Unknown key size");
        return DPCP_ERR_INVALID_PARAM;
    }

    return DPCP_OK;
}

uint32_t dek::key_size_flag_to_bytes_size(const uint8_t size_flag)
{
    switch (size_flag) {
    case MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_128:
        return KEY_128_BIT_SIZE_IN_BYTES;
    case MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_256:
        return KEY_256_BIT_SIZE_IN_BYTES;
    default:
        log_error("Unknown key size flag");
        return 0;
    }
}

status dek::verify_attr(const dpcp::dek_attr& attr)
{
    if (attr.pd_id == 0) {
        log_error("Protection Domain is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (attr.key_blob == nullptr) {
        log_error("Key is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (attr.key_blob_size == 0) {
        log_error("Key size is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (attr.key_size == 0) {
        log_error("Key size is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    return DPCP_OK;
}

////////////////////////////////////////////////////////////////////////
// aes_xts_dek implementation.                                        //
////////////////////////////////////////////////////////////////////////

aes_xts_dek::aes_xts_dek(dcmd::ctx* ctx)
    : dek(ctx)
{
}

uint8_t aes_xts_dek::get_key_type() const
{
    return MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_TYPE_AES_XTS;
}

status aes_xts_dek::get_key_params(const uint32_t key_blob_size, const uint32_t key_size,
                                   dek::key_params& params) const
{
    params.type = get_key_type();
    params.offset = 0;

    static constexpr uint32_t KEY_BLOB_SIZE_128_BIT_KEY_NO_KEYTAG =
        key_size_to_blob_size(KEY_128_BIT_SIZE_IN_BYTES, false);
    static constexpr uint32_t KEY_BLOB_SIZE_256_BIT_KEY_NO_KEYTAG =
        key_size_to_blob_size(KEY_256_BIT_SIZE_IN_BYTES, false);
    static constexpr uint32_t KEY_BLOB_SIZE_128_BIT_KEY_WITH_KEYTAG =
        key_size_to_blob_size(KEY_128_BIT_SIZE_IN_BYTES, true);
    static constexpr uint32_t KEY_BLOB_SIZE_256_BIT_KEY_WITH_KEYTAG =
        key_size_to_blob_size(KEY_256_BIT_SIZE_IN_BYTES, true);

    switch (key_blob_size) {
    case KEY_BLOB_SIZE_128_BIT_KEY_NO_KEYTAG:
        params.size = MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_128;
        params.has_keytag = false;
        break;
    case KEY_BLOB_SIZE_128_BIT_KEY_WITH_KEYTAG:
        params.size = MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_128;
        params.has_keytag = true;
        break;
    case KEY_BLOB_SIZE_256_BIT_KEY_NO_KEYTAG:
        params.size = MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_256;
        params.has_keytag = false;
        break;
    case KEY_BLOB_SIZE_256_BIT_KEY_WITH_KEYTAG:
        params.size = MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_256;
        params.has_keytag = true;
        break;
    default:
        log_error("invalid key blob size");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (key_size_to_blob_size(key_size, params.has_keytag) != key_blob_size) {
        log_error("invalid key size for provided key blob");
        return DPCP_ERR_INVALID_PARAM;
    }

    return DPCP_OK;
}

////////////////////////////////////////////////////////////////////////
// tls_dek implementation.                                            //
////////////////////////////////////////////////////////////////////////

tls_dek::tls_dek(dcmd::ctx* ctx)
    : dek(ctx)
{
}

uint8_t tls_dek::get_key_type() const
{
    return MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_TYPE_TLS;
}

////////////////////////////////////////////////////////////////////////
// ipsec_dek implementation.                                          //
////////////////////////////////////////////////////////////////////////

ipsec_dek::ipsec_dek(dcmd::ctx* ctx)
    : dek(ctx)
{
}

uint8_t ipsec_dek::get_key_type() const
{
    return MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_TYPE_IPSEC;
}

} // namespace dpcp
