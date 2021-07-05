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

status dek::create(const uint32_t pd_id, void* key, const uint32_t key_size_bytes)
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
        return DPCP_OK;
    } else {
        return DPCP_ERR_CREATE;
    }
}

} // namespace dpcp
