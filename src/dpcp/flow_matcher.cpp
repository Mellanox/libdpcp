/*
 * Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include "dpcp/internal.h"
#include "utils/os.h"

namespace dpcp {

// TODO: should replace the definition in fr.cpp
static inline void copy_ether_mac(uint8_t* dst, const uint8_t* src)
{
    *(uint32_t*)dst = *(const uint32_t*)src;
    *(uint16_t*)(dst + 4) = *(const uint16_t*)(src + 4);
}

flow_matcher::flow_matcher(const flow_matcher_attr& attr)
    : m_attr(attr)
{
}

status flow_matcher::set_outer_header_lyr_2_fields(void* outer,
                                                   const match_params_ex& match_value) const
{
    const match_params_lyr_2& match_crateria_lyr2(m_attr.match_criteria.match_lyr2);
    const match_params_lyr_2& match_value_lyr2(match_value.match_lyr2);
    uint8_t zero_mac[sizeof(match_crateria_lyr2.dst_mac)] = {0};

    if (memcmp(match_crateria_lyr2.dst_mac, zero_mac, sizeof(zero_mac))) {
        copy_ether_mac((uint8_t*)DEVX_ADDR_OF(fte_match_set_lyr_2_4, outer, dmac_47_16),
                       match_value_lyr2.dst_mac);
    }
    if (memcmp(match_crateria_lyr2.src_mac, zero_mac, sizeof(zero_mac))) {
        copy_ether_mac((uint8_t*)DEVX_ADDR_OF(fte_match_set_lyr_2_4, outer, smac_47_16),
                       match_value_lyr2.src_mac);
    }
    if (match_crateria_lyr2.ethertype) {
        DEVX_SET(fte_match_set_lyr_2_4, outer, ethertype, match_value_lyr2.ethertype);
    }
    if (match_crateria_lyr2.first_vlan_id) {
        DEVX_SET(fte_match_set_lyr_2_4, outer, first_vid, match_value_lyr2.first_vlan_id);
        DEVX_SET(fte_match_set_lyr_2_4, outer, cvlan_tag, 0x1);
    }

    return DPCP_OK;
}

status flow_matcher::set_outer_header_lyr_3_fields(void* outer,
                                                   const match_params_ex& match_value) const
{
    const match_params_lyr_3& match_crateria_lyr3(m_attr.match_criteria.match_lyr3);
    const match_params_lyr_3& match_value_lyr3(match_value.match_lyr3);

    if (match_crateria_lyr3.dst_ip) {
        DEVX_SET(fte_match_set_lyr_2_4, outer, dst_ipv4_dst_ipv6.ipv4_layout.ipv4,
                 match_value_lyr3.dst_ip);
    }
    if (match_crateria_lyr3.src_ip) {
        DEVX_SET(fte_match_set_lyr_2_4, outer, src_ipv4_src_ipv6.ipv4_layout.ipv4,
                 match_value_lyr3.src_ip);
    }
    if (match_crateria_lyr3.ip_protocol) {
        DEVX_SET(fte_match_set_lyr_2_4, outer, ip_protocol, match_value_lyr3.ip_protocol);
    }
    if (match_crateria_lyr3.ip_version) {
        DEVX_SET(fte_match_set_lyr_2_4, outer, ip_version, match_value_lyr3.ip_version);
    }

    return DPCP_OK;
}

status flow_matcher::set_outer_header_lyr_4_fields(void* outer,
                                                   const match_params_ex& match_value) const
{
    const match_params_lyr_4& match_crateria_lyr4(m_attr.match_criteria.match_lyr4);
    const match_params_lyr_4& match_value_lyr4(match_value.match_lyr4);

    switch (match_crateria_lyr4.type) {
    case match_params_lyr_4_type::NONE:
        break;
    case match_params_lyr_4_type::UDP:
        if (match_crateria_lyr4.dst_port) {
            DEVX_SET(fte_match_set_lyr_2_4, outer, udp_dport, match_value_lyr4.dst_port);
        }
        if (match_crateria_lyr4.src_port) {
            DEVX_SET(fte_match_set_lyr_2_4, outer, udp_sport, match_value_lyr4.src_port);
        }
        break;
    case match_params_lyr_4_type::TCP:
        if (match_crateria_lyr4.dst_port) {
            DEVX_SET(fte_match_set_lyr_2_4, outer, tcp_dport, match_value_lyr4.dst_port);
        }
        if (match_crateria_lyr4.src_port) {
            DEVX_SET(fte_match_set_lyr_2_4, outer, tcp_sport, match_value_lyr4.src_port);
        }
        break;
    default:
        log_error("Flow matcher layer 4 match params of type %d is not supported\n",
                  match_crateria_lyr4.type);
        return DPCP_ERR_NO_SUPPORT;
    }

    return DPCP_OK;
}

status flow_matcher::set_outer_header_fields(void* match_params,
                                             const match_params_ex& match_value) const
{
    status ret = DPCP_OK;
    void* outer = DEVX_ADDR_OF(fte_match_param, match_params, outer_headers);

    // Check if match criteria outer header was set.
    if (!(m_attr.match_criteria_enabled & flow_group_match_criteria_enable::FG_MATCH_OUTER_HDR)) {
        return DPCP_OK;
    }

    ret = set_outer_header_lyr_2_fields(outer, match_value);
    if (ret != DPCP_OK) {
        log_error("Flow matcher failed to set layer 2 fields, ret %d\n", ret);
        return ret;
    }

    ret = set_outer_header_lyr_3_fields(outer, match_value);
    if (ret != DPCP_OK) {
        log_error("Flow matcher failed to set layer 3 fields, ret %d\n", ret);
        return ret;
    }

    ret = set_outer_header_lyr_4_fields(outer, match_value);
    if (ret != DPCP_OK) {
        log_error("Flow matcher failed to set layer 4 fields, ret %d\n", ret);
        return ret;
    }

    return DPCP_OK;
}

status flow_matcher::set_prog_sample_fileds(void* match_params,
                                            const match_params_ex& match_value) const
{
    const match_params_ex& match_criteria(m_attr.match_criteria);
    void* progr = DEVX_ADDR_OF(fte_match_param, match_params, misc_parameters_4);

    // Check if match criteria programmable fields was set.
    if (!(m_attr.match_criteria_enabled &
          flow_group_match_criteria_enable::FG_MATCH_PARSER_FIELDS)) {
        return DPCP_OK;
    }
    if (match_criteria.match_parser_sample_field_vec.size() !=
        match_value.match_parser_sample_field_vec.size()) {
        log_error("Flow matcher not valid programmable fields\n");
        return DPCP_ERR_INVALID_PARAM;
    }

    switch (match_criteria.match_parser_sample_field_vec.size()) {
    case 8:
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_value_7,
                 match_value.match_parser_sample_field_vec[7].val);
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_id_7,
                 match_value.match_parser_sample_field_vec[7].id);
    case 7:
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_value_6,
                 match_value.match_parser_sample_field_vec[6].val);
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_id_6,
                 match_value.match_parser_sample_field_vec[6].id);
    case 6:
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_value_5,
                 match_value.match_parser_sample_field_vec[5].val);
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_id_5,
                 match_value.match_parser_sample_field_vec[5].id);
    case 5:
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_value_4,
                 match_value.match_parser_sample_field_vec[4].val);
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_id_4,
                 match_value.match_parser_sample_field_vec[4].id);
    case 4:
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_value_3,
                 match_value.match_parser_sample_field_vec[3].val);
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_id_3,
                 match_value.match_parser_sample_field_vec[3].id);
    case 3:
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_value_2,
                 match_value.match_parser_sample_field_vec[2].val);
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_id_2,
                 match_value.match_parser_sample_field_vec[2].id);
    case 2:
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_value_1,
                 match_value.match_parser_sample_field_vec[1].val);
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_id_1,
                 match_value.match_parser_sample_field_vec[1].id);
    case 1:
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_value_0,
                 match_value.match_parser_sample_field_vec[0].val);
        DEVX_SET(fte_match_set_misc4, progr, prog_sample_field_id_0,
                 match_value.match_parser_sample_field_vec[0].id);
    case 0:
        break;
    default:
        log_error("Up to 8 programmable fields are supported\n");
        return DPCP_ERR_OUT_OF_RANGE;
    }

    return DPCP_OK;
}

status flow_matcher::apply(void* match_params, const match_params_ex& match_value) const
{
    status ret = set_outer_header_fields(match_params, match_value);
    if (ret != DPCP_OK) {
        return ret;
    }
    ret = set_prog_sample_fileds(match_params, match_value);
    if (ret != DPCP_OK) {
        return ret;
    }
    ret = set_metadata_registers_fields(match_params, match_value);
    if (ret != DPCP_OK) {
        return ret;
    }

    return DPCP_OK;
}

status flow_matcher::set_metadata_registers_fields(void* match_params,
                                                   const match_params_ex& match_value) const
{
    void* metadata_registers = DEVX_ADDR_OF(fte_match_param, match_params, misc_parameters_2);

    status ret = set_metadata_register_0_field(metadata_registers, match_value);
    if (ret != DPCP_OK) {
        return ret;
    }

    return ret;
}

status flow_matcher::set_metadata_register_0_field(void* metadata_registers,
                                                   const match_params_ex& match_value) const
{
    // Check if match criteria metadata register fields was set.
    if (!(m_attr.match_criteria_enabled &
          flow_group_match_criteria_enable::FG_MATCH_METADATA_REG_C_0)) {
        return DPCP_OK;
    }

    if (m_attr.match_criteria.match_metadata_reg_c_0) {
        DEVX_SET(fte_match_set_misc2, metadata_registers, metadata_reg_c_0,
                 match_value.match_metadata_reg_c_0);
    }

    return DPCP_OK;
}

} // namespace dpcp
