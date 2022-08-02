/*
 * Copyright © 2019-2022 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Nvidia Corporation and its affiliates
 * (the "Company") and all right, title, and interest in and to the software
 * product, including all associated intellectual property rights, are and
 * shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
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

    return DPCP_OK;
}

flow_matcher::flow_matcher(const flow_matcher_attr& attr)
    : m_attr(attr)
{
}

} // namespace dpcp
