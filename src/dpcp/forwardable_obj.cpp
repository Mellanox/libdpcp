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

forwardable_obj::forwardable_obj(dcmd::ctx* ctx)
    : obj(ctx)
{
}

status forwardable_obj::get_fwd_desc(dcmd::fwd_dst_desc& desc)
{
    status ret = DPCP_OK;

    desc.type = get_fwd_type();

    ret = get_handle(desc.handle);
    if (ret != DPCP_OK) {
        log_error("Forwardable Object, failed to get destination handle\n");
        return ret;
    }

    ret = get_id(desc.id);
    if (ret != DPCP_OK) {
        log_error("Forwardable Object, failed to get destination id\n");
        return ret;
    }

    return DPCP_OK;
}

} // namespace dpcp
