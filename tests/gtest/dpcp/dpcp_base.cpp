/*
 * Copyright (c) 2019-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

adapter* dpcp_base::OpenAdapter(uint32_t vendor_part_id)
{
    size_t num = 0;

    status ret = pr->get_instance(pr);

    ret = pr->get_adapter_info_lst(nullptr, num);

    adapter_info* temp_p_ainfo = new (std::nothrow) adapter_info[num];

    ret = pr->get_adapter_info_lst(temp_p_ainfo, num);
    if (ret != DPCP_OK) {
        return nullptr;
    }

    adapter_info* ai = temp_p_ainfo;
    bool adapter_found = false;
    for (int i = 0; i < (int)num; i++) {
        ai = temp_p_ainfo + i;

        // Ignore unsupported devices
        if ((ai->vendor_id != VendorIdMellanox) && (ai->vendor_id != PCIVendorIdMellanox)) {
            continue;
        }

        // Select device requested in command line
        if (gtest_conf.adapter[0] != '\0') {
            if (!strcmp(ai->name.c_str(), gtest_conf.adapter) &&
                    (vendor_part_id == 0 || ai->vendor_part_id == vendor_part_id)) {
                adapter_found = true;
                break;
            }
        } else {
            // Find the first adapter not older than ConnectX-5
            // or explicitly requested
            if ((vendor_part_id == 0 && ai->vendor_part_id >= DevPartIdConnectX5) ||
                    (ai->vendor_part_id == vendor_part_id)) {
                adapter_found = true;
                break;
            }
        }
    }
    if (!adapter_found) {
        return nullptr;
    }
    adapter* ad = nullptr;
    ret = pr->open_adapter(ai->id, ad);
    if (DPCP_OK == ret) {
        log_trace("selected adapter: name: %s pci: 0x%x:0x%x\n", ai->name.c_str(), ai->vendor_id, ai->vendor_part_id);
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

    log_trace("cq %p buf %p cq_sz %d cqe_cz %d cqn 0x%x\n", cq, dv_cq->buf, dv_cq->cq_size,
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
    log_trace("rq_num %u wqe_sz %zu\n", rqp.rq_num, rqp.wqe_sz);
    rqp.rq_at.cqn = cqd.cqn;
    rqp.rq_at.wqe_num = rqp.rq_num;
    rqp.rq_at.wqe_sz = rqp.wqe_sz;

    striding_rq* srq = nullptr;
    ret = ad->create_striding_rq(rqp.rq_at, srq);
    if (DPCP_OK != ret) {
        return nullptr;
    }
    return srq;
}

void dpcp_base::TearDown()
{
}
