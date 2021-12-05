/*
 Copyright (C) Mellanox Technologies, Ltd. 2019-2020. ALL RIGHTS RESERVED.

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

#include "dpcp/internal.h"
#include "utils/os.h"

namespace dpcp {

comp_channel::comp_channel(adapter* ad)
    : eq(ad->get_ctx())
{
    try {
        m_cc = new dcmd::compchannel((ctx_handle)(eq::get_ctx())->get_context());
    } catch (...) {
        log_error("Can't create compchannel\n");
    }
}

comp_channel::~comp_channel()
{
    delete m_cc;
}

status comp_channel::bind(cq& in_cq)
{
    uintptr_t obj_h;
    status ret = in_cq.get_handle(obj_h);
    if (ret) {
        return ret;
    }
    int err = m_cc->bind((cq_handle)obj_h, false);
    if (err) {
        return DPCP_ERR_NO_DEVICES;
    }
    return DPCP_OK;
}

status comp_channel::unbind(cq& to_unbind)
{
    UNUSED(to_unbind); // TODO: add processing
    if (m_cc->unbind()) {
        return DPCP_ERR_NO_CONTEXT;
    }
    return DPCP_OK;
}

status comp_channel::get_comp_channel(event_channel*& ch)
{
    int ret = m_cc->get_comp_channel(ch);
    if (ret) {
        return DPCP_ERR_NO_CONTEXT;
    }
    return DPCP_OK;
}

status comp_channel::request(cq& for_cq, eq_context& eq_ctx)
{
    UNUSED(for_cq); // TODO: add processing
    dcmd::compchannel_ctx cc_ctx;
    cc_ctx.overlapped = eq_ctx.p_overlapped;
    int ret = m_cc->request(cc_ctx);
    if (ret) {
        return DPCP_ERR_NO_CONTEXT;
    }
    eq_ctx.num_eqe = cc_ctx.eqe_nums;
    return DPCP_OK;
}

status comp_channel::flush(cq& for_cq)
{
    UNUSED(for_cq); // TODO: add processing
    m_cc->flush(0);
    return DPCP_OK;
}

} // namespace dpcp
