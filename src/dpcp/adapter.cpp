/*
 * Copyright (c) 2019-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <algorithm>
#include <vector>
#include <cmath>

#include "utils/os.h"
#include "dpcp/internal.h"

namespace dpcp {

static const std::vector<int> s_supported_cap_types {MLX5_CAP_GENERAL,
                                                     MLX5_CAP_TLS,
                                                     MLX5_CAP_PARSE_GRAPH_NODE,
                                                     MLX5_CAP_ETHERNET_OFFLOADS,
                                                     MLX5_CAP_GENERAL_2,
                                                     MLX5_CAP_FLOW_TABLE,
                                                     MLX5_CAP_DPP,
                                                     MLX5_CAP_NVMEOTCP,
                                                     MLX5_CAP_CRYPTO};

static void store_hca_device_frequency_khz_caps(adapter_hca_capabilities* external_hca_caps,
                                                const caps_map_t& caps_map)
{
    external_hca_caps->device_frequency_khz =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.device_frequency_khz);
    log_trace("Capability - device_frequency_khz: %d\n", external_hca_caps->device_frequency_khz);
}

static void store_hca_tls_caps(adapter_hca_capabilities* external_hca_caps,
                               const caps_map_t& caps_map)
{
    external_hca_caps->tls_tx = DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                                         capability.cmd_hca_cap.tls_tx);
    log_trace("Capability - tls_tx: %d\n", external_hca_caps->tls_tx);

    external_hca_caps->tls_rx = DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                                         capability.cmd_hca_cap.tls_rx);
    log_trace("Capability - tls_rx: %d\n", external_hca_caps->tls_rx);
}

static void store_hca_cap_crypto_enable(adapter_hca_capabilities* external_hca_caps,
                                        const caps_map_t& caps_map)
{
    external_hca_caps->crypto_enable = DEVX_GET(
        query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second, capability.cmd_hca_cap.crypto);
    log_trace("Capability - crypto: %d\n", external_hca_caps->crypto_enable);
}

static void store_hca_general_object_types_encryption_key_caps(
    adapter_hca_capabilities* external_hca_caps, const caps_map_t& caps_map)
{
    uint64_t general_obj_types =
        DEVX_GET64(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                   capability.cmd_hca_cap.general_obj_types);
    if (general_obj_types & MLX5_HCA_CAP_GENERAL_OBJECT_TYPES_ENCRYPTION_KEY) {
        external_hca_caps->general_object_types_encryption_key = true;
    }

    log_trace("Capability - general_object_types_encryption_key: %d\n",
              external_hca_caps->general_object_types_encryption_key);
}

static void store_hca_log_max_dek_caps(adapter_hca_capabilities* external_hca_caps,
                                       const caps_map_t& caps_map)
{
    external_hca_caps->log_max_dek =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.log_max_dek);
    log_trace("Capability - log_max_dek: %d\n", external_hca_caps->log_max_dek);
}

static void store_hca_tls_1_2_aes_gcm_caps(adapter_hca_capabilities* external_hca_caps,
                                           const caps_map_t& caps_map)
{
    external_hca_caps->tls_1_2_aes_gcm_128 =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_TLS)->second,
                 capability.tls_cap.tls_1_2_aes_gcm_128);
    log_trace("Capability - tls_1_2_aes_gcm_128_caps: %d\n",
              external_hca_caps->tls_1_2_aes_gcm_128);

    external_hca_caps->tls_1_2_aes_gcm_256 =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_TLS)->second,
                 capability.tls_cap.tls_1_2_aes_gcm_256);
    log_trace("Capability - tls_1_2_aes_gcm_256_caps: %d\n",
              external_hca_caps->tls_1_2_aes_gcm_256);
}

static void store_hca_sq_ts_format_caps(adapter_hca_capabilities* external_hca_caps,
                                        const caps_map_t& caps_map)
{
    external_hca_caps->sq_ts_format =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.sq_ts_format);
    log_trace("Capability - sq_ts_format: %d\n", external_hca_caps->sq_ts_format);
}

static void store_hca_rq_ts_format_caps(adapter_hca_capabilities* external_hca_caps,
                                        const caps_map_t& caps_map)
{
    external_hca_caps->rq_ts_format =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.rq_ts_format);
    log_trace("Capability - rq_ts_format: %d\n", external_hca_caps->rq_ts_format);
}

static void store_hca_lro_caps(adapter_hca_capabilities* external_hca_caps,
                               const caps_map_t& caps_map)
{
    caps_map_t::const_iterator iter = caps_map.find(MLX5_CAP_ETHERNET_OFFLOADS);
    void* hcattr;
    int i;

    if (iter == caps_map.end()) {
        log_fatal("Incorrect caps_map object\n");
        return;
    }

    hcattr = DEVX_ADDR_OF(query_hca_cap_out, iter->second, capability);

    external_hca_caps->lro_cap = DEVX_GET(per_protocol_networking_offload_caps, hcattr, lro_cap);
    log_trace("Capability - lro_cap: %d\n", external_hca_caps->lro_cap);

    external_hca_caps->lro_psh_flag =
        DEVX_GET(per_protocol_networking_offload_caps, hcattr, lro_psh_flag);
    log_trace("Capability - lro_psh_flag: %d\n", external_hca_caps->lro_psh_flag);

    external_hca_caps->lro_time_stamp =
        DEVX_GET(per_protocol_networking_offload_caps, hcattr, lro_time_stamp);
    log_trace("Capability - lro_time_stamp: %d\n", external_hca_caps->lro_time_stamp);

    external_hca_caps->lro_max_msg_sz_mode =
        DEVX_GET(per_protocol_networking_offload_caps, hcattr, lro_max_msg_sz_mode);
    log_trace("Capability - lro_max_msg_sz_mode: %d\n", external_hca_caps->lro_max_msg_sz_mode);

    external_hca_caps->lro_min_mss_size =
        DEVX_GET(per_protocol_networking_offload_caps, hcattr, lro_min_mss_size);
    log_trace("Capability - lro_min_mss_size: %d\n", external_hca_caps->lro_min_mss_size);

    i = 0;
    external_hca_caps->lro_timer_supported_periods[i] = (uint8_t)DEVX_GET(
        per_protocol_networking_offload_caps, hcattr, lro_timer_supported_periods[0]);
    log_trace("Capability - lro_timer_supported_periods[%d]: %d\n", i,
              external_hca_caps->lro_timer_supported_periods[i]);

    i = 1;
    external_hca_caps->lro_timer_supported_periods[i] = (uint8_t)DEVX_GET(
        per_protocol_networking_offload_caps, hcattr, lro_timer_supported_periods[1]);
    log_trace("Capability - lro_timer_supported_periods[%d]: %d\n", i,
              external_hca_caps->lro_timer_supported_periods[i]);

    i = 2;
    external_hca_caps->lro_timer_supported_periods[i] = (uint8_t)DEVX_GET(
        per_protocol_networking_offload_caps, hcattr, lro_timer_supported_periods[2]);
    log_trace("Capability - lro_timer_supported_periods[%d]: %d\n", i,
              external_hca_caps->lro_timer_supported_periods[i]);

    i = 3;
    external_hca_caps->lro_timer_supported_periods[i] = (uint8_t)DEVX_GET(
        per_protocol_networking_offload_caps, hcattr, lro_timer_supported_periods[3]);
    log_trace("Capability - lro_timer_supported_periods[%d]: %d\n", i,
              external_hca_caps->lro_timer_supported_periods[i]);
}

static void store_hca_ibq_caps(adapter_hca_capabilities* external_hca_caps,
                               const caps_map_t& caps_map)
{
    external_hca_caps->ibq = DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                                      capability.cmd_hca_cap.ibq);
    log_trace("Capability - ibq: %d\n", external_hca_caps->ibq);

    external_hca_caps->ibq_wire_protocol =
        DEVX_GET64(query_hca_cap_out, caps_map.find(MLX5_CAP_DPP)->second,
                   capability.ibq_cap.ibq_wire_protocol);
    log_trace("Capability - ibq_wire_protocol: 0x%llx\n",
              static_cast<unsigned long long>(external_hca_caps->ibq_wire_protocol));

    external_hca_caps->ibq_max_scatter_offset =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_DPP)->second,
                 capability.ibq_cap.ibq_max_scatter_offset);
    log_trace("Capability - ibq_max_scatter_offset: %d\n",
              external_hca_caps->ibq_max_scatter_offset);
}

static void store_hca_parse_graph_node_caps(adapter_hca_capabilities* external_hca_caps,
                                            const caps_map_t& caps_map)
{
    auto* parse_graph_cap = caps_map.find(MLX5_CAP_PARSE_GRAPH_NODE)->second;
    uint64_t general_obj_types =
        DEVX_GET64(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                   capability.cmd_hca_cap.general_obj_types);

    if (general_obj_types & MLX5_HCA_CAP_GENERAL_OBJECT_TYPES_PARSE_GRAPH_NODE) {
        external_hca_caps->general_object_types_parse_graph_node = true;
    }

    external_hca_caps->parse_graph_node_in = DEVX_GET(
        query_hca_cap_out, parse_graph_cap, capability.parse_graph_node_cap.parse_graph_node_in);
    external_hca_caps->parse_graph_header_length_mode =
        DEVX_GET(query_hca_cap_out, parse_graph_cap,
                 capability.parse_graph_node_cap.parse_graph_header_length_mode);
    external_hca_caps->parse_graph_flow_match_sample_offset_mode =
        DEVX_GET(query_hca_cap_out, parse_graph_cap,
                 capability.parse_graph_node_cap.parse_graph_flow_match_sample_offset_mode);
    external_hca_caps->max_num_parse_graph_arc_in =
        DEVX_GET(query_hca_cap_out, parse_graph_cap,
                 capability.parse_graph_node_cap.max_num_parse_graph_arc_in);
    external_hca_caps->max_num_parse_graph_flow_match_sample =
        DEVX_GET(query_hca_cap_out, parse_graph_cap,
                 capability.parse_graph_node_cap.max_num_parse_graph_flow_match_sample);
    external_hca_caps->parse_graph_flow_match_sample_id_in_out =
        DEVX_GET(query_hca_cap_out, parse_graph_cap,
                 capability.parse_graph_node_cap.parse_graph_flow_match_sample_id_in_out);
    external_hca_caps->max_parse_graph_header_length_base_value =
        DEVX_GET(query_hca_cap_out, parse_graph_cap,
                 capability.parse_graph_node_cap.max_parse_graph_header_length_base_value);
    external_hca_caps->max_parse_graph_flow_match_sample_field_base_offset_value = DEVX_GET(
        query_hca_cap_out, parse_graph_cap,
        capability.parse_graph_node_cap.max_parse_graph_flow_match_sample_field_base_offset_value);
    external_hca_caps->parse_graph_header_length_field_mask_width =
        DEVX_GET(query_hca_cap_out, parse_graph_cap,
                 capability.parse_graph_node_cap.parse_graph_header_length_field_mask_width);

    log_trace("Capability - general_object_types_parse_graph_node: %d\n",
              external_hca_caps->general_object_types_parse_graph_node);
    log_trace("Capability - parse_graph_node_in: 0x%x\n", external_hca_caps->parse_graph_node_in);
    log_trace("Capability - parse_graph_header_length_mode: 0x%x\n",
              external_hca_caps->parse_graph_header_length_mode);
    log_trace("Capability - parse_graph_flow_match_sample_offset_mode: 0x%x\n",
              external_hca_caps->parse_graph_flow_match_sample_offset_mode);
    log_trace("Capability - max_num_parse_graph_arc_in: %d\n",
              external_hca_caps->max_num_parse_graph_arc_in);
    log_trace("Capability - max_num_parse_graph_flow_match_sample: %d\n",
              external_hca_caps->max_num_parse_graph_flow_match_sample);
    log_trace("Capability - parse_graph_flow_match_sample_id_in_out: %d\n",
              external_hca_caps->parse_graph_flow_match_sample_id_in_out);
    log_trace("Capability - max_parse_graph_header_length_base_value: %d\n",
              external_hca_caps->max_parse_graph_header_length_base_value);
    log_trace("Capability - max_parse_graph_flow_match_sample_field_base_offset_value: %d\n",
              external_hca_caps->max_parse_graph_flow_match_sample_field_base_offset_value);
    log_trace("Capability - parse_graph_header_length_field_mask_width: %d\n",
              external_hca_caps->parse_graph_header_length_field_mask_width);
}

static void store_hca_2_reformat_caps(adapter_hca_capabilities* external_hca_caps,
                                      const caps_map_t& caps_map)
{
    external_hca_caps->flow_table_caps.reformat_flow_action_caps.max_size_reformat_insert_buff =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL_2)->second,
                 capability.cmd_hca_cap_2.max_reformat_insert_size);
    log_trace(
        "Capability - flow_table_caps.reformat_flow_action_caps.max_size_reformat_insert_buff: "
        "%d\n",
        external_hca_caps->flow_table_caps.reformat_flow_action_caps.max_size_reformat_insert_buff);

    external_hca_caps->flow_table_caps.reformat_flow_action_caps.max_reformat_insert_offset =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL_2)->second,
                 capability.cmd_hca_cap_2.max_reformat_insert_offset);
    log_trace(
        "Capability - flow_table_receive.reformat_flow_action_caps.max_reformat_insert_offset: "
        "%d\n",
        external_hca_caps->flow_table_caps.reformat_flow_action_caps.max_reformat_insert_offset);
    external_hca_caps->flow_table_caps.receive
        .is_flow_action_non_tunnel_reformat_and_fwd_to_flow_table =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL_2)->second,
                 capability.cmd_hca_cap_2.non_tunnel_reformat);
    log_trace("Capability - "
              "flow_table_caps.receive.is_flow_action_non_tunnel_reformat_and_fwd_to_flow_table: "
              "%d\n",
              external_hca_caps->flow_table_caps.receive
                  .is_flow_action_non_tunnel_reformat_and_fwd_to_flow_table);
}

static void store_hca_flow_table_caps(adapter_hca_capabilities* external_hca_caps,
                                      const caps_map_t& caps_map)
{
    external_hca_caps->is_flow_table_caps_supported =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.nic_flow_table);
    log_trace("Capability - is_flow_table_caps_supported: %d\n",
              external_hca_caps->is_flow_table_caps_supported);

    external_hca_caps->flow_table_caps.receive.max_steering_depth =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.nic_receive_max_steering_depth);
    log_trace("Capability - flow_table_caps.receive.max_steering_depth: %d\n",
              external_hca_caps->flow_table_caps.receive.max_steering_depth);

    external_hca_caps->flow_table_caps.reformat_flow_action_caps.max_log_num_of_packet_reformat =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.log_max_packet_reformat_context);
    log_trace("Capability - "
              "flow_table_caps.reformat_flow_action_caps.max_log_num_of_packet_reformat: %d\n",
              external_hca_caps->flow_table_caps.reformat_flow_action_caps
                  .max_log_num_of_packet_reformat);
}

static void store_hca_flow_table_nic_receive_caps(adapter_hca_capabilities* external_hca_caps,
                                                  const caps_map_t& caps_map)
{
    external_hca_caps->flow_table_caps.receive.is_flow_table_supported =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.ft_support);
    log_trace("Capability - flow_table_caps.receive.is_flow_table_supported: %d\n",
              external_hca_caps->flow_table_caps.receive.is_flow_table_supported);

    external_hca_caps->flow_table_caps.receive.is_flow_action_tag_supported =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.flow_tag);
    log_trace("Capability - flow_table_caps.receive.is_flow_action_tag_supported: %d\n",
              external_hca_caps->flow_table_caps.receive.is_flow_action_tag_supported);

    external_hca_caps->flow_table_caps.receive.is_flow_action_modify_supported =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.flow_modify_en);
    log_trace("Capability - flow_table_caps.receive.is_flow_action_modify_supported: %d\n",
              external_hca_caps->flow_table_caps.receive.is_flow_action_modify_supported);

    external_hca_caps->flow_table_caps.receive.is_flow_action_reformat_supported =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.reformat);
    log_trace("Capability - flow_table_caps.receive.is_flow_action_reformat_supported: %d\n",
              external_hca_caps->flow_table_caps.receive.is_flow_action_reformat_supported);

    external_hca_caps->flow_table_caps.receive
        .is_flow_action_reformat_and_modify_supported = DEVX_GET(
        query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
        capability.flow_table_nic_cap.flow_table_properties_nic_receive.reformat_and_modify_action);
    log_trace(
        "Capability - flow_table_caps.receive.is_flow_action_reformat_and_modify_supported: %d\n",
        external_hca_caps->flow_table_caps.receive.is_flow_action_reformat_and_modify_supported);

    external_hca_caps->flow_table_caps.receive
        .is_flow_action_reformat_and_fwd_to_flow_table = DEVX_GET(
        query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
        capability.flow_table_nic_cap.flow_table_properties_nic_receive.reformat_and_fwd_to_table);
    log_trace(
        "Capability - flow_table_caps.receive.is_flow_action_reformat_and_fwd_to_flow_table: %d\n",
        external_hca_caps->flow_table_caps.receive.is_flow_action_reformat_and_fwd_to_flow_table);

    external_hca_caps->flow_table_caps.receive.max_log_size_flow_table =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.log_max_ft_size);
    log_trace("Capability - flow_table_caps.receive.max_log_size_flow_table: %d\n",
              external_hca_caps->flow_table_caps.receive.max_log_size_flow_table);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.max_obj_log_num =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive
                     .log_max_modify_header_context);
    log_trace("Capability - flow_table_caps.receive.modify_flow_action_caps.max_obj_log_num: %d\n",
              external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.max_obj_log_num);

    external_hca_caps->flow_table_caps.receive.ft_field_support.outer_udp_dport =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.ft_field_support
                     .outer_udp_dport);
    log_trace("Capability - "
              "flow_table_caps.receive.ft_field_support.outer_udp_dport: %d\n",
              external_hca_caps->flow_table_caps.receive.ft_field_support.outer_udp_dport);

    external_hca_caps->flow_table_caps.receive.ft_field_support.prog_sample_field =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.ft_field_support
                     .prog_sample_field);
    log_trace("Capability - "
              "flow_table_caps.receive.ft_field_support.prog_sample_field: %d\n",
              external_hca_caps->flow_table_caps.receive.ft_field_support.prog_sample_field);

    external_hca_caps->flow_table_caps.receive.ft_field_support.metadata_reg_c_0 =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.ft_field_support
                     .metadata_reg_c_0);
    log_trace("Capability - "
              "flow_table_caps.receive.ft_field_support.metadata_reg_c_0: %d\n",
              external_hca_caps->flow_table_caps.receive.ft_field_support.metadata_reg_c_0);

    external_hca_caps->flow_table_caps.receive.ft_field_support.metadata_reg_c_1 =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.ft_field_support
                     .metadata_reg_c_1);
    log_trace("Capability - "
              "flow_table_caps.receive.ft_field_support.metadata_reg_c_1: %d\n",
              external_hca_caps->flow_table_caps.receive.ft_field_support.metadata_reg_c_1);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps
        .max_obj_in_flow_rule = DEVX_GET(
        query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
        capability.flow_table_nic_cap.flow_table_properties_nic_receive.max_modify_header_actions);
    log_trace(
        "Capability - flow_table_caps.receive.modify_flow_action_caps.max_obj_in_flow_rule: %d\n",
        external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.max_obj_in_flow_rule);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps
        .log_max_num_header_modify_argument =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.log_max_num_header_modify_argument);
    log_trace(
        "Capability - "
        "flow_table_caps.receive.modify_flow_action_caps.log_max_num_header_modify_argument: %d\n",
        external_hca_caps->flow_table_caps.receive.modify_flow_action_caps
            .log_max_num_header_modify_argument);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps
        .log_header_modify_argument_granularity =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.log_header_modify_argument_granularity);
    log_trace("Capability - "
              "flow_table_caps.receive.modify_flow_action_caps.log_header_modify_argument_"
              "granularity: %d\n",
              external_hca_caps->flow_table_caps.receive.modify_flow_action_caps
                  .log_header_modify_argument_granularity);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps
        .log_header_modify_argument_max_alloc =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.log_header_modify_argument_max_alloc);
    log_trace("Capability - "
              "flow_table_caps.receive.modify_flow_action_caps.log_header_modify_argument_max_"
              "alloc: %d\n",
              external_hca_caps->flow_table_caps.receive.modify_flow_action_caps
                  .log_header_modify_argument_max_alloc);

    external_hca_caps->flow_table_caps.receive.max_flow_table_level =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.max_ft_level);
    log_trace("Capability - flow_table_caps.receive.max_flow_table_level: %d\n",
              external_hca_caps->flow_table_caps.receive.max_flow_table_level);

    external_hca_caps->flow_table_caps.receive.is_flow_action_reformat_from_type_insert_supported =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.reformat_insert);
    log_trace("Capability - "
              "flow_table_caps.receive.is_flow_action_reformat_from_type_insert_supported: %d\n",
              external_hca_caps->flow_table_caps.receive
                  .is_flow_action_reformat_from_type_insert_supported);

    external_hca_caps->flow_table_caps.receive.max_log_num_of_flow_table =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.log_max_ft_num);
    log_trace("Capability - flow_table_caps.receive.max_log_num_of_flow_table: %d\n",
              external_hca_caps->flow_table_caps.receive.max_log_num_of_flow_table);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.max_obj_log_num =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive
                     .log_max_modify_header_context);
    log_trace("Capability - flow_table_caps.receive.modify_flow_action_caps.max_obj_log_num: %d\n",
              external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.max_obj_log_num);

    external_hca_caps->flow_table_caps.receive.max_log_num_of_flow_rule =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.log_max_flow);
    log_trace("Capability - flow_table_caps.receive.max_log_num_of_flow_rule: %d\n",
              external_hca_caps->flow_table_caps.receive.max_log_num_of_flow_rule);

    external_hca_caps->flow_table_caps.receive.is_flow_action_reparse_supported =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                 capability.flow_table_nic_cap.flow_table_properties_nic_receive.reparse);
    log_trace("Capability - flow_table_caps.receive.is_flow_action_reparse_supported: %d\n",
              external_hca_caps->flow_table_caps.receive.is_flow_action_reparse_supported);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.set_fields_support
        .outer_ethertype = DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                                    capability.flow_table_nic_cap.header_modify_nic_receive
                                        .set_action_field_support.outer_ether_type);
    log_trace(
        "Capability - "
        "flow_table_caps.receive.modify_flow_action_caps.set_fields_support.outer_ethertype: %d\n",
        external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.set_fields_support
            .outer_ethertype);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.set_fields_support
        .outer_udp_dport = DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                                    capability.flow_table_nic_cap.header_modify_nic_receive
                                        .set_action_field_support.outer_udp_dport);
    log_trace(
        "Capability - "
        "flow_table_caps.receive.modify_flow_action_caps.set_fields_support.outer_udp_dport: %d\n",
        external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.set_fields_support
            .outer_udp_dport);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.set_fields_support
        .metadata_reg_c_0 = DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                                     capability.flow_table_nic_cap.header_modify_nic_receive
                                         .set_action_field_support.metadata_reg_c_0);
    log_trace(
        "Capability - "
        "flow_table_caps.receive.modify_flow_action_caps.set_fields_support.metadata_reg_c_0: %d\n",
        external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.set_fields_support
            .metadata_reg_c_0);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.set_fields_support
        .metadata_reg_c_1 = DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                                     capability.flow_table_nic_cap.header_modify_nic_receive
                                         .set_action_field_support.metadata_reg_c_1);
    log_trace(
        "Capability - "
        "flow_table_caps.receive.modify_flow_action_caps.set_fields_support.metadata_reg_c_1: %d\n",
        external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.set_fields_support
            .metadata_reg_c_1);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.copy_fields_support
        .outer_udp_dport = DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                                    capability.flow_table_nic_cap.header_modify_nic_receive
                                        .copy_action_field_support.outer_udp_dport);
    log_trace(
        "Capability - "
        "flow_table_caps.receive.modify_flow_action_caps.copy_fields_support.outer_udp_dport: %d\n",
        external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.copy_fields_support
            .outer_udp_dport);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.copy_fields_support
        .metadata_reg_c_0 = DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                                     capability.flow_table_nic_cap.header_modify_nic_receive
                                         .copy_action_field_support.metadata_reg_c_0);
    log_trace("Capability - "
              "flow_table_caps.receive.modify_flow_action_caps.copy_fields_support.metadata_reg_c_"
              "0: %d\n",
              external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.copy_fields_support
                  .metadata_reg_c_0);

    external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.copy_fields_support
        .metadata_reg_c_1 = DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_FLOW_TABLE)->second,
                                     capability.flow_table_nic_cap.header_modify_nic_receive
                                         .copy_action_field_support.metadata_reg_c_1);
    log_trace("Capability - "
              "flow_table_caps.receive.modify_flow_action_caps.copy_fields_support.metadata_reg_c_"
              "1: %d\n",
              external_hca_caps->flow_table_caps.receive.modify_flow_action_caps.copy_fields_support
                  .metadata_reg_c_1);
}

static void store_hca_crypto_caps(adapter_hca_capabilities* external_hca_caps,
                                  const caps_map_t& caps_map)
{
    external_hca_caps->synchronize_dek =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_CRYPTO)->second,
                 capability.crypto_cap.synchronize_dek);
    log_trace("Capability - synchronize_dek: %d\n", external_hca_caps->synchronize_dek);

    external_hca_caps->log_max_num_deks =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_CRYPTO)->second,
                 capability.crypto_cap.log_max_num_deks);
    log_trace("Capability - log_max_num_deks: %d\n", external_hca_caps->log_max_num_deks);
}

static void store_hca_nvmeotcp_caps(adapter_hca_capabilities* external_hca_caps,
                                    const caps_map_t& caps_map)
{
    external_hca_caps->nvmeotcp_caps.enabled =
        DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_GENERAL)->second,
                 capability.cmd_hca_cap.nvmeotcp);

    if (external_hca_caps->nvmeotcp_caps.enabled) {
        external_hca_caps->nvmeotcp_caps.zerocopy =
            DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_NVMEOTCP)->second,
                     capability.nvmeotcp_cap.zerocopy);
        external_hca_caps->nvmeotcp_caps.crc_rx =
            DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_NVMEOTCP)->second,
                     capability.nvmeotcp_cap.crc_rx);
        external_hca_caps->nvmeotcp_caps.crc_tx =
            DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_NVMEOTCP)->second,
                     capability.nvmeotcp_cap.crc_tx);
        external_hca_caps->nvmeotcp_caps.log_max_nvmeotcp_tag_buffer_table =
            DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_NVMEOTCP)->second,
                     capability.nvmeotcp_cap.log_max_nvmeotcp_tag_buffer_table);
        external_hca_caps->nvmeotcp_caps.log_max_nvmeotcp_tag_buffer_size =
            DEVX_GET(query_hca_cap_out, caps_map.find(MLX5_CAP_NVMEOTCP)->second,
                     capability.nvmeotcp_cap.log_max_nvmeotcp_tag_buffer_size);
        log_trace(
            "Capability - nvmeotcp: ENABLED - zerocopy:%d, crc_rx: %d, crc_tx: %d, version: %d, "
            "log_max_nvmeotcp_tag_buffer_table: %d, log_max_nvmeotcp_tag_buffer_size: %d\n",
            external_hca_caps->nvmeotcp_caps.zerocopy, external_hca_caps->nvmeotcp_caps.crc_rx,
            external_hca_caps->nvmeotcp_caps.crc_tx, external_hca_caps->nvmeotcp_caps.version,
            external_hca_caps->nvmeotcp_caps.log_max_nvmeotcp_tag_buffer_table,
            external_hca_caps->nvmeotcp_caps.log_max_nvmeotcp_tag_buffer_size);
    } else {
        external_hca_caps->nvmeotcp_caps.zerocopy = false;
        external_hca_caps->nvmeotcp_caps.crc_tx = false;
        external_hca_caps->nvmeotcp_caps.crc_rx = false;
        log_trace("Capability - nvmeotcp: DISABLED\n");
    }
}

static const std::vector<cap_cb_fn> caps_callbacks = {
    store_hca_device_frequency_khz_caps,
    store_hca_tls_caps,
    store_hca_general_object_types_encryption_key_caps,
    store_hca_log_max_dek_caps,
    store_hca_tls_1_2_aes_gcm_caps,
    store_hca_cap_crypto_enable,
    store_hca_sq_ts_format_caps,
    store_hca_rq_ts_format_caps,
    store_hca_lro_caps,
    store_hca_ibq_caps,
    store_hca_parse_graph_node_caps,
    store_hca_2_reformat_caps,
    store_hca_flow_table_caps,
    store_hca_flow_table_nic_receive_caps,
    store_hca_crypto_caps,
    store_hca_nvmeotcp_caps,
};

status pd_devx::create()
{
    uint32_t in[DEVX_ST_SZ_DW(alloc_pd_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(alloc_pd_out)] = {};
    size_t outlen = sizeof(out);

    DEVX_SET(alloc_pd_in, in, opcode, MLX5_CMD_OP_ALLOC_PD);
    status ret = obj::create(in, sizeof(in), out, outlen);

    if (DPCP_OK == ret) {
        m_pd_id = DEVX_GET(alloc_pd_out, out, pd);
    }
    return ret;
}

status pd_ibv::create()
{
    dcmd::ctx* ctx = obj::get_ctx();
    if (nullptr == ctx) {
        return DPCP_ERR_NO_CONTEXT;
    }

    if (!m_is_external_ibv_pd) {
        m_ibv_pd = (void*)ibv_alloc_pd((ibv_context*)ctx->get_context());
        if (nullptr == m_ibv_pd) {
            return DPCP_ERR_CREATE;
        }
        log_trace("ibv_pd %p was created internaly\n", m_ibv_pd);
    }

    int err = ctx->create_ibv_pd(m_ibv_pd, m_pd_id);
    if (err) {
        return DPCP_ERR_INVALID_ID;
    }

    return DPCP_OK;
}

status td::create()
{
    uint32_t in[DEVX_ST_SZ_DW(alloc_transport_domain_in)] = {};
    uint32_t out[DEVX_ST_SZ_DW(alloc_transport_domain_out)] = {};
    size_t outlen = sizeof(out);

    DEVX_SET(alloc_transport_domain_in, in, opcode, MLX5_CMD_OP_ALLOC_TRANSPORT_DOMAIN);
    status ret = obj::create(in, sizeof(in), out, outlen);

    if (DPCP_OK == ret) {
        m_td_id = DEVX_GET(alloc_transport_domain_out, out, transport_domain);
    }
    return ret;
}

adapter::adapter(dcmd::device* dev, dcmd::ctx* ctx)
    : m_dcmd_dev(dev)
    , m_dcmd_ctx(ctx)
    , m_td(nullptr)
    , m_pd(nullptr)
    , m_uarpool(nullptr)
    , m_ibv_pd(nullptr)
    , m_pd_id(0)
    , m_td_id(0)
    , m_eqn(0)
    , m_is_caps_available(false)
    , m_caps()
    , m_external_hca_caps(nullptr)
    , m_caps_callbacks(caps_callbacks)
    , m_opened(false)
    , m_flow_action_generator(m_dcmd_ctx, m_external_hca_caps)
{
    for (auto cap_type : s_supported_cap_types) {
        m_caps.insert(std::make_pair(cap_type, calloc(1, DEVX_ST_SZ_BYTES(query_hca_cap_out))));
    }

    query_hca_caps();
    set_external_hca_caps();
}

status adapter::set_pd(uint32_t pdn, void* ibv_pd)
{
    if (0 == pdn || nullptr == ibv_pd) {
        return DPCP_ERR_INVALID_PARAM;
    }
    m_pd_id = pdn;
    m_ibv_pd = ibv_pd; // TODO: till DevX GPU memory is supported
    return DPCP_OK;
}

status adapter::create_ibv_pd(void* ibv_pd)
{
    status ret = DPCP_OK;

    // Protection Domain was already created.
    if (m_pd) {
        if (m_ibv_pd == ibv_pd) {
            log_trace("ibv_pd %p was already created\n", ibv_pd);
            return DPCP_OK;
        } else {
            log_error("failed to create ibv_pd, it's already set to %p\n", m_ibv_pd);
            return DPCP_ERR_CREATE;
        }
    }

    m_pd = new (std::nothrow) pd_ibv(m_dcmd_ctx, ibv_pd);
    if (nullptr == m_pd) {
        return DPCP_ERR_NO_MEMORY;
    }

    ret = m_pd->create();
    if (DPCP_OK != ret) {
        return ret;
    }

    return set_pd(m_pd->get_pd_id(), ((pd_ibv*)m_pd)->get_ibv_pd());
}

status adapter::create_own_pd()
{
    status ret = DPCP_OK;

    m_pd = new (std::nothrow) pd_devx(m_dcmd_ctx);
    if (nullptr == m_pd) {
        return DPCP_ERR_NO_MEMORY;
    }

    ret = m_pd->create();
    if (DPCP_OK != ret) {
        return ret;
    }

    ret = m_pd->get_id(m_pd_id);
    if (DPCP_OK != ret) {
        return ret;
    }

    return ret;
}

std::string adapter::get_name()
{
    return m_dcmd_dev->get_name();
}

status adapter::set_td(uint32_t tdn)
{
    if (0 == tdn) {
        return DPCP_ERR_INVALID_PARAM;
    }
    m_td_id = tdn;
    if (nullptr != m_td) {
        delete m_td;
        m_td = nullptr;
    }
    return DPCP_OK;
}

void* adapter::get_ibv_context()
{
    return m_dcmd_ctx->get_context();
}

status adapter::get_real_time(uint64_t& real_time)
{
    uint64_t rtc = m_dcmd_ctx->get_real_time();
    if (0 == rtc) {
        return DPCP_ERR_NO_CONTEXT;
    }
    uint32_t nanoseconds = (uint32_t)(rtc & ~(0x3 << 30)); // get the low 30 bits
    uint32_t seconds = (uint32_t)(rtc >> 32); // get the high 32 bits
    std::chrono::seconds s(seconds);

    real_time = (uint64_t)(nanoseconds + std::chrono::nanoseconds(s).count());
    return DPCP_OK;
}

status adapter::open()
{
    status ret = DPCP_OK;
    if (is_opened()) {
        return ret;
    }
    // Allocate and Create Protection Domain
    if (!get_pd()) {
        ret = create_ibv_pd();
        if (DPCP_OK != ret) {
            return ret;
        }
    }
    // Allocate and Create Transport Domain
    if (!m_td_id) {
        m_td = new (std::nothrow) td(m_dcmd_ctx);
        if (nullptr == m_td) {
            return DPCP_ERR_NO_MEMORY;
        }
        ret = m_td->create();
        if (DPCP_OK != ret) {
            return ret;
        }
        ret = m_td->get_id(m_td_id);
        if (DPCP_OK != ret) {
            return ret;
        }
    }
    // Allocate UAR pool
    if (nullptr == m_uarpool) {
        m_uarpool = new (std::nothrow) uar_collection(get_ctx());
        if (nullptr == m_uarpool) {
            return DPCP_ERR_NO_MEMORY;
        }
    }
    // Mapping device ctx to iseg for getting RTC on BF2 device
    int err = m_dcmd_ctx->hca_iseg_mapping();
    if (err) {
        log_error("hca_iseg_mapping failed ret=0x%x\n", err);
        return DPCP_ERR_NO_CONTEXT;
    }
    m_opened = true;
    return ret;
}

status adapter::create_tir(const tir::attr& tir_attr, tir*& tir_obj)
{
    status ret = DPCP_OK;
    tir* _tir_obj = nullptr;

    _tir_obj = new (std::nothrow) tir(get_ctx());
    if (nullptr == _tir_obj) {
        return DPCP_ERR_NO_MEMORY;
    }

    ret = _tir_obj->create(tir_attr);
    if (DPCP_OK != ret) {
        delete _tir_obj;
        return DPCP_ERR_CREATE;
    }
    tir_obj = _tir_obj;

    return DPCP_OK;
}

status adapter::create_tis(const tis::attr& tis_attr, tis*& tis_obj)
{
    status ret = DPCP_OK;
    tis* _tis_obj = nullptr;

    _tis_obj = new (std::nothrow) tis(get_ctx());
    if (nullptr == _tis_obj) {
        return DPCP_ERR_NO_MEMORY;
    }

    ret = _tis_obj->create(tis_attr);
    if (DPCP_OK != ret) {
        delete _tis_obj;
        return DPCP_ERR_CREATE;
    }
    tis_obj = _tis_obj;

    return DPCP_OK;
}

status adapter::create_direct_mkey(void* address, size_t length, mkey_flags flags,
                                   direct_mkey*& dmk)
{
    dmk = new (std::nothrow) direct_mkey(this, address, length, flags);
    log_trace("dmk: %p\n", dmk);
    if (nullptr == dmk) {
        return DPCP_ERR_NO_MEMORY;
    }
    // Register UserMEMory
    status ret = dmk->reg_mem(m_ibv_pd);
    if (DPCP_OK != ret) {
        delete dmk;
        return DPCP_ERR_UMEM;
    }
    // Create MKey
    ret = dmk->create();
    if (DPCP_OK != ret) {
        delete dmk;
        return DPCP_ERR_CREATE;
    }

    return DPCP_OK;
}

status adapter::create_pattern_mkey(void* addr, mkey_flags flags, size_t stride_num, size_t bb_num,
                                    pattern_mkey_bb bb_arr[], pattern_mkey*& pmk)
{
    pmk = new (std::nothrow) pattern_mkey(this, addr, flags, stride_num, bb_num, bb_arr);
    log_trace("pattern mkey: %p\n", pmk);
    if (nullptr == pmk) {
        return DPCP_ERR_NO_MEMORY;
    }

    // Create MKey
    status ret = pmk->create();
    if (DPCP_OK != ret) {
        delete pmk;
        return DPCP_ERR_CREATE;
    }

    return DPCP_OK;
}

status reg_mem(dcmd::ctx* ctx, void* buf, size_t sz, dcmd::umem*& umem, uint32_t& mem_id)
{
    if (nullptr == ctx) {
        return DPCP_ERR_NO_CONTEXT;
    }
    if (nullptr == buf) {
        return DPCP_ERR_NO_MEMORY;
    }
    if (0 == sz) {
        return DPCP_ERR_OUT_OF_RANGE;
    }

    dcmd::umem_desc dscr = {(void*)buf, sz, 1};

    umem = ctx->create_umem(&dscr);
    if (nullptr == umem) {
        return DPCP_ERR_UMEM;
    }
    mem_id = umem->get_id();
    return DPCP_OK;
}

status adapter::create_reserved_mkey(reserved_mkey_type type, void* address, size_t length,
                                     mkey_flags flags, reserved_mkey*& rmk)
{
    rmk = new (std::nothrow) reserved_mkey(this, type, address, (uint32_t)length, flags);
    log_trace("rmk: %p\n", rmk);
    if (nullptr == rmk) {
        return DPCP_ERR_NO_MEMORY;
    }
    // Create MKey
    status ret = rmk->create();
    if (DPCP_OK != ret) {
        delete rmk;
        return DPCP_ERR_CREATE;
    }

    return DPCP_OK;
}

status adapter::create_ref_mkey(mkey* parent, void* address, size_t length, ref_mkey*& mkey)
{
    mkey = new (std::nothrow) ref_mkey(this, address, length);
    log_trace("refmk: %p\n", mkey);
    if (nullptr == mkey) {
        return DPCP_ERR_NO_MEMORY;
    }
    // Create MKey
    status ret = mkey->create(parent);
    if (DPCP_OK != ret) {
        delete mkey;
        return DPCP_ERR_CREATE;
    }

    return DPCP_OK;
}

status adapter::create_extern_mkey(void* address, size_t length, uint32_t id, extern_mkey*& mkey)
{
    mkey = new (std::nothrow) extern_mkey(this, address, length, id);
    log_trace("extern_mk: %p\n", mkey);
    return (nullptr == mkey) ? DPCP_ERR_NO_MEMORY : DPCP_OK;
}

status adapter::create_cq(const cq_attr& attrs, cq*& out_cq)
{
    // CQ_SIZE is mandatory
    if (!attrs.cq_attr_use.test(CQ_SIZE) || !attrs.cq_sz) {
        return DPCP_ERR_INVALID_PARAM;
    }
    // EventQueue Id number is also mandatory
    if (!attrs.cq_attr_use.test(CQ_EQ_NUM)) {
        return DPCP_ERR_INVALID_PARAM;
    }

    if (nullptr == m_uarpool) {
        // Allocate UAR pool
        m_uarpool = new (std::nothrow) uar_collection(get_ctx());
        if (nullptr == m_uarpool) {
            return DPCP_ERR_NO_MEMORY;
        }
    }
    cq* cq64 = new (std::nothrow) cq(this, attrs);
    if (nullptr == cq64) {
        return DPCP_ERR_NO_MEMORY;
    }
    // Obrain UAR for new CQ
    uar cq_uar = m_uarpool->get_uar(cq64);
    if (nullptr == cq_uar) {
        delete cq64;
        return DPCP_ERR_ALLOC_UAR;
    }
    uar_t uar_p;
    status ret = m_uarpool->get_uar_page(cq_uar, uar_p);
    if (DPCP_OK != ret) {
        delete cq64;
        return ret;
    }
    // Allocate CQ Buf
    void* cq_buf = nullptr;
    size_t cq_buf_sz = cq64->get_cq_buf_sz();
    ret = cq64->allocate_cq_buf(cq_buf, cq_buf_sz);
    if (DPCP_OK != ret) {
        delete cq64;
        return ret;
    }
    // Register UMEM for CQ Buffer
    ret = reg_mem(get_ctx(), (void*)cq_buf, cq_buf_sz, cq64->m_cq_buf_umem, cq64->m_cq_buf_umem_id);
    if (DPCP_OK != ret) {
        cq64->release_cq_buf(cq_buf);
        delete cq64;
        return ret;
    }
    log_trace("create_cq Buf: 0x%p sz: 0x%x umem_id: %x\n", cq_buf, (uint32_t)cq_buf_sz,
              cq64->m_cq_buf_umem_id);
    //
    // Allocated DB
    uint32_t* db_rec = nullptr;
    size_t db_rec_sz = 0;
    ret = cq64->allocate_db_rec(db_rec, db_rec_sz);
    if (DPCP_OK != ret) {
        delete cq64->m_cq_buf_umem;
        cq64->release_cq_buf(cq_buf);
        delete cq64;
        return ret;
    }
    // Register UMEM for DoorBell record
    ret = reg_mem(get_ctx(), (void*)db_rec, db_rec_sz, cq64->m_db_rec_umem, cq64->m_db_rec_umem_id);
    if (DPCP_OK != ret) {
        cq64->release_db_rec(db_rec);
        delete cq64->m_cq_buf_umem;
        cq64->release_cq_buf(cq_buf);
        delete cq64;
        return ret;
    }
    log_trace("create_cq DB: 0x%p sz: 0x%zx umem_id: %x\n", db_rec, db_rec_sz,
              cq64->m_db_rec_umem_id);

    ret = cq64->init(&uar_p);
    if (DPCP_OK == ret) {
        out_cq = cq64;
    } else {
        delete cq64->m_db_rec_umem;
        cq64->release_db_rec(db_rec);
        delete cq64->m_cq_buf_umem;
        cq64->release_cq_buf(cq_buf);
        delete cq64;
    }
    return ret;
}

status adapter::prepare_basic_rq(basic_rq& srq)
{
    // Obrain UAR for new RQ
    uar rq_uar = m_uarpool->get_uar(&srq);
    if (nullptr == rq_uar) {
        return DPCP_ERR_ALLOC_UAR;
    }
    uar_t uar_p;
    status ret = m_uarpool->get_uar_page(rq_uar, uar_p);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Allocate WQ Buf
    void* wq_buf = nullptr;
    size_t wq_buf_sz = srq.get_wq_buf_sz();
    ret = srq.allocate_wq_buf(wq_buf, wq_buf_sz);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Register UMEM for WQ Buffer
    ret = reg_mem(get_ctx(), (void*)wq_buf, wq_buf_sz, srq.m_wq_buf_umem, srq.m_wq_buf_umem_id);
    if (DPCP_OK != ret) {
        return ret;
    }
    log_trace("prepare_basic_rq Buf: 0x%p sz: 0x%x umem_id: %x\n", wq_buf, (uint32_t)wq_buf_sz,
              srq.m_wq_buf_umem_id);
    //
    // Allocated DB
    uint32_t* db_rec = nullptr;
    size_t db_rec_sz = 0;
    ret = srq.allocate_db_rec(db_rec, db_rec_sz);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Register UMEM for DoorBell record
    ret = reg_mem(get_ctx(), (void*)db_rec, db_rec_sz, srq.m_db_rec_umem, srq.m_db_rec_umem_id);
    if (DPCP_OK != ret) {
        return ret;
    }
    log_trace("prepare_basic_rq DB: 0x%p sz: 0x%zx umem_id: %x\n", db_rec, db_rec_sz,
              srq.m_db_rec_umem_id);

    return srq.init(&uar_p);
}

status adapter::create_striding_rq(const rq_attr& rq_attr, striding_rq*& str_rq)
{
    if (!m_uarpool) {
        // Allocate UAR pool
        m_uarpool = new (std::nothrow) uar_collection(get_ctx());
        if (!m_uarpool)
            return DPCP_ERR_NO_MEMORY;
    }

    std::unique_ptr<striding_rq> srq(new (std::nothrow) striding_rq(this, rq_attr));
    if (!srq)
        return DPCP_ERR_NO_MEMORY;

    status ret = prepare_basic_rq(*srq);
    if (DPCP_OK == ret)
        str_rq = srq.release();

    return ret;
}

status adapter::create_regular_rq(const rq_attr& rq_attr, regular_rq*& reg_rq)
{
    if (!m_uarpool) {
        // Allocate UAR pool
        m_uarpool = new (std::nothrow) uar_collection(get_ctx());
        if (!m_uarpool)
            return DPCP_ERR_NO_MEMORY;
    }

    std::unique_ptr<regular_rq> srq(new (std::nothrow) regular_rq(this, rq_attr));
    if (!srq)
        return DPCP_ERR_NO_MEMORY;

    status ret = prepare_basic_rq(*srq);
    if (DPCP_OK == ret)
        reg_rq = srq.release();

    return ret;
}

status adapter::create_ibq_rq(rq_attr& rq_attr, dpcp_ibq_protocol ibq_protocol, uint32_t mkey,
                              ibq_rq*& d_rq)
{
    ibq_rq* drq = new (std::nothrow) ibq_rq(this, rq_attr);
    if (nullptr == drq) {
        return DPCP_ERR_NO_MEMORY;
    }
    status ret = drq->init(ibq_protocol, mkey);
    if (DPCP_OK != ret) {
        delete drq;
        return ret;
    }
    d_rq = drq;

    return ret;
}

std::shared_ptr<flow_table> adapter::get_root_table(flow_table_type type)
{
    if (type >= flow_table_type::FT_END || type < 0) {
        return std::shared_ptr<flow_table>();
    }

    if (!m_root_table_arr[type]) {
        m_root_table_arr[type].reset(new (std::nothrow) flow_table_kernel(m_dcmd_ctx, type));
        m_root_table_arr[type]->create();
    }

    return m_root_table_arr[type];
}

status adapter::verify_flow_table_receive_attr(const flow_table_attr& attr)
{
    auto caps = m_external_hca_caps;

    if (!caps->flow_table_caps.receive.is_flow_table_supported) {
        log_error("Flow Table from type receive is not supported\n");
        return DPCP_ERR_CREATE;
    }
    if (caps->flow_table_caps.receive.max_log_size_flow_table < attr.log_size) {
        log_error("Flow Table max log size %d, requested %d\n",
                  caps->flow_table_caps.receive.max_log_size_flow_table, attr.log_size);
        return DPCP_ERR_INVALID_PARAM;
    }
    if (caps->flow_table_caps.receive.max_flow_table_level < attr.level) {
        log_error("Flow Table max level %d, requested %d\n",
                  caps->flow_table_caps.receive.max_flow_table_level, attr.level);
        return DPCP_ERR_INVALID_PARAM;
    }

    return DPCP_OK;
}

status adapter::create_flow_table(flow_table_attr& attr, std::shared_ptr<flow_table>& table)
{
    status ret;

    if (attr.level == 0) {
        log_error("Flow Table level 0 is reserved for root table\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    switch (attr.type) {
    case flow_table_type::FT_RX:
        ret = verify_flow_table_receive_attr(attr);
        break;
    default:
        log_error("Adapter do not support Flow Table from type %d\n", attr.type);
        ret = DPCP_ERR_NO_SUPPORT;
    }
    if (ret != DPCP_OK) {
        log_error("Flow Table of type %d, invalid attributes, ret %d\n", attr.type, ret);
        return ret;
    }

    table.reset(new (std::nothrow) flow_table_prm(m_dcmd_ctx, attr));
    if (!table) {
        log_error("Flow table allocation failed\n");
        return DPCP_ERR_NO_MEMORY;
    }

    return DPCP_OK;
}

status adapter::create_flow_rule(uint16_t priority, match_params& match_criteria, flow_rule*& rule)
{
    flow_rule* fr = new (std::nothrow) flow_rule(m_dcmd_ctx, priority, match_criteria);
    if (nullptr == fr) {
        return DPCP_ERR_NO_MEMORY;
    }
    rule = fr;

    return DPCP_OK;
}

status adapter::create_comp_channel(comp_channel*& out_cch)
{
    comp_channel* cch = new (std::nothrow) comp_channel(this);
    if (nullptr == cch) {
        return DPCP_ERR_NO_MEMORY;
    }
    out_cch = cch;

    return DPCP_OK;
}

status adapter::query_eqn(uint32_t& eqn, uint32_t cpu_vector)
{
    uint32_t e;
    if (!m_dcmd_ctx->query_eqn(cpu_vector, e)) {
        eqn = m_eqn = e;
        log_trace("query_eqn: %d for cpu_vector 0x%x\n", eqn, cpu_vector);
        return DPCP_OK;
    }
    return DPCP_ERR_QUERY;
}

status adapter::create_pp_sq(sq_attr& sq_attr, pp_sq*& packet_pacing_sq)
{
    if (nullptr == m_uarpool) {
        // Allocate UAR pool
        m_uarpool = new (std::nothrow) uar_collection(get_ctx());
        if (nullptr == m_uarpool) {
            return DPCP_ERR_NO_MEMORY;
        }
    }
    pp_sq* ppsq = new (std::nothrow) pp_sq(this, sq_attr);
    if (nullptr == ppsq) {
        return DPCP_ERR_NO_MEMORY;
    }
    packet_pacing_sq = ppsq;
    // Obrain UAR for new SQ
    uar sq_uar = m_uarpool->get_uar(ppsq);
    if (nullptr == sq_uar) {
        return DPCP_ERR_ALLOC_UAR;
    }
    uar_t uar_p;
    status ret = m_uarpool->get_uar_page(sq_uar, uar_p);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Allocate WQ Buf
    void* wq_buf = nullptr;
    size_t wq_buf_sz = ppsq->get_wq_buf_sz();
    ret = ppsq->allocate_wq_buf(wq_buf, wq_buf_sz);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Register UMEM for WQ Buffer
    ret = reg_mem(get_ctx(), (void*)wq_buf, wq_buf_sz, ppsq->m_wq_buf_umem, ppsq->m_wq_buf_umem_id);
    if (DPCP_OK != ret) {
        return ret;
    }
    log_trace("create_pp_sq Buf: 0x%p sz: 0x%x umem_id: %x\n", wq_buf, (uint32_t)wq_buf_sz,
              ppsq->m_wq_buf_umem_id);
    //
    // Allocated DB
    uint32_t* db_rec = nullptr;
    size_t db_rec_sz = 0;
    ret = ppsq->allocate_db_rec(db_rec, db_rec_sz);
    if (DPCP_OK != ret) {
        return ret;
    }
    // Register UMEM for DoorBell record
    ret = reg_mem(get_ctx(), (void*)db_rec, db_rec_sz, ppsq->m_db_rec_umem, ppsq->m_db_rec_umem_id);
    if (DPCP_OK != ret) {
        return ret;
    }
    log_trace("create_pp_sq DB: 0x%p sz: 0x%zx umem_id: %x\n", db_rec, db_rec_sz,
              ppsq->m_db_rec_umem_id);

    ret = ppsq->init(&uar_p);
    return ret;
}

status adapter::query_hca_caps()
{
    uint32_t in[DEVX_ST_SZ_DW(query_hca_cap_in)] = {0};
    enum mlx5_cap_mode cap_mode = HCA_CAP_OPMOD_GET_CUR;
    int ret;
    uint32_t opmod;

    for (auto cap_type : s_supported_cap_types) {
        opmod = (cap_type << 1) | cap_mode;
        DEVX_SET(query_hca_cap_in, in, opcode, MLX5_CMD_OP_QUERY_HCA_CAP);
        DEVX_SET(query_hca_cap_in, in, op_mod, opmod);
        ret = m_dcmd_ctx->exec_cmd(in, sizeof(in), m_caps[cap_type],
                                   DEVX_ST_SZ_BYTES(query_hca_cap_out));
        if (ret) {
            log_trace("Cap type: %d query failed %d\n", cap_type, ret);
        }
    }

    return DPCP_OK;
}

void adapter::set_external_hca_caps()
{
    m_external_hca_caps = new adapter_hca_capabilities();
    for (auto& callback : m_caps_callbacks) {
        callback(m_external_hca_caps, m_caps);
    }
    m_is_caps_available = true;
}

status adapter::get_hca_caps_frequency_khz(uint32_t& freq)
{
    if (!m_is_caps_available) {
        return DPCP_ERR_QUERY;
    }

    freq = m_external_hca_caps->device_frequency_khz;
    log_trace("Adapter frequency (khz) %d\n", freq);
    return DPCP_OK;
}

status adapter::create_dek(const dek::attr& dek_attr, dek*& dek_obj)
{
    status ret = DPCP_OK;
    dek* _dek_obj = nullptr;

    if (!(dek_attr.flags & DEK_ATTR_TLS)) {
        log_trace("Only TLS encryption key type is supported");
        return DPCP_ERR_NO_SUPPORT;
    }

    if (m_is_caps_available && !m_external_hca_caps->general_object_types_encryption_key) {
        log_trace("The adapter doesn't support the creation of general object encryption key");
        return DPCP_ERR_NO_SUPPORT;
    }

    _dek_obj = new (std::nothrow) dek(get_ctx());
    if (nullptr == _dek_obj) {
        return DPCP_ERR_NO_MEMORY;
    }

    ret = _dek_obj->create(dek_attr);
    if (DPCP_OK != ret) {
        delete _dek_obj;
        return DPCP_ERR_CREATE;
    }
    dek_obj = _dek_obj;

    return DPCP_OK;
}

status adapter::sync_crypto_tls()
{
    uint32_t in[DEVX_ST_SZ_DW(sync_crypto_in)] = {0};
    uint32_t out[DEVX_ST_SZ_DW(sync_crypto_out)] = {0};

    DEVX_SET(sync_crypto_in, in, opcode, MLX5_CMD_OP_SYNC_CRYPTO);
    DEVX_SET(sync_crypto_in, in, crypto_type, MLX5_GENERAL_OBJECT_TYPE_ENCRYPTION_KEY_TYPE_TLS);
    int ret = m_dcmd_ctx->exec_cmd(in, sizeof(in), out, sizeof(out));
    if (ret) {
        log_trace("CRYPTO_SYNC TLS failed %d, errno: %d\n", ret, errno);
        return DPCP_ERR_MODIFY;
    }

    log_trace("CRYPTO_SYNC success: status: %u syndrome: %x\n",
              static_cast<unsigned int>(DEVX_GET(sync_crypto_out, out, status)),
              static_cast<unsigned int>(DEVX_GET(sync_crypto_out, out, syndrome)));

    return DPCP_OK;
}

status adapter::create_tag_buffer_table_obj(const tag_buffer_table_obj::attr& tag_buffer_table_attr,
                                            tag_buffer_table_obj*& tag_buffer_table_object)
{
    status ret = DPCP_OK;
    tag_buffer_table_obj* tag_buf_obj = nullptr;

    tag_buf_obj = new (std::nothrow) tag_buffer_table_obj(get_ctx());
    if (nullptr == tag_buf_obj) {
        return DPCP_ERR_NO_MEMORY;
    }

    ret = tag_buf_obj->create(tag_buffer_table_attr);
    if (DPCP_OK != ret) {
        delete tag_buf_obj;
        return DPCP_ERR_CREATE;
    }

    tag_buffer_table_object = tag_buf_obj;

    return DPCP_OK;
}

adapter::~adapter()
{
    m_is_caps_available = false;

    if (m_pd) {
        delete m_pd;
        m_pd = nullptr;
    }
    if (m_td) {
        delete m_td;
        m_td = nullptr;
    }
    if (m_uarpool) {
        delete m_uarpool;
        m_uarpool = nullptr;
    }
    for (auto cap_type : m_caps) {
        free(cap_type.second);
    }
    if (m_external_hca_caps) {
        delete m_external_hca_caps;
        m_external_hca_caps = nullptr;
    }
    delete m_dcmd_ctx;
    m_dcmd_ctx = nullptr;
}

uar_collection::uar_collection(dcmd::ctx* ctx)
    : m_mutex()
    , m_ex_uars()
    , m_sh_vc()
    , m_ctx(ctx)
    , m_shared_uar(nullptr)
{
}

uar uar_collection::get_uar(const void* p_key, uar_type type)
{
    uar u = nullptr;
    if (nullptr == p_key) {
        return u;
    }

    std::lock_guard<std::mutex> guard(m_mutex);

    if (SHARED_UAR == type) {
        if (nullptr == m_shared_uar) {
            // allocated shared UAR
            m_shared_uar = allocate();
            if (m_shared_uar) {
                m_sh_vc.push_back(p_key);
            }
        } else {
            // no need to add to shared if already shared.
            auto vit = std::find(m_sh_vc.begin(), m_sh_vc.end(), p_key);
            if (vit == m_sh_vc.end()) {
                m_sh_vc.push_back(p_key);
            }
        }
        return m_shared_uar;

    } else {
        // Exclusive UAR
        auto elem = m_ex_uars.find(p_key);

        if (elem != m_ex_uars.end()) {
            // Already allocated
            return elem->second;
        }
        // there is no UAR for this rq, find free slot

        elem = m_ex_uars.find(0);
        if (elem == m_ex_uars.end()) {
            // No free slots - allocate new uar.
            uar u_new = allocate();
            if (nullptr == u_new) {
                return nullptr;
            }
            u = add_uar(p_key, u_new);
        } else {
            // Move UAR to attached to specific rq
            u = add_uar(p_key, elem->second);
            m_ex_uars.erase(0);
        }
    }
    return u;
}

uar uar_collection::add_uar(const void* p_key, uar u)
{

    auto ret = m_ex_uars.emplace(std::make_pair(p_key, u));
    if (ret->second) {
        return ret->second;
    }
    return nullptr;
}

uar uar_collection::allocate()
{
    dcmd::uar_desc desc = {0};
    return m_ctx->create_uar(&desc);
}

void uar_collection::free(uar u)
{
    delete u;
}

status uar_collection::release_uar(const void* p_key)
{
    if (nullptr == p_key) {
        return DPCP_ERR_INVALID_PARAM;
    }

    std::lock_guard<std::mutex> guard(m_mutex);
    // check first shared
    auto vit = std::find(m_sh_vc.begin(), m_sh_vc.end(), p_key);
    if (vit != m_sh_vc.end()) {
        const auto new_end(remove(begin(m_sh_vc), end(m_sh_vc), p_key));
        m_sh_vc.erase(new_end, end(m_sh_vc));
        return DPCP_OK;
    }
    // Nothing was found, check in exclusive map
    auto it = m_ex_uars.find(p_key);
    if (it != m_ex_uars.end()) {
        // Find UAR, move it to free poll (p_key=0)
        uar u = it->second;
        m_ex_uars.erase(it);
        add_uar(0, u);
    } else {
        return DPCP_ERR_INVALID_PARAM;
    }
    return DPCP_OK;
}

status uar_collection::get_uar_page(const uar u, uar_t& uar_dsc)
{
    if (nullptr == u) {
        return DPCP_ERR_INVALID_PARAM;
    }
    uar_dsc.m_page = u->get_page();
    uar_dsc.m_bf_reg = u->get_reg();
    uar_dsc.m_page_id = u->get_id();
    return DPCP_OK;
}

uar_collection::~uar_collection()
{
    delete m_shared_uar;
    log_trace("~uar_collection shared=%zd ex=%zd\n", m_sh_vc.size(), m_ex_uars.size());
    m_ex_uars.clear();
    m_sh_vc.clear();
}

enum {
    MLX5_PP_DATA_RATE = 0x0,
    MLX5_PP_WQE_RATE = 0x1,
};

status packet_pacing::create()
{
    uint32_t pp[DEVX_ST_SZ_DW(set_pp_rate_limit_context)] = {};

    DEVX_SET(set_pp_rate_limit_context, &pp, burst_upper_bound, m_attr.burst_sz);
    DEVX_SET(set_pp_rate_limit_context, &pp, typical_packet_size, m_attr.packet_sz);
    DEVX_SET(set_pp_rate_limit_context, &pp, rate_limit, m_attr.sustained_rate);
    DEVX_SET(set_pp_rate_limit_context, &pp, rate_mode, MLX5_PP_DATA_RATE);
    m_pp_handle = devx_alloc_pp((ctx_handle)get_ctx()->get_context(), pp, sizeof(pp), 0);
    if (IS_ERR(m_pp_handle)) {
        log_error("alloc_pp failed, errno %d for rate %u burst %u packet_sz %u\n", errno,
                  m_attr.sustained_rate, m_attr.burst_sz, m_attr.packet_sz);
        return DPCP_ERR_CREATE;
    }
    m_index = get_pp_index(m_pp_handle);
    log_trace("packet pacing index: %u for rate: %d burst: %d packet_sz: %d\n", m_index,
              m_attr.sustained_rate, m_attr.burst_sz, m_attr.packet_sz);
    return DPCP_OK;
}

status adapter::create_parser_graph_node(const parser_graph_node_attr& attributes,
                                         parser_graph_node*& out_parser_graph_node)
{
    auto caps = m_external_hca_caps;

    bool general_object_types_parse_graph_node_supported =
        caps->general_object_types_parse_graph_node;
    if (!general_object_types_parse_graph_node_supported) {
        log_error("The adapter doesn't support the creation of general object parse graph node");
        return DPCP_ERR_NO_SUPPORT;
    }

    bool parse_graph_header_length_mode_supported =
        ((1 << attributes.header_length_mode.to_ulong()) & caps->parse_graph_header_length_mode) !=
        0;
    if (!parse_graph_header_length_mode_supported) {
        log_error("The header_length_mode attribute is not supported");
        return DPCP_ERR_NO_SUPPORT;
    }

    bool parse_graph_header_length_field_mask_width_supported;
    if (attributes.header_length_field_mask == 0) {
        parse_graph_header_length_field_mask_width_supported = true;
    } else {
        parse_graph_header_length_field_mask_width_supported =
            (attributes.header_length_field_mask &
             static_cast<uint32_t>((1 << caps->parse_graph_header_length_field_mask_width) - 1)) !=
            0;
    }
    if (!parse_graph_header_length_field_mask_width_supported) {
        log_error("The header_length_field_mask attribute"
                  " uses more than the supported number of bits");
        return DPCP_ERR_NO_SUPPORT;
    }

    bool max_parse_graph_header_length_base_value_supported =
        (attributes.header_length_base_value <= caps->max_parse_graph_header_length_base_value);
    if (!max_parse_graph_header_length_base_value_supported) {
        log_error("The header_length_base_value attribute "
                  "exceeds the maximum value supported");
        return DPCP_ERR_NO_SUPPORT;
    }

    // Supporting 1 input arc now, using vector to be able to extend in the future
    // without breaking backward compatibility.
    constexpr uint8_t max_in_arcs = 1;
    bool max_num_parse_graph_arc_in_supported =
        (attributes.in_arcs.size() <=
         std::min<uint8_t>(caps->max_num_parse_graph_arc_in, max_in_arcs));
    if (!max_num_parse_graph_arc_in_supported) {
        log_error("The number of in_arcs attribute exceeds the maximum value supported");
        return DPCP_ERR_NO_SUPPORT;
    }

    constexpr int first_arc = 0;
    bool parse_graph_node_in_supported =
        (1 << attributes.in_arcs[first_arc].arc_parse_graph_node & caps->parse_graph_node_in) != 0;
    if (!parse_graph_node_in_supported) {
        log_error("The arc_parse_graph_node attribute is not supported");
        return DPCP_ERR_NO_SUPPORT;
    }

    bool max_num_parse_graph_flow_match_sample_supported =
        (attributes.samples.size() <= caps->max_num_parse_graph_flow_match_sample);
    if (!max_num_parse_graph_flow_match_sample_supported) {
        log_error("The number of samples attribute exceeds the maximum value supported");
        return DPCP_ERR_NO_SUPPORT;
    }

    for (auto& sample : attributes.samples) {
        if (!sample.enabled) {
            continue;
        }

        bool max_parse_graph_flow_match_sample_field_base_offset_value_supported =
            (sample.field_base_offset <=
             caps->max_parse_graph_flow_match_sample_field_base_offset_value);
        if (!max_parse_graph_flow_match_sample_field_base_offset_value_supported) {
            log_error("The field_base_offset attribute of the sample"
                      " exceeds the maximum value supported");
            return DPCP_ERR_NO_SUPPORT;
        }

        bool parse_graph_flow_match_sample_offset_mode_supported =
            ((1 << sample.offset_mode.to_ulong()) & caps->parse_graph_header_length_mode) != 0;
        if (!parse_graph_flow_match_sample_offset_mode_supported) {
            log_error("The offset_mode attribute of the sample is not supported");
            return DPCP_ERR_NO_SUPPORT;
        }

        bool parse_graph_flow_match_sample_id_in_out_supported =
            sample.field_id == 0 ? true : caps->parse_graph_flow_match_sample_id_in_out;
        if (!parse_graph_flow_match_sample_id_in_out_supported) {
            log_error("Setting field_id attribute of the sample is not supported");
            return DPCP_ERR_NO_SUPPORT;
        }
    }

    parser_graph_node* _parser_graph_node =
        new (std::nothrow) parser_graph_node(get_ctx(), attributes);
    if (_parser_graph_node == nullptr) {
        return DPCP_ERR_NO_MEMORY;
    }

    status ret = _parser_graph_node->create();
    if (ret != DPCP_OK) {
        delete _parser_graph_node;
        return DPCP_ERR_CREATE;
    }

    out_parser_graph_node = _parser_graph_node;

    return DPCP_OK;
}

} // namespace dpcp
