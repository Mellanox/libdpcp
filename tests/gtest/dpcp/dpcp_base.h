/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2019-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef TESTS_GTEST_DPCP_BASE_H_
#define TESTS_GTEST_DPCP_BASE_H_

#include "src/dcmd/dcmd.h"
#include "src/api/dpcp.h"
#include "src/dpcp/internal.h"

using namespace dpcp;

const uint32_t VendorIdMellanox = 0x02c9;
const uint32_t PCIVendorIdMellanox = 0x15b3;
const uint32_t DevPartIdConnectX5 = 0x1017;
const uint32_t DevPartIdConnectX6DX = 0x101D;
const uint32_t DevPartIdBlueField = 0xA2D6;

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
    size_t wqe_sz;
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
    adapter* OpenAdapter(uint32_t vendor_part_id = 0);
    int create_cq(adapter* ad, cq_data* dv_cq);
    striding_rq* open_str_rq(adapter* ad, rq_params& rqp);
    virtual void TearDown();
};

#endif /* TESTS_GTEST_DPCP_BASE_H_ */
