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
#pragma once
#ifndef TESTS_GTEST_DPCP_BASE_H_
#define TESTS_GTEST_DPCP_BASE_H_

#include "src/dcmd/dcmd.h"
#include "src/api/dpcp.h"
#include "src/dpcp/internal.h"

using namespace dpcp;

struct cq_data {
    void* buf;
    uint32_t cq_size;
    uint32_t cqe_cnt;
    uint32_t cqe_size;
    uint32_t cqn;
    uint32_t p_cq_ci;
    uint32_t* dbrec;
};

struct rq_params {
    rq_attr rq_at;
    uint32_t rq_num;
    uint32_t wqe_sz;
};

/**
 * DCMD Base class for tests
 */
class dpcp_base : public testing::Test, public test_base {
    provider* pr;
    adapter_info* p_ainfo;

protected:
    rq_params m_rqp;

    virtual void SetUp();
    adapter* OpenAdapter();
    int create_cq(adapter* ad, cq_data* dv_cq);
    striding_rq* open_str_rq(adapter* ad, rq_params& rqp);
    virtual void TearDown();
};

#endif /* TESTS_GTEST_DPCP_BASE_H_ */