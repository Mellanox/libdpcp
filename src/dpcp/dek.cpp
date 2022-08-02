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

status dek::create(const uint32_t pd_id, const void* key, const uint32_t key_size_bytes)
{
    if (pd_id == 0) {
        log_error("Protection Domain is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (key == nullptr) {
        log_error("Key is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    if (key_size_bytes == 0) {
        log_error("Key size is not set");
        return DPCP_ERR_INVALID_PARAM;
    }

    uint32_t in[DEVX_ST_SZ_DW(create_encryption_key_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(general_obj_out_cmd_hdr)] = {0};
    size_t outlen = sizeof(out);
    uint32_t key_size_bits = key_size_bytes * BITS_PER_BYTE;
    uint8_t general_obj_key_size;
    void *_obj, *key_p;

    _obj = DEVX_ADDR_OF(create_encryption_key_in, in, encryption_key_object);
    key_p = DEVX_ADDR_OF(encryption_key_obj, _obj, key);

    switch (key_size_bits) {
    case 128:
        general_obj_key_size = MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_128;
        // For 128 bit key, padding should be added in the beginning, see PRM 8.27.17 DEK Object
        // section.
        key_p = static_cast<char*>(key_p) + key_size_bytes;
        break;
    case 256:
        general_obj_key_size = MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_KEY_SIZE_256;
        break;
    default:
        return DPCP_ERR_INVALID_PARAM;
    }

    memcpy(key_p, key, key_size_bytes);
    DEVX_SET(encryption_key_obj, _obj, key_size, general_obj_key_size);
    DEVX_SET(encryption_key_obj, _obj, key_type, MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_TYPE_TLS);
    DEVX_SET(general_obj_in_cmd_hdr, in, opcode, MLX5_CMD_OP_CREATE_GENERAL_OBJECT);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_type, MLX5_GENERAL_OBJECT_TYPES_ENCRYPTION_KEY);
    DEVX_SET(encryption_key_obj, _obj, pd, pd_id);

    status ret = obj::create(in, sizeof(in), out, outlen);
    if (ret == DPCP_OK) {
        m_key_id = DEVX_GET(general_obj_out_cmd_hdr, out, obj_id);
        m_pd_id = pd_id;
        return DPCP_OK;
    } else {
        log_warn("DEK create failed");
        return DPCP_ERR_CREATE;
    }
}

status dek::create(const dek::attr& dek_attr)
{
    return create(dek_attr.pd_id, dek_attr.key, dek_attr.key_size_bytes);
}

status dek::modify(const dek::attr& dek_attr)
{
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
    uint8_t general_obj_key_size = 0;

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

    uint64_t modify_select = 0x1;

    memcpy(key_p, dek_attr.key, dek_attr.key_size_bytes);
    DEVX_SET64(encryption_key_obj, _obj, modify_field_select, modify_select);
    DEVX_SET(encryption_key_obj, _obj, key_size, general_obj_key_size);
    DEVX_SET(encryption_key_obj, _obj, key_type, MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_TYPE_TLS);
    DEVX_SET(general_obj_in_cmd_hdr, in, opcode, MLX5_CMD_OP_MODIFY_GENERAL_OBJECT);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_type, MLX5_GENERAL_OBJECT_TYPES_ENCRYPTION_KEY);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_id, m_key_id);
    DEVX_SET(encryption_key_obj, _obj, pd, m_pd_id);

    size_t out_sz = sizeof(out);
    return obj::modify(in, sizeof(in), out, out_sz);
}

status dek::query(dek::attr& dek_attr)
{
    uint32_t in[DEVX_ST_SZ_DW(general_obj_in_cmd_hdr)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(query_encryption_key_out)] = {0};

    DEVX_SET(general_obj_in_cmd_hdr, in, opcode, MLX5_CMD_OP_QUERY_GENERAL_OBJECT);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_type, MLX5_GENERAL_OBJECT_TYPES_ENCRYPTION_KEY);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_id, m_key_id);

    size_t out_sz = sizeof(out);
    status rc = obj::query(in, sizeof(in), out, out_sz);
    if (DPCP_OK != rc) {
        log_warn("DEK query failed");
        return DPCP_ERR_QUERY;
    }

    dek_attr.key_size_bytes = DEVX_GET(encryption_key_obj, out, key_size);
    dek_attr.pd_id = DEVX_GET(encryption_key_obj, out, pd);

    if (dek_attr.key) {
        void* key_p = DEVX_ADDR_OF(encryption_key_obj, out, key);
        if (dek_attr.key_size_bytes == 128) {
            key_p = static_cast<char*>(key_p) + dek_attr.key_size_bytes;
        }

        memcpy(dek_attr.key, key_p, dek_attr.key_size_bytes);
    }

    return rc;
}

} // namespace dpcp
