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

#include "action.h"
#include "dcmd/dcmd.h"

namespace dcmd {

action_fwd::action_fwd(const std::vector<fwd_dst_desc>& dests)
    : base_action_fwd(dests)
{
    size_t num_dest_obj = m_dests.size();
    std::unique_ptr<uintptr_t[]> dst_obj_handls_guard(new uintptr_t[num_dest_obj]);
    uintptr_t* dst_obj_handls = dst_obj_handls_guard.get();

    for (size_t i = 0; i < num_dest_obj; i++) {
        dst_obj_handls[i] = m_dests[i].handle;
    }

    m_dst_obj_handls = std::move(dst_obj_handls_guard);
}

int action_fwd::apply(dcmd::flow_desc& desc) const
{
    desc.dst_obj = reinterpret_cast<obj_handle*>(m_dst_obj_handls.get());
    desc.num_dst_obj = m_dests.size();
    return DCMD_EOK;
}

} /* namespace dcmd */
