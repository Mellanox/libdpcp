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

#include <algorithm>

#include "utils/os.h"
#include "dpcp/internal.h"

namespace dpcp {

using namespace std;

flow_rule::flow_rule(dcmd::ctx* ctx, uint16_t priority, match_params& match_criteria)
    : obj(ctx)
    , m_mask(match_criteria)
    , m_value()
    , m_dst_tir()
    , m_flow(nullptr)
    , m_flow_id(0)
    , m_priority(priority)
    , m_changed(false)
{
}

flow_rule::~flow_rule()
{
    revoke_settings();
    m_dst_tir.clear();
}

status flow_rule::set_match_value(match_params& val)
{
    m_value = val;
    m_changed = true;
    return DPCP_OK;
}

status flow_rule::get_match_value(match_params& val)
{
    val = m_value;
    if (m_changed && m_flow) {
        return DPCP_ERR_NOT_APPLIED;
    }
    return DPCP_OK;
}

status flow_rule::get_priority(uint16_t& priority)
{
    priority = m_priority;
    return DPCP_OK;
}

status flow_rule::set_flow_id(uint32_t flow_id)
{
    if (0xFFFFF < flow_id) {
        return DPCP_ERR_OUT_OF_RANGE;
    }
    m_flow_id = flow_id;
    m_changed = true;
    return DPCP_OK;
}

status flow_rule::get_flow_id(uint32_t& flow_id)
{
    flow_id = m_flow_id;
    if (m_changed && m_flow) {
        return DPCP_ERR_NOT_APPLIED;
    }
    return DPCP_OK;
}

status flow_rule::get_num_tirs(uint32_t& num_tirs)
{
    num_tirs = (uint32_t)m_dst_tir.size();
    return DPCP_OK;
}

status flow_rule::get_tir(uint32_t index, tir*& tr)
{
    if (index > m_dst_tir.size() - 1) {
        return DPCP_ERR_OUT_OF_RANGE;
    }
    tr = (tir*)m_dst_tir[index];
    return DPCP_OK;
}

status flow_rule::add_dest_tir(tir* tr)
{
    if (nullptr == tr) {
        return DPCP_ERR_INVALID_PARAM;
    }
    m_dst_tir.push_back(tr);
    m_changed = true;
    return DPCP_OK;
}

status flow_rule::remove_dest_tir(const tir* dst_tir)
{
    auto tit = find(m_dst_tir.begin(), m_dst_tir.end(), dst_tir);
    if (tit != m_dst_tir.end()) {
        const auto new_end(remove(begin(m_dst_tir), end(m_dst_tir), dst_tir));
        m_dst_tir.erase(new_end, end(m_dst_tir));
        return DPCP_OK;
    }
    m_changed = true;
    return DPCP_ERR_INVALID_PARAM;
}

static inline void copy_ether_mac(uint8_t* dst, const uint8_t* src)
{
    *(uint32_t*)dst = *(const uint32_t*)src;
    *(uint16_t*)(dst + 4) = *(const uint16_t*)(src + 4);
}

struct prm_match_params {
    size_t buf_sz;
    uint32_t buf[DEVX_ST_SZ_DW(fte_match_param)];
};

status flow_rule::apply_settings()
{
    dcmd::ctx* ctx = get_ctx();
    if (nullptr == ctx) {
        log_error("Context is unknown\n");
        return DPCP_ERR_NO_CONTEXT;
    }
    if (0 == m_dst_tir.size()) {
        log_error("Not TIRs sets to apply flow_rule\n");
        return DPCP_ERR_NOT_APPLIED;
    }
    // Set mask (match criteria)
    prm_match_params mask;

    memset(&mask, 0, sizeof(mask));
    mask.buf_sz = sizeof(mask.buf);

    log_trace("sz: %zd ethertype: 0x%x vlan_id: 0x%x protocol: 0x%x ip_version: %x\n", mask.buf_sz,
              m_mask.ethertype, m_mask.vlan_id, m_mask.protocol, m_mask.ip_version);
    log_trace("src_port: 0x%x dst_port: 0x%x src_ip: 0x%x dst_ip: 0x%x\n", m_mask.src_port,
              m_mask.dst_port, m_mask.src_ip, m_mask.dst_ip);
    void* prm_mc = DEVX_ADDR_OF(fte_match_param, &mask.buf, outer_headers);
    DEVX_SET(fte_match_set_lyr_2_4, prm_mc, ethertype, m_mask.ethertype);
    if (m_mask.vlan_id) {
        // Vlan_id is dynamic, will be set only when mask set
        DEVX_SET(fte_match_set_lyr_2_4, prm_mc, cvlan_tag, 0x1);
        DEVX_SET(fte_match_set_lyr_2_4, prm_mc, first_vid, m_mask.vlan_id);
    }
    DEVX_SET(fte_match_set_lyr_2_4, prm_mc, ip_protocol, m_mask.protocol);
    if (m_mask.ip_version) {
        // IP version is dynamic too.
        DEVX_SET(fte_match_set_lyr_2_4, prm_mc, ip_version, m_mask.ip_version);
    }

#if !defined(KERNEL_PRM)
    uint64_t dmac = 0;
    memcpy(&dmac, m_mask.dst_mac, sizeof(dmac));
    // will send DST_MAC only if was set in mask
    bool dmac_set = dmac ? true : false;
    if (dmac_set) {
        copy_ether_mac((uint8_t*)DEVX_ADDR_OF(fte_match_set_lyr_2_4, prm_mc, dmac_47_16),
                       m_mask.dst_mac);
    }
    void* p_src_ip = DEVX_ADDR_OF(fte_match_set_lyr_2_4, prm_mc, src_ipv4_src_ipv6);
    DEVX_SET(ipv4_layout, p_src_ip, ipv4, m_mask.src_ip);
    void* p_dst_ip = DEVX_ADDR_OF(fte_match_set_lyr_2_4, prm_mc, dst_ipv4_dst_ipv6);
    DEVX_SET(ipv4_layout, p_dst_ip, ipv4, m_mask.dst_ip);
#else
    DEVX_SET(fte_match_set_lyr_2_4, prm_mc, src_ip[3], m_mask.src_ip);
    DEVX_SET(fte_match_set_lyr_2_4, prm_mc, dst_ip[3], m_mask.dst_ip);
#endif
    //
    // Set match values
    prm_match_params values;
    memset(&values, 0, sizeof(values));
    values.buf_sz = sizeof(values.buf);
    uint32_t* prm_mv = (uint32_t*)&values.buf;
    DEVX_SET(fte_match_set_lyr_2_4, prm_mv, ethertype, m_value.ethertype);
    if (m_mask.vlan_id) {
        // Vlan_id is dynamic, will be set only when mask set
        DEVX_SET(fte_match_set_lyr_2_4, prm_mv, cvlan_tag, 0x1);
        DEVX_SET(fte_match_set_lyr_2_4, prm_mv, first_vid, m_value.vlan_id);
    }
    DEVX_SET(fte_match_set_lyr_2_4, prm_mv, ip_protocol, m_value.protocol);
    if (m_mask.ip_version) {
        // IP version is dynamic too.
        DEVX_SET(fte_match_set_lyr_2_4, prm_mv, ip_version, m_value.ip_version);
    }

    if (m_value.protocol != 6U) { // If not TCP(6)
        DEVX_SET(fte_match_set_lyr_2_4, prm_mc, udp_dport, m_mask.dst_port);
        DEVX_SET(fte_match_set_lyr_2_4, prm_mc, udp_sport, m_mask.src_port);

        DEVX_SET(fte_match_set_lyr_2_4, prm_mv, udp_dport, m_value.dst_port);
        DEVX_SET(fte_match_set_lyr_2_4, prm_mv, udp_sport, m_value.src_port);
    } else {
        DEVX_SET(fte_match_set_lyr_2_4, prm_mc, tcp_dport, m_mask.dst_port);
        DEVX_SET(fte_match_set_lyr_2_4, prm_mc, tcp_sport, m_mask.src_port);

        DEVX_SET(fte_match_set_lyr_2_4, prm_mv, tcp_dport, m_value.dst_port);
        DEVX_SET(fte_match_set_lyr_2_4, prm_mv, tcp_sport, m_value.src_port);
    }

#if !defined(KERNEL_PRM)
    if (dmac_set) {
        // Vlan_id is dynamic, will be set only when mask set
        copy_ether_mac((uint8_t*)DEVX_ADDR_OF(fte_match_set_lyr_2_4, prm_mv, dmac_47_16),
                       m_value.dst_mac);
        uint8_t* d = m_value.dst_mac;
        log_trace("dmac [%x:%x:%x:%x:%x:%x]\n", d[0], d[1], d[2], d[3], d[4], d[5]);
    }
    p_src_ip = DEVX_ADDR_OF(fte_match_set_lyr_2_4, prm_mv, src_ipv4_src_ipv6);
    DEVX_SET(ipv4_layout, p_src_ip, ipv4, m_value.src_ip);
    p_dst_ip = DEVX_ADDR_OF(fte_match_set_lyr_2_4, prm_mv, dst_ipv4_dst_ipv6);
    DEVX_SET(ipv4_layout, p_dst_ip, ipv4, m_value.dst_ip);
#else
    DEVX_SET(fte_match_set_lyr_2_4, prm_mv, src_ip[3], m_value.src_ip);
    DEVX_SET(fte_match_set_lyr_2_4, prm_mv, dst_ip[3], m_value.dst_ip);
#endif
    //
    // Fill parameters for dcmd::flow
    struct dcmd::flow_desc dcmd_flow;
    dcmd_flow.match_criteria = (dcmd::flow_match_parameters*)&mask;
    dcmd_flow.match_value = (dcmd::flow_match_parameters*)&values;
    dcmd_flow.priority = m_priority;
    dcmd_flow.flow_id = m_flow_id;
    dcmd_flow.num_dst_tir = m_dst_tir.size();
    // we would need tir objects for Linux and tir ids for Windows which fill in
    // mlx5_ifc_dest_format_struct_bits
    uintptr_t* dst_tir_obj = new (std::nothrow) uintptr_t[dcmd_flow.num_dst_tir];
    auto dst_formats = new (std::nothrow) mlx5_ifc_dest_format_struct_bits[dcmd_flow.num_dst_tir];
    if (!dst_tir_obj || !dst_formats) {
        delete[] dst_formats;
        delete[] dst_tir_obj;
        return DPCP_ERR_NO_MEMORY;
    }
    memset(dst_formats, 0, DEVX_ST_SZ_BYTES(dest_format_struct) * dcmd_flow.num_dst_tir);

    for (uint32_t i = 0; i < dcmd_flow.num_dst_tir; i++) {
        if (DPCP_OK == m_dst_tir[i]->get_handle(dst_tir_obj[i])) {
            uint32_t tir_id = 0;
            m_dst_tir[i]->get_id(tir_id);
            DEVX_SET(dest_format_struct, dst_formats + i, destination_type,
                     MLX5_FLOW_DESTINATION_TYPE_TIR);
            DEVX_SET(dest_format_struct, dst_formats + i, destination_id, tir_id);
            uint32_t ud_id = DEVX_GET(dest_format_struct, dst_formats + i, destination_id);
            log_trace("tir_id[%i] 0x%x (0x%x)\n", i, tir_id, ud_id);
        }
    }
    dcmd_flow.dst_tir_obj = (obj_handle*)dst_tir_obj;
    dcmd_flow.dst_formats = dst_formats;

    m_flow = ctx->create_flow(&dcmd_flow);

    m_changed = false;
    delete[] dst_formats;
    delete[] dst_tir_obj;

    if (nullptr == m_flow) {
        return DPCP_ERR_CREATE;
    }
    return DPCP_OK;
}

status flow_rule::revoke_settings()
{
    if (nullptr != m_flow) {
        delete m_flow;
        m_flow = nullptr;
    }
    return DPCP_OK;
}
} // namespace dpcp
