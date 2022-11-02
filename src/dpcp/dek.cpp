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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "utils/os.h"
#include "dpcp/internal.h"

namespace dpcp {

constexpr uint8_t BITS_PER_BYTE = 8;

dek::dek(dcmd::ctx* ctx)
    : obj(ctx)
    , m_key_id(0)
{
}

dek::~dek()
{
}

status dek::create(const dek::attr& dek_attr)
{
    if (dek_attr.pd_id == 0) {
        log_error("Protection Domain is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (dek_attr.key == nullptr) {
        log_error("Key is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (dek_attr.key_size_bytes == 0) {
        log_error("Key size is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    uint32_t in[DEVX_ST_SZ_DW(create_encryption_key_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(general_obj_out_cmd_hdr)] = {0};
    size_t outlen = sizeof(out);
    uint32_t key_size_bits = dek_attr.key_size_bytes * BITS_PER_BYTE;
    uint8_t general_obj_key_size;
    void *_obj, *key_p;

    _obj = DEVX_ADDR_OF(create_encryption_key_in, in, encryption_key_object);
    key_p = DEVX_ADDR_OF(encryption_key_obj, _obj, key);

    switch (key_size_bits) {
    case 128:
        general_obj_key_size = MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_128;
        // For 128 bit key, padding should be added in the beginning, see PRM 8.27.17 DEK Object
        // section.
        key_p = static_cast<char*>(key_p) + dek_attr.key_size_bytes;
        break;
    case 256:
        general_obj_key_size = MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_256;
        break;
    default:
        return DPCP_ERR_INVALID_PARAM;
    }

    if (dek_attr.flags & DEK_ATTR_TLS) {
        memcpy(key_p, dek_attr.key, dek_attr.key_size_bytes);
        DEVX_SET(encryption_key_obj, _obj, key_size, general_obj_key_size);
        DEVX_SET(encryption_key_obj, _obj, key_type,
                 MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_TYPE_TLS);
        DEVX_SET(general_obj_in_cmd_hdr, in, opcode, MLX5_CMD_OP_CREATE_GENERAL_OBJECT);
        DEVX_SET(general_obj_in_cmd_hdr, in, obj_type, MLX5_GENERAL_OBJECT_TYPES_ENCRYPTION_KEY);
        DEVX_SET(encryption_key_obj, _obj, pd, dek_attr.pd_id);
    }

    status ret = obj::create(in, sizeof(in), out, outlen);
    if (DPCP_OK == ret) {
        m_key_id = DEVX_GET(general_obj_out_cmd_hdr, out, obj_id);
        log_trace("DEK key_id: 0x%x created\n", m_key_id);
    }

    return ret;
}

status dek::modify(const dek::attr& dek_attr)
{
    status ret = DPCP_OK;
    uint32_t in[DEVX_ST_SZ_DW(create_encryption_key_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(general_obj_out_cmd_hdr)] = {0};
    size_t outlen = sizeof(out);
    uint8_t general_obj_key_size = 0;
    uintptr_t handle;

    if (DPCP_OK != get_handle(handle)) {
        log_error("DEK is invalid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (dek_attr.key == nullptr) {
        log_error("Key is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (dek_attr.key_size_bytes == 0) {
        log_error("Key size is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    void* _obj = DEVX_ADDR_OF(create_encryption_key_in, in, encryption_key_object);
    void* key_p = DEVX_ADDR_OF(encryption_key_obj, _obj, key);

    switch (dek_attr.key_size_bytes * BITS_PER_BYTE) {
    case 128:
        general_obj_key_size = MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_128;
        // For 128 bit key, padding should be added in the beginning, see PRM 8.27.17 DEK Object
        // section.
        key_p = static_cast<char*>(key_p) + dek_attr.key_size_bytes;
        break;
    case 256:
        general_obj_key_size = MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_256;
        break;
    default:
        return DPCP_ERR_INVALID_PARAM;
    }

    if (dek_attr.flags & DEK_ATTR_TLS) {
        uint64_t modify_select = 0x1;

        memcpy(key_p, dek_attr.key, dek_attr.key_size_bytes);
        DEVX_SET64(encryption_key_obj, _obj, modify_field_select, modify_select);
        DEVX_SET(encryption_key_obj, _obj, key_size, general_obj_key_size);
        DEVX_SET(encryption_key_obj, _obj, key_type,
                 MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_TYPE_TLS);
        DEVX_SET(general_obj_in_cmd_hdr, in, opcode, MLX5_CMD_OP_MODIFY_GENERAL_OBJECT);
        DEVX_SET(general_obj_in_cmd_hdr, in, obj_type, MLX5_GENERAL_OBJECT_TYPES_ENCRYPTION_KEY);
        DEVX_SET(general_obj_in_cmd_hdr, in, obj_id, m_key_id);
        DEVX_SET(encryption_key_obj, _obj, pd, dek_attr.pd_id);
    }

    ret = obj::modify(in, sizeof(in), out, outlen);
    if (DPCP_OK == ret) {
        log_trace("DEK key_id: 0x%x modified\n", m_key_id);
    }

    return ret;
}

status dek::query(dek::attr& dek_attr)
{
    status ret = DPCP_OK;
    uint32_t in[DEVX_ST_SZ_DW(general_obj_in_cmd_hdr)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(query_encryption_key_out)] = {0};
    size_t outlen = sizeof(out);
    uintptr_t handle;

    if (DPCP_OK != get_handle(handle)) {
        log_error("DEK is invalid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    memset(&dek_attr, 0, sizeof(dek_attr));

    DEVX_SET(general_obj_in_cmd_hdr, in, opcode, MLX5_CMD_OP_QUERY_GENERAL_OBJECT);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_type, MLX5_GENERAL_OBJECT_TYPES_ENCRYPTION_KEY);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_id, m_key_id);

    ret = obj::query(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        log_warn("DEK query failed");
        return DPCP_ERR_QUERY;
    }

    dek_attr.flags |= DEK_ATTR_TLS;
    dek_attr.key_size_bytes = DEVX_GET(encryption_key_obj, out, key_size);
    dek_attr.pd_id = DEVX_GET(encryption_key_obj, out, pd);

    if (dek_attr.key) {
        void* key_p = DEVX_ADDR_OF(encryption_key_obj, out, key);
        if (dek_attr.key_size_bytes == 128) {
            key_p = static_cast<char*>(key_p) + dek_attr.key_size_bytes;
        }
        memcpy(dek_attr.key, key_p, dek_attr.key_size_bytes);
    }

    log_trace("DEK attr:\n");
    log_trace("          key_size=0x%x\n", dek_attr.key_size_bytes);
    log_trace("          pd=0x%x\n", dek_attr.pd_id);
    log_trace("          key_type=0x%x\n", m_key_id);

    return DPCP_OK;
}

} // namespace dpcp
