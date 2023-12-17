/*
 * Copyright (c) 2021-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

tag_buffer_table_obj::tag_buffer_table_obj(dcmd::ctx* ctx)
    : obj(ctx)
    , m_key_id(0)
{
}

tag_buffer_table_obj::~tag_buffer_table_obj()
{
}

status tag_buffer_table_obj::create(const tag_buffer_table_obj::attr& tag_buffer_table_obj_attr)
{
    uint32_t in[DEVX_ST_SZ_DW(create_tag_buffer_table_obj_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(general_obj_out_cmd_hdr)] = {0};
    size_t outlen = sizeof(out);
    void* _obj = DEVX_ADDR_OF(create_tag_buffer_table_obj_in, in, tag_buffer_table_object);

    DEVX_SET(general_obj_in_cmd_hdr, in, opcode, MLX5_CMD_OP_CREATE_GENERAL_OBJECT);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_type,
             MLX5_GENERAL_OBJECT_TYPES_NVMEOTCP_TAG_BUFFER_TABLE);
    DEVX_SET(tag_buffer_table_obj, _obj, log_tag_buffer_table_size,
             tag_buffer_table_obj_attr.log_tag_buffer_table_size);

    status ret = obj::create(in, sizeof(in), out, outlen);
    if (DPCP_OK == ret) {
        m_key_id = DEVX_GET(general_obj_out_cmd_hdr, out, obj_id);
        log_trace("Tag Buffer Table Object key_id: 0x%x created\n", m_key_id);
    }

    return ret;
}

status tag_buffer_table_obj::query(tag_buffer_table_obj::attr& tag_buffer_table_obj_attr)
{
    status ret = DPCP_OK;
    uint32_t in[DEVX_ST_SZ_DW(general_obj_in_cmd_hdr)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(query_tag_buffer_table_obj_out)] = {0};
    size_t outlen = sizeof(out);
    uintptr_t handle;

    if (DPCP_OK != get_handle(handle)) {
        log_error("Tag Buffer Table is invalid\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    memset(&tag_buffer_table_obj_attr, 0, sizeof(tag_buffer_table_obj_attr));

    DEVX_SET(general_obj_in_cmd_hdr, in, opcode, MLX5_CMD_OP_QUERY_GENERAL_OBJECT);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_type,
             MLX5_GENERAL_OBJECT_TYPES_NVMEOTCP_TAG_BUFFER_TABLE);
    DEVX_SET(general_obj_in_cmd_hdr, in, obj_id, m_key_id);

    ret = obj::query(in, sizeof(in), out, outlen);
    if (DPCP_OK != ret) {
        log_warn("Tag Buffer Table query failed");
        return DPCP_ERR_QUERY;
    }

    void* tag_buf_ctx = DEVX_ADDR_OF(query_tag_buffer_table_obj_out, out, tag_buffer_table_object);
    tag_buffer_table_obj_attr.log_tag_buffer_table_size =
        DEVX_GET(tag_buffer_table_obj, tag_buf_ctx, log_tag_buffer_table_size);

    log_trace("Tag Buffer Table attr:\n");
    log_trace("          modify_field_select=0x%x\n",
              tag_buffer_table_obj_attr.modify_field_select);
    log_trace("          log_tag_buffer_table_size=0x%x\n",
              tag_buffer_table_obj_attr.log_tag_buffer_table_size);

    return DPCP_OK;
}

} // namespace dpcp
