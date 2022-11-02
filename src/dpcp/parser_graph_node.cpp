/*
 * Copyright (c) 2020-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

parser_graph_node::parser_graph_node(dcmd::ctx* ctx, const parser_graph_node_attr& attrs)
    : obj(ctx)
    , m_attrs(attrs)
    , m_parser_graph_node_id(0)
{
}

parser_graph_node::~parser_graph_node()
{
}

status parser_graph_node::create()
{
    uint32_t in[DEVX_ST_SZ_DW(create_parse_graph_node_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(general_obj_out_cmd_hdr)] = {0};
    size_t outlen = sizeof(out);
    void* hdr = DEVX_ADDR_OF(create_parse_graph_node_in, in, hdr);
    void* node = DEVX_ADDR_OF(create_parse_graph_node_in, in, node);
    void* sample = DEVX_ADDR_OF(parse_graph_node, node, flow_match_sample);
    void* in_arc = DEVX_ADDR_OF(parse_graph_node, node, input_arc);

    DEVX_SET(general_obj_in_cmd_hdr, hdr, opcode, MLX5_CMD_OP_CREATE_GENERAL_OBJECT);
    DEVX_SET(general_obj_in_cmd_hdr, hdr, obj_type, MLX5_GENERAL_OBJECT_TYPES_PARSE_GRAPH_NODE);
    DEVX_SET(parse_graph_node, node, header_length_mode, m_attrs.header_length_mode.to_ulong());
    DEVX_SET(parse_graph_node, node, header_length_base_value, m_attrs.header_length_base_value);
    DEVX_SET(parse_graph_node, node, header_length_field_offset,
             m_attrs.header_length_field_offset);
    DEVX_SET(parse_graph_node, node, header_length_field_shift,
             m_attrs.header_length_field_shift.to_ulong());
    DEVX_SET(parse_graph_node, node, header_length_field_mask, m_attrs.header_length_field_mask);

    for (size_t index = 0; index < m_attrs.samples.size(); index++) {
        auto _sample = m_attrs.samples[index];
        if (!_sample.enabled) {
            continue;
        }

        void* sample_offset =
            reinterpret_cast<void*>(reinterpret_cast<char*>(sample) +
                                    index * DEVX_ST_SZ_BYTES(parse_graph_flow_match_sample));
        DEVX_SET(parse_graph_flow_match_sample, sample_offset, flow_match_sample_en,
                 !!_sample.enabled);
        DEVX_SET(parse_graph_flow_match_sample, sample_offset, flow_match_sample_field_offset,
                 _sample.field_offset);
        DEVX_SET(parse_graph_flow_match_sample, sample_offset, flow_match_sample_offset_mode,
                 _sample.offset_mode.to_ulong());
        DEVX_SET(parse_graph_flow_match_sample, sample_offset, flow_match_sample_field_offset_mask,
                 _sample.field_offset_mask);
        DEVX_SET(parse_graph_flow_match_sample, sample_offset, flow_match_sample_field_offset_shift,
                 _sample.field_offset_shift.to_ulong());
        DEVX_SET(parse_graph_flow_match_sample, sample_offset, flow_match_sample_field_base_offset,
                 _sample.field_base_offset);
        DEVX_SET(parse_graph_flow_match_sample, sample_offset, flow_match_sample_tunnel_mode,
                 _sample.tunnel_mode.to_ulong());
    }

    // Supporting 1 input arc now, using vector to be able to extend in the future
    // without breaking backward computability.
    constexpr int first_arc = 0;
    auto* _in_arc = &m_attrs.in_arcs[first_arc];
    if (_in_arc->arc_parse_graph_node != 0) {
        DEVX_SET(parse_graph_arc, in_arc, compare_condition_value,
                 _in_arc->compare_condition_value);
        DEVX_SET(parse_graph_arc, in_arc, start_inner_tunnel, _in_arc->start_inner_tunnel);
        DEVX_SET(parse_graph_arc, in_arc, arc_parse_graph_node, _in_arc->arc_parse_graph_node);
        DEVX_SET(parse_graph_arc, in_arc, parse_graph_node_handle,
                 _in_arc->parse_graph_node_handle);
    }

    status ret = obj::create(in, sizeof(in), out, outlen);
    if (ret != DPCP_OK) {
        log_error("Failed to create parser graph node");
        return DPCP_ERR_CREATE;
    }
    m_parser_graph_node_id = DEVX_GET(general_obj_out_cmd_hdr, out, obj_id);

    return DPCP_OK;
}

status parser_graph_node::query()
{
    uint32_t in[DEVX_ST_SZ_DW(general_obj_in_cmd_hdr)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(create_parse_graph_node_out)] = {0};
    size_t outlen = sizeof(out);
    void* hdr = DEVX_ADDR_OF(create_parse_graph_node_out, in, hdr);
    void* node = DEVX_ADDR_OF(create_parse_graph_node_out, out, node);
    void* sample = DEVX_ADDR_OF(parse_graph_node, node, flow_match_sample);
    uint32_t object_id = 0;
    status ret = get_id(object_id);

    if (ret != DPCP_OK) {
        log_error("Failed to get object ID for parser graph node");
        return DPCP_ERR_QUERY;
    }

    DEVX_SET(general_obj_in_cmd_hdr, hdr, opcode, MLX5_CMD_OP_QUERY_GENERAL_OBJECT);
    DEVX_SET(general_obj_in_cmd_hdr, hdr, obj_type, MLX5_GENERAL_OBJECT_TYPES_PARSE_GRAPH_NODE);
    DEVX_SET(general_obj_in_cmd_hdr, hdr, obj_id, object_id);

    ret = obj::query(in, sizeof(in), out, outlen);
    if (ret != DPCP_OK) {
        log_error("Failed to query parser graph node with ID (%d)", object_id);
        return DPCP_ERR_QUERY;
    }

    for (size_t index = 0; index < m_attrs.samples.size(); index++) {
        void* sample_offset =
            reinterpret_cast<void*>(reinterpret_cast<char*>(sample) +
                                    index * DEVX_ST_SZ_BYTES(parse_graph_flow_match_sample));
        uint32_t enabled =
            DEVX_GET(parse_graph_flow_match_sample, sample_offset, flow_match_sample_en);
        if (!enabled) {
            continue;
        }

        uint32_t sample_id =
            DEVX_GET(parse_graph_flow_match_sample, sample_offset, flow_match_sample_field_id);
        m_sample_ids.push_back(sample_id);
    }

    if (m_attrs.samples.size() != m_sample_ids.size()) {
        log_error("Number of sample IDs are not as expected for parser graph node with ID (%d)",
                  object_id);
        return DPCP_ERR_QUERY;
    }

    return DPCP_OK;
}

status parser_graph_node::get_id(uint32_t& id)
{
    id = m_parser_graph_node_id;
    return DPCP_OK;
}

} // namespace dpcp
