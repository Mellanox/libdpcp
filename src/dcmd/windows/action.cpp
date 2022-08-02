/*
 * Copyright © 2022 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Nvidia Corporation and its affiliates
 * (the "Company") and all right, title, and interest in and to the software
 * product, including all associated intellectual property rights, are and
 * shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 */

#include "stdafx.h"

#include "action.h"

namespace dcmd {

action_fwd::action_fwd(const std::vector<fwd_dst_desc>& dests)
    : base_action_fwd(dests)
{
    size_t num_dest_obj = m_dests.size();
    std::unique_ptr<mlx5_ifc_dest_format_struct_bits[]> dst_formats_guared;
    dst_formats_guared.reset(new mlx5_ifc_dest_format_struct_bits[num_dest_obj]);
    mlx5_ifc_dest_format_struct_bits* dst_formats = dst_formats_guared.get();
    memset(dst_formats, 0, DEVX_ST_SZ_BYTES(dest_format_struct) * num_dest_obj);

    for (size_t i = 0; i < num_dest_obj; i++) {
        DEVX_SET(dest_format_struct, dst_formats + i, destination_type, m_dests[i].type);
        DEVX_SET(dest_format_struct, dst_formats + i, destination_id, m_dests[i].id);
    }

    m_dst_formats = std::move(dst_formats_guared);
}

int action_fwd::apply(dcmd::flow_desc& desc) const
{
    desc.dst_formats = m_dst_formats.get();
    desc.num_dst_obj = m_dests.size();
    return DCMD_EOK;
}

} /* namespace dcmd */
