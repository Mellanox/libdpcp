/*
Copyright (C) Mellanox Technologies, Ltd. 2020-2021. ALL RIGHTS RESERVED.

This software product is a proprietary product of Mellanox Technologies, Ltd.
(the "Company") and all right, title, and interest in and to the software
product, including all associated intellectual property rights, are and shall
remain exclusively with the Company.  All rights in or to the software product
are licensed, not sold.  All rights not licensed are reserved.

This software product is governed by the End User License Agreement provided
with the software product.
*/

#include "stdafx.h"
#include "src/dcmd/dcmd.h"
#include "compchannel.h"

using namespace dcmd;

compchannel::compchannel(ctx_handle ctx)
    : m_ctx(ctx)
    , m_handle(INVALID_HANDLE_VALUE)
    , m_binded(false)
{
    if (INVALID_HANDLE_VALUE == m_handle) {
        int err = devx_overlapped_file_open(m_ctx, &m_handle);
        if (err) {
            log_error("overlapped_file_open failed ret=0x%x\n", err);
            throw DCMD_ENOTSUP;
        }
    }
    log_trace("overlapped file handle %p\n", m_handle);
}

int compchannel::bind(obj_handle src_obj, bool solicited)
{
    UNUSED(solicited);
    if (src_obj) {
        m_cq_obj = src_obj;
    } else {
        log_error("event src obj is missing\n");
        return DCMD_EINVAL;
    }

    int err = devx_overlapped_io_enable(m_cq_obj, DEVX_EVENT_TYPE_CQE);
    if (err) {
        log_error("io_enable ret = %d\n", err);
        return DCMD_EIO;
    }
    log_trace("overlapped_io_enable ret = %d\n", err);
    m_binded = true;
    return err;
}

int compchannel::unbind()
{
    // After flush completions will not be generated till next park (query)
    flush(0);
    m_binded = false;
    return DCMD_EOK;
}

int compchannel::get_comp_channel(event_channel*& ch)
{
    ch = &m_handle;
    return DCMD_EOK;
}

int compchannel::request(compchannel_ctx& cc_ctx)
{
    uint32_t num_eqe = 0;
    int err = devx_overlapped_io_park(m_handle, m_cq_obj, DEVX_EVENT_TYPE_CQE, cc_ctx.overlapped,
                                      &num_eqe);
#if defined(DPCP_DEBUG)
    log_trace("m_handle %p err %d num_eqe %d ovl %p\n", m_handle, err, num_eqe, cc_ctx.overlapped);
#endif
    if (err) {
        if (err != -ECANCELED) {
            return DCMD_EIO;
        }

        m_adapter_shutdown = true;
        return DCMD_ENOTSUP;
    }
    cc_ctx.eqe_nums = num_eqe;
    return DCMD_EOK;
}

void compchannel::flush(uint32_t unused)
{
    UNUSED(unused);
    if (m_binded) {
        devx_overlapped_io_flush(m_cq_obj, DEVX_EVENT_TYPE_CQE);
    }
}

compchannel::~compchannel()
{
    flush(0);
    CloseHandle(m_handle);
}
