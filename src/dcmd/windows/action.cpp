/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
