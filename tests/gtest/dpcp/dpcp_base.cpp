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

#include "common/def.h"
#include "common/log.h"
#include "common/base.h"

#include "dpcp_base.h"

void dpcp_base::SetUp()
{
    errno = EOK;
    m_rqp = {{2048, 16384, 0, 0}, 4, 0};

    m_rqp.wqe_sz = m_rqp.rq_at.buf_stride_num * m_rqp.rq_at.buf_stride_sz / 16; // in DS (16B)
}

adapter* dpcp_base::OpenAdapter()
{
    size_t num = 0;

    status ret = pr->get_instance(pr);

    ret = pr->get_adapter_info_lst(nullptr, num);

    adapter_info* p_ainfo = new (std::nothrow) adapter_info[num];

    ret = pr->get_adapter_info_lst(p_ainfo, num);
    if (ret != DPCP_OK) {
        return nullptr;
    }

    adapter_info* ai = p_ainfo;
    adapter* ad = nullptr;
    ret = pr->open_adapter(ai->id, ad);
    if (DPCP_OK == ret) {
        return ad;
    }

    return nullptr;
}

#if defined(__linux__)
int dpcp_base::create_cq(adapter* ad, cq_data* dv_cq)
{
    ibv_context* ctx = (ibv_context*)ad->get_ibv_context();
    errno = 0;
    uint32_t cq_sz = 4096 * 4;
    struct ibv_cq* cq = ibv_create_cq(ctx, cq_sz, nullptr, nullptr, 0);
    if (!cq) {
        log_error("failed creating CQ errno=0x%x", errno);
        return -1;
    }
    mlx5dv_obj obj;
    struct mlx5dv_cq out;
    obj.cq.in = cq;
    obj.cq.out = &out;

    int ret = mlx5dv_init_obj(&obj, MLX5DV_OBJ_CQ);
    if (ret) {
        log_error("Failed getting CQ attributes ret=0x%x\n", ret);
        return -1;
    }
    dv_cq->buf = out.buf;
    dv_cq->cq_size = out.cqe_cnt;
    dv_cq->cqe_size = out.cqe_size;
    dv_cq->cqn = out.cqn;
    dv_cq->dbrec = out.dbrec;
    dv_cq->p_cq_ci = 0;

    log_trace("cq 0x%lx buf 0x%lx cq_sz %d cqe_cz %d cqn 0x%x\n", cq, dv_cq->buf, dv_cq->cq_size,
              dv_cq->cqe_size, dv_cq->cqn);
    return 0;
}
#else
int dpcp_base::create_cq(adapter* ad, cq_data* dv_cq)
{
    errno = 0;
    uint32_t eqn = 0;
    status ret = ad->query_eqn(eqn);
    if (DPCP_OK != ret) {
        log_error("failed to query EQn ret=0x%x errno=0x%x", ret, errno);
        return ret;
    }
    uint32_t cq_sz = 4096 * 4;
    std::bitset<ATTR_CQ_MAX_CNT_FLAG> flags;
    flags.set(ATTR_CQ_NONE_FLAG);
    std::bitset<CQ_ATTR_MAX_CNT> cq_attr_use;
    cq_attr_use.set(CQ_SIZE);
    cq_attr_use.set(CQ_EQ_NUM);
    cq_attr attr = {cq_sz, eqn, {0, 0}};
    attr.flags = flags;
    attr.cq_attr_use = cq_attr_use;
    cq* pcq = nullptr;
    ret = ad->create_cq(attr, pcq);
    if (!pcq || (DPCP_OK != ret)) {
        log_error("failed creating CQ errno=0x%x", errno);
        return -1;
    }
    ret = pcq->get_cq_buf(dv_cq->buf);
    ret = pcq->get_dbrec(dv_cq->dbrec);
    ret = pcq->get_cqe_num(dv_cq->cq_size);
    ret = pcq->get_id(dv_cq->cqn);
    dv_cq->cqe_size = 64;
    dv_cq->p_cq_ci = 0;

    log_trace("cq 0x%p buf 0x%p cq_sz %d cqe_cz %d cqn 0x%x\n", pcq, dv_cq->buf, dv_cq->cq_size,
              dv_cq->cqe_size, dv_cq->cqn);
    return 0;
}
#endif

striding_rq* dpcp_base::open_str_rq(adapter* ad, rq_params& rqp)
{
    cq_data cqd = {};
    status ret = (status)create_cq(ad, &cqd);
    if (DPCP_OK != ret || (0 == cqd.cqn)) {
        return nullptr;
    }
    log_trace("rq_num %d wqe_sz %d\n", rqp.rq_num, rqp.wqe_sz);
    rqp.rq_at.cqn = cqd.cqn;
    striding_rq* srq = nullptr;
    ret = ad->create_striding_rq(rqp.rq_at, rqp.rq_num, rqp.wqe_sz, srq);
    return srq;
}

void dpcp_base::TearDown()
{
}
