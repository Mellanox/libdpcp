/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <cstdlib>
#include <cstring>
#include <new>

#include "common/def.h"
#include "common/log.h"
#include "common/sys.h"
#include "common/base.h"

#include "dpcp_base.h"

#include "utils/os.h"

using namespace dpcp;

/*
 * ---------------------------------------------------------------------------
 *  Shared test constants.
 *
 *  Used by make_attr_dma_mmo and the test bodies that assert what it
 *  populated.
 * ---------------------------------------------------------------------------
 */
static constexpr uint8_t  TEST_PORT_NUM = 1; /**< First HCA port. */
static constexpr uint16_t TEST_PKEY_INDEX = 0;
static constexpr uint32_t TEST_USER_INDEX = 0;
static constexpr uint32_t TEST_SQ_WQE_NUM = 1024; /**< Must be power of 2. */
static constexpr uint32_t TEST_SQ_WQE_SZ = 64; /**< One WQEBB. */
static constexpr uint32_t TEST_BOGUS_CQN = 0xDEADBEEF; /**< Unallocated CQN sentinel. */

/*
 * ---------------------------------------------------------------------------
 *  Shared fixture state.
 *
 *  Mirrors the convention used by qp_tests.cpp / sq_tests.cpp:
 *  early cases build the adapter + CQ + a long-lived QP, intermediate
 *  cases consume them, and the final case tears everything down.
 *
 *  Lifecycle:
 *    - s_ad / s_cqd_snd / s_cqd_rcv : opened in ti_03_factory_create_basic;
 *                                     destroyed in ti_27_destroy_idempotent.
 *    - s_qp                         : built in ti_03; destroyed in ti_15.
 *                                     Used by every accessor case in between.
 *    - s_dma_mmo_supported          : set in ti_03 from caps; cases past
 *                                     ti_03 early-return when false so the
 *                                     suite degrades gracefully on HW that
 *                                     lacks the offload.
 * ---------------------------------------------------------------------------
 */
static adapter* s_ad = nullptr; /**< Adapter handle (HW-bound). */
static cq_data s_cqd_snd = {}; /**< Send CQ. */
static cq_data s_cqd_rcv = {}; /**< Receive CQ; DMA MMO QPs have no RQ - feeds the RQ-reject case (ti_14). */
static std::unique_ptr<dma_mmo_qp> s_qp; /**< Shared QP for accessor cases (ti_03..ti_14). */
static bool s_dma_mmo_supported = false; /**< Caps gate flag. */

/**
 * @brief Early-return from the current test if the HCA lacks DMA MMO QP
 *        (RoCE-disabled) support.
 */
#define SKIP_IF_DMA_MMO_UNSUPPORTED()                                          \
    do {                                                                       \
        if (!s_dma_mmo_supported) return;                                      \
    } while (0)

/**
 * @brief class dpcp_dma_mmo_qp - Test fixture for dma_mmo_qp.
 */
class dpcp_dma_mmo_qp : public dpcp_base {
protected:
    void SetUp() override
    {
        if (errno) {
            log_trace("dpcp_dma_mmo_qp::SetUp errno=%d\n", errno);
            errno = EOK;
        }
    }
};

/**
 * @brief Build a baseline dma_mmo_qp_attr for the factory path.
 *
 * Produces a valid SQ-only attr (RQ fields 0). The type forces st=QPST_RC, so
 * a value set here for st is ignored (see ti_13); the RQ fields, by contrast,
 * are rejected by the factory when non-zero (see ti_14).
 *
 * @param [in] cqn_snd Send-side CQ number (must be a valid CQN).
 *
 * @retval The populated dma_mmo_qp_attr.
 */
static dma_mmo_qp_attr make_attr_dma_mmo(uint32_t cqn_snd)
{
    dma_mmo_qp_attr attr = {};
    attr.st = QPST_RC;
    attr.user_index = TEST_USER_INDEX;
    attr.port_num = TEST_PORT_NUM;
    attr.pkey_index = TEST_PKEY_INDEX;
    attr.mtu = QP_MTU_BYTES_4096;
    attr.wq_buf_addr = nullptr;
    attr.db_addr = nullptr;
    attr.cqn_snd = cqn_snd;
    attr.sq_wqe_num = TEST_SQ_WQE_NUM;
    attr.sq_wqe_sz = TEST_SQ_WQE_SZ;
    attr.cqn_rcv = 0;
    attr.rq_wqe_num = 0;
    attr.rq_wqe_sz = 0;
    return attr;
}

/**
 * @brief Group static_* - pure helpers, no QP needed.
 */

/**
 * @test dpcp_dma_mmo_qp.ti_01_static_query_qp_buffer_sizes_sq_only
 * @brief
 *    query_qp_buffer_sizes returns sq_wqe_sz*sq_wqe_num and DB_REC_SIZE
 *    for an SQ-only attr (RQ zeroed).
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_01_static_query_qp_buffer_sizes_sq_only)
{
    qp_attr a = {};
    a.sq_wqe_sz = TEST_SQ_WQE_SZ;
    a.sq_wqe_num = TEST_SQ_WQE_NUM;
    a.rq_wqe_sz = 0;
    a.rq_wqe_num = 0;

    size_t wq_sz = 0;
    size_t db_sz = 0;
    adapter::query_qp_buffer_sizes(a, wq_sz, db_sz);

    EXPECT_EQ(static_cast<size_t>(TEST_SQ_WQE_SZ) * TEST_SQ_WQE_NUM, wq_sz);
    EXPECT_EQ(qp::get_db_rec_sz(), db_sz);
}

/**
 * @test dpcp_dma_mmo_qp.ti_02_static_query_qp_buffer_sizes_sq_rq
 * @brief
 *    query_qp_buffer_sizes sums SQ + RQ contributions for an SQ+RQ attr.
 * @details
 *    Although dma_mmo_qp itself never carries an RQ (ctor zeroes it), the
 *    helper is generic and must handle both shapes.
 */
TEST_F(dpcp_dma_mmo_qp, ti_02_static_query_qp_buffer_sizes_sq_rq)
{
    qp_attr a = {};
    a.sq_wqe_sz = TEST_SQ_WQE_SZ;
    a.sq_wqe_num = TEST_SQ_WQE_NUM;
    a.rq_wqe_sz = 64;
    a.rq_wqe_num = 64;

    size_t wq_sz = 0;
    size_t db_sz = 0;
    adapter::query_qp_buffer_sizes(a, wq_sz, db_sz);

    EXPECT_EQ(static_cast<size_t>(TEST_SQ_WQE_SZ) * TEST_SQ_WQE_NUM + 64U * 64U, wq_sz);
    EXPECT_EQ(qp::get_db_rec_sz(), db_sz);
}

/**
 * @brief Group factory_create_* - successful factory path on shared s_qp.
 */

/**
 * @test dpcp_dma_mmo_qp.ti_03_factory_create_basic
 * @brief
 *    Open adapter, query caps, create CQs, build shared dma_mmo_qp via the
 *    factory. If caps disable the offload, mark the suite for graceful
 *    early-return on subsequent cases.
 * @details
 *    Establishes s_qp for subsequent factory_create_* cases.
 */
TEST_F(dpcp_dma_mmo_qp, ti_03_factory_create_basic)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);
    ASSERT_EQ(DPCP_OK, ad->open());
    s_ad = ad;

    adapter_hca_capabilities caps = {};
    ASSERT_EQ(DPCP_OK, s_ad->get_hca_capabilities(caps));
    s_dma_mmo_supported = caps.dma_mmo_qp_when_roce_disabled_supported;
    if (!s_dma_mmo_supported) {
        log_trace("DMA MMO QP (RoCE-disabled) not supported on this HCA - suite will skip\n");
        return;
    }

    ASSERT_EQ(DPCP_OK, (status)create_cq(s_ad, &s_cqd_snd));
    ASSERT_NE(0U, s_cqd_snd.cqn);
    ASSERT_EQ(DPCP_OK, (status)create_cq(s_ad, &s_cqd_rcv));
    ASSERT_NE(0U, s_cqd_rcv.cqn);

    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);
    dma_mmo_qp* s_qp_raw = nullptr;
    ASSERT_EQ(DPCP_OK, s_ad->create_dma_mmo_qp(a, s_qp_raw));
    s_qp.reset(s_qp_raw);
    ASSERT_TRUE(s_qp != nullptr);
}

/**
 * @test dpcp_dma_mmo_qp.ti_04_factory_create_state_rts
 * @brief
 *    Newly factory-created QP is in QP_RTS (cached state).
 * @details
 *    The factory drives RST -> INIT -> RTR -> RTS before returning.
 */
TEST_F(dpcp_dma_mmo_qp, ti_04_factory_create_state_rts)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_TRUE(s_qp != nullptr);
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, s_qp->get_state(st));
    EXPECT_EQ(QP_RTS, st);
}

/**
 * @test dpcp_dma_mmo_qp.ti_05_factory_create_qpn_nonzero
 * @brief
 *    get_qpn returns non-zero QPN.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_05_factory_create_qpn_nonzero)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_TRUE(s_qp != nullptr);
    uint32_t qpn = 0;
    ASSERT_EQ(DPCP_OK, s_qp->get_qpn(qpn));
    EXPECT_NE(0U, qpn);
}

/**
 * @test dpcp_dma_mmo_qp.ti_06_factory_create_get_sq_wqe_matches_attr
 * @brief
 *    get_sq_wqe_sz / get_sq_wqe_num return the values passed in attr.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_06_factory_create_get_sq_wqe_matches_attr)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_TRUE(s_qp != nullptr);
    uint32_t sz = 0, num = 0;
    ASSERT_EQ(DPCP_OK, s_qp->get_sq_wqe_sz(sz));
    ASSERT_EQ(DPCP_OK, s_qp->get_sq_wqe_num(num));
    EXPECT_EQ(TEST_SQ_WQE_SZ, sz);
    EXPECT_EQ(TEST_SQ_WQE_NUM, num);
}

/**
 * @test dpcp_dma_mmo_qp.ti_07_factory_create_get_rq_zero
 * @brief
 *    DMA MMO QPs have no RQ - get_rq_wqe_sz / get_rq_wqe_num return 0.
 * @details
 *    The ctor zeroes rq_wqe_num / rq_wqe_sz regardless of attr; this case
 *    confirms the override stuck end-to-end via the accessors.
 */
TEST_F(dpcp_dma_mmo_qp, ti_07_factory_create_get_rq_zero)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_TRUE(s_qp != nullptr);
    uint32_t sz = 1, num = 1;
    ASSERT_EQ(DPCP_OK, s_qp->get_rq_wqe_sz(sz));
    ASSERT_EQ(DPCP_OK, s_qp->get_rq_wqe_num(num));
    EXPECT_EQ(0U, sz);
    EXPECT_EQ(0U, num);
}

/**
 * @test dpcp_dma_mmo_qp.ti_08_factory_create_get_cqn_snd
 * @brief
 *    get_cqn_snd matches the CQN supplied in attr.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_08_factory_create_get_cqn_snd)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_TRUE(s_qp != nullptr);
    uint32_t cqn = 0;
    ASSERT_EQ(DPCP_OK, s_qp->get_cqn_snd(cqn));
    EXPECT_EQ(s_cqd_snd.cqn, cqn);
}

/**
 * @test dpcp_dma_mmo_qp.ti_09_factory_create_get_wq_buf_nonnull_aligned
 * @brief
 *    get_wq_buf returns a non-null page-aligned buffer (factory allocated it
 *    via qp::allocate_wq_buf, which uses aligned_alloc(get_page_size(), ...)).
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_09_factory_create_get_wq_buf_nonnull_aligned)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_TRUE(s_qp != nullptr);
    void* buf = nullptr;
    ASSERT_EQ(DPCP_OK, s_qp->get_wq_buf(buf));
    ASSERT_NE(nullptr, buf);
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(buf) % get_page_size());
}

/**
 * @test dpcp_dma_mmo_qp.ti_10_factory_create_get_dbrec_nonnull_zeroed
 * @brief
 *    get_dbrec returns a non-null cacheline-aligned, zero-initialized record.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_10_factory_create_get_dbrec_nonnull_zeroed)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_TRUE(s_qp != nullptr);
    qp_db_rec* dbrec = nullptr;
    ASSERT_EQ(DPCP_OK, s_qp->get_dbrec(dbrec));
    ASSERT_NE(nullptr, dbrec);
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(dbrec) % get_cacheline_size());
    uint8_t zero_db[DB_REC_SIZE] = {};
    EXPECT_EQ(0, memcmp(dbrec, zero_db, qp::get_db_rec_sz()));
}

/**
 * @test dpcp_dma_mmo_qp.ti_11_factory_create_get_uar_page_nonnull
 * @brief
 *    get_uar_page returns a non-null UAR page.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_11_factory_create_get_uar_page_nonnull)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_TRUE(s_qp != nullptr);
    volatile void* page = nullptr;
    ASSERT_EQ(DPCP_OK, s_qp->get_uar_page(page));
    EXPECT_NE(nullptr, const_cast<void*>(page));
}

/**
 * @test dpcp_dma_mmo_qp.ti_12_factory_create_get_bf_reg
 * @brief
 *    get_bf_reg returns DPCP_OK with non-null bf_reg.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_12_factory_create_get_bf_reg)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_TRUE(s_qp != nullptr);
    uint64_t* bf = nullptr;
    ASSERT_EQ(DPCP_OK, s_qp->get_bf_reg(bf));
    EXPECT_NE(nullptr, bf);
}

/**
 * @brief Group factory_attr_* - type forces RC; factory rejects an RQ request.
 */

/**
 * @test dpcp_dma_mmo_qp.ti_13_factory_overrides_service_type_to_rc
 * @brief
 *    Pass attr.st=QPST_UD; factory still creates the QP successfully because
 *    the ctor forces st=QPST_RC before forwarding to qp::create().
 * @details
 *    Lock the override: an RC-only feature like DMA MMO must never see a
 *    non-RC service type reach FW.
 */
TEST_F(dpcp_dma_mmo_qp, ti_13_factory_overrides_service_type_to_rc)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);
    a.st = QPST_UD;
    dma_mmo_qp* q_raw = nullptr;
    ASSERT_EQ(DPCP_OK, s_ad->create_dma_mmo_qp(a, q_raw));
    std::unique_ptr<dma_mmo_qp> q(q_raw);
    ASSERT_TRUE(q != nullptr);
    EXPECT_EQ(DPCP_OK, q->destroy());
}

/**
 * @test dpcp_dma_mmo_qp.ti_14_factory_rejects_rq_attr
 * @brief
 *    Any non-zero RQ-side field (rq_wqe_num / rq_wqe_sz / cqn_rcv) on this
 *    SQ-only QP -> DPCP_ERR_INVALID_PARAM, out QP unset. Each field is checked
 *    on its own so a regression in any one branch is caught.
 */
TEST_F(dpcp_dma_mmo_qp, ti_14_factory_rejects_rq_attr)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp* q_raw = nullptr;

    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);
    a.rq_wqe_num = 64;
    EXPECT_EQ(DPCP_ERR_INVALID_PARAM, s_ad->create_dma_mmo_qp(a, q_raw));
    EXPECT_EQ(nullptr, q_raw);

    a = make_attr_dma_mmo(s_cqd_snd.cqn);
    a.rq_wqe_sz = 64;
    EXPECT_EQ(DPCP_ERR_INVALID_PARAM, s_ad->create_dma_mmo_qp(a, q_raw));
    EXPECT_EQ(nullptr, q_raw);

    a = make_attr_dma_mmo(s_cqd_snd.cqn);
    a.cqn_rcv = s_cqd_rcv.cqn;
    EXPECT_EQ(DPCP_ERR_INVALID_PARAM, s_ad->create_dma_mmo_qp(a, q_raw));
    EXPECT_EQ(nullptr, q_raw);
}

/**
 * @brief Group lifecycle_* - state machine RST -> INIT -> RTR -> RTS.
 */

/**
 * @test dpcp_dma_mmo_qp.ti_15_lifecycle_destroy_shared
 * @brief
 *    destroy() on shared s_qp -> DPCP_OK; release the shared QP.
 * @details
 *    Last accessor case has run; release the shared QP so subsequent cases
 *    construct their own fresh instances for lifecycle/state-machine probes.
 */
TEST_F(dpcp_dma_mmo_qp, ti_15_lifecycle_destroy_shared)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_TRUE(s_qp != nullptr);
    EXPECT_EQ(DPCP_OK, s_qp->destroy());
    s_qp.reset();
}

/**
 * @test dpcp_dma_mmo_qp.ti_16_lifecycle_transition_to_rts_oneshot
 * @brief
 *    Reset a factory-created QP back to QP_RST, then transition_to_rts()
 *    drives RST -> RTS in one call. Exercises the public RST->RTS path
 *    explicitly on top of the factory's built-in transition.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_16_lifecycle_transition_to_rts_oneshot)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);
    dma_mmo_qp* q_raw = nullptr;
    ASSERT_EQ(DPCP_OK, s_ad->create_dma_mmo_qp(a, q_raw));
    std::unique_ptr<dma_mmo_qp> q(q_raw);
    ASSERT_TRUE(q != nullptr);
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_RST));
    ASSERT_EQ(DPCP_OK, q->transition_to_rts());
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, q->query_state(st));
    EXPECT_EQ(QP_RTS, st);
    EXPECT_EQ(DPCP_OK, q->destroy());
}

/**
 * @test dpcp_dma_mmo_qp.ti_17_lifecycle_step_by_step_modify
 * @brief
 *    Reset to RST, then step through INIT -> RTR -> RTS via modify_state();
 *    each step OK.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_17_lifecycle_step_by_step_modify)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);
    dma_mmo_qp* q_raw = nullptr;
    ASSERT_EQ(DPCP_OK, s_ad->create_dma_mmo_qp(a, q_raw));
    std::unique_ptr<dma_mmo_qp> q(q_raw);
    ASSERT_TRUE(q != nullptr);
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_RST));
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_INIT));
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_RTR));
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_RTS));
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, q->query_state(st));
    EXPECT_EQ(QP_RTS, st);
    EXPECT_EQ(DPCP_OK, q->destroy());
}

/**
 * @test dpcp_dma_mmo_qp.ti_18_lifecycle_to_reset_from_rts
 * @brief
 *    modify_state(QP_RST) from RTS succeeds; query reads QP_RST.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_18_lifecycle_to_reset_from_rts)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);
    dma_mmo_qp* q_raw = nullptr;
    ASSERT_EQ(DPCP_OK, s_ad->create_dma_mmo_qp(a, q_raw));
    std::unique_ptr<dma_mmo_qp> q(q_raw);
    ASSERT_TRUE(q != nullptr);
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_RST));
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, q->query_state(st));
    EXPECT_EQ(QP_RST, st);
    EXPECT_EQ(DPCP_OK, q->destroy());
}

/**
 * @test dpcp_dma_mmo_qp.ti_19_lifecycle_reset_from_init
 * @brief
 *    Reset to RST; drive QP to INIT; reset; query reads QP_RST.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_19_lifecycle_reset_from_init)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);
    dma_mmo_qp* q_raw = nullptr;
    ASSERT_EQ(DPCP_OK, s_ad->create_dma_mmo_qp(a, q_raw));
    std::unique_ptr<dma_mmo_qp> q(q_raw);
    ASSERT_TRUE(q != nullptr);
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_RST));
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_INIT));
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_RST));
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, q->query_state(st));
    EXPECT_EQ(QP_RST, st);
    EXPECT_EQ(DPCP_OK, q->destroy());
}

/**
 * @test dpcp_dma_mmo_qp.ti_20_lifecycle_reset_from_rtr
 * @brief
 *    Reset to RST; drive QP to RTR; reset; query reads QP_RST.
 * @details
 *    RTR is the first state where the DMA MMO on_build_init2rtr override
 *    runs (it sets fl=1 and remote_qpn=self). Reaching RTR validates that
 *    the override is accepted by FW.
 */
TEST_F(dpcp_dma_mmo_qp, ti_20_lifecycle_reset_from_rtr)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);
    dma_mmo_qp* q_raw = nullptr;
    ASSERT_EQ(DPCP_OK, s_ad->create_dma_mmo_qp(a, q_raw));
    std::unique_ptr<dma_mmo_qp> q(q_raw);
    ASSERT_TRUE(q != nullptr);
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_RST));
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_INIT));
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_RTR));
    ASSERT_EQ(DPCP_OK, q->modify_state(QP_RST));
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, q->query_state(st));
    EXPECT_EQ(QP_RST, st);
    EXPECT_EQ(DPCP_OK, q->destroy());
}

/**
 * @brief Group ext_* - caller-owned external WQ buffer / DB record paths.
 */

/**
 * @test dpcp_dma_mmo_qp.ti_21_ext_wq_buf
 * @brief
 *    External wq_buf via attr.wq_buf_addr; factory uses it; get_wq_buf
 *    returns the caller's pointer.
 * @details
 *    Sized via the static query_qp_buffer_sizes helper - the documented
 *    way to size externally allocated buffers before the factory call.
 */
TEST_F(dpcp_dma_mmo_qp, ti_21_ext_wq_buf)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);

    size_t wq_sz = 0, db_sz = 0;
    adapter::query_qp_buffer_sizes(a, wq_sz, db_sz);
    ASSERT_GT(wq_sz, 0U);

    void* ext = ::aligned_alloc(get_page_size(), wq_sz);
    ASSERT_NE(nullptr, ext);
    a.wq_buf_addr = ext;
    {
        dma_mmo_qp* q_raw = nullptr;
        ASSERT_EQ(DPCP_OK, s_ad->create_dma_mmo_qp(a, q_raw));
        std::unique_ptr<dma_mmo_qp> q(q_raw);
        ASSERT_TRUE(q != nullptr);
        void* got = nullptr;
        ASSERT_EQ(DPCP_OK, q->get_wq_buf(got));
        EXPECT_EQ(ext, got);
        EXPECT_EQ(DPCP_OK, q->destroy());
    }
    ::aligned_free(ext);
}

/**
 * @test dpcp_dma_mmo_qp.ti_22_ext_db_rec
 * @brief
 *    External db_rec via attr.db_addr; factory uses it; get_dbrec returns
 *    the caller's pointer.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_22_ext_db_rec)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);

    qp_db_rec* ext_db = static_cast<qp_db_rec*>(::aligned_alloc(get_cacheline_size(),
                                                                 qp::get_db_rec_sz()));
    ASSERT_NE(nullptr, ext_db);
    memset(ext_db, 0, qp::get_db_rec_sz());
    a.db_addr = ext_db;
    {
        dma_mmo_qp* q_raw = nullptr;
        ASSERT_EQ(DPCP_OK, s_ad->create_dma_mmo_qp(a, q_raw));
        std::unique_ptr<dma_mmo_qp> q(q_raw);
        ASSERT_TRUE(q != nullptr);
        qp_db_rec* got = nullptr;
        ASSERT_EQ(DPCP_OK, q->get_dbrec(got));
        EXPECT_EQ(ext_db, got);
        EXPECT_EQ(DPCP_OK, q->destroy());
    }
    ::aligned_free(ext_db);
}

/**
 * @test dpcp_dma_mmo_qp.ti_23_ext_both
 * @brief
 *    Both external wq_buf and db_rec supplied; both used; accessors return
 *    caller pointers.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_23_ext_both)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);

    size_t wq_sz = 0, db_sz = 0;
    adapter::query_qp_buffer_sizes(a, wq_sz, db_sz);
    void* ext_wq = ::aligned_alloc(get_page_size(), wq_sz);
    qp_db_rec* ext_db = static_cast<qp_db_rec*>(::aligned_alloc(get_cacheline_size(), db_sz));
    ASSERT_NE(nullptr, ext_wq);
    ASSERT_NE(nullptr, ext_db);
    memset(ext_db, 0, db_sz);
    a.wq_buf_addr = ext_wq;
    a.db_addr = ext_db;
    {
        dma_mmo_qp* q_raw = nullptr;
        ASSERT_EQ(DPCP_OK, s_ad->create_dma_mmo_qp(a, q_raw));
        std::unique_ptr<dma_mmo_qp> q(q_raw);
        ASSERT_TRUE(q != nullptr);
        void* got_wq = nullptr;
        qp_db_rec* got_db = nullptr;
        ASSERT_EQ(DPCP_OK, q->get_wq_buf(got_wq));
        ASSERT_EQ(DPCP_OK, q->get_dbrec(got_db));
        EXPECT_EQ(ext_wq, got_wq);
        EXPECT_EQ(ext_db, got_db);
        EXPECT_EQ(DPCP_OK, q->destroy());
    }
    ::aligned_free(ext_wq);
    ::aligned_free(ext_db);
}

/**
 * @brief Group negative_* - failure paths must out=nullptr.
 */

/**
 * @test dpcp_dma_mmo_qp.ti_24_negative_bad_cqn_snd
 * @brief
 *    Bogus cqn_snd causes FW CREATE_QP to fail; factory returns non-OK and
 *    sets out=nullptr (caller never observes a partially-constructed QP).
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_24_negative_bad_cqn_snd)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp_attr a = make_attr_dma_mmo(TEST_BOGUS_CQN);
    dma_mmo_qp* q = reinterpret_cast<dma_mmo_qp*>(0x1); /**< Sentinel - must be cleared. */
    status ret = s_ad->create_dma_mmo_qp(a, q);
    log_trace("ti_24 (bad cqn_snd) status=%d\n", ret);
    EXPECT_NE(DPCP_OK, ret);
    EXPECT_EQ(nullptr, q);
}

/**
 * @test dpcp_dma_mmo_qp.ti_25_negative_bad_mtu_fails_at_factory
 * @brief
 *    attr.mtu=qp_mtu(0). Factory drives INIT2RTR internally; the FW rejects
 *    the bad MTU and the factory returns non-OK with out=nullptr. Documents
 *    that the DMA MMO on_build_init2rtr override does not paper over invalid
 *    base attrs.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_25_negative_bad_mtu_fails_at_factory)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);
    a.mtu = static_cast<qp_mtu>(0);
    dma_mmo_qp* q = reinterpret_cast<dma_mmo_qp*>(0x1); /**< Sentinel - must be cleared. */
    status ret = s_ad->create_dma_mmo_qp(a, q);
    log_trace("ti_25 (bad_mtu) factory status=%d\n", ret);
    EXPECT_NE(DPCP_OK, ret);
    EXPECT_EQ(nullptr, q);
}

/**
 * @brief Group null_safe_* - post-destroy accessor behavior.
 */

/**
 * @test dpcp_dma_mmo_qp.ti_26_null_safe_post_destroy
 * @brief
 *    Fresh dma_mmo_qp; create + destroy; accessors handle the released
 *    state without crashing. Ptr-returning accessors return NO_MEMORY /
 *    NO_SUPPORT; get_qpn returns INVALID_ID (qp::destroy() zeroes m_qpn /
 *    m_state); cached-attr accessors (e.g. cqn) still return OK with stale
 *    values.
 * @details
 */
TEST_F(dpcp_dma_mmo_qp, ti_26_null_safe_post_destroy)
{
    SKIP_IF_DMA_MMO_UNSUPPORTED();
    ASSERT_NE(nullptr, s_ad);
    dma_mmo_qp_attr a = make_attr_dma_mmo(s_cqd_snd.cqn);
    dma_mmo_qp* q_raw = nullptr;
    ASSERT_EQ(DPCP_OK, s_ad->create_dma_mmo_qp(a, q_raw));
    std::unique_ptr<dma_mmo_qp> q(q_raw);
    ASSERT_TRUE(q != nullptr);

    uint32_t qpn_pre = 0;
    ASSERT_EQ(DPCP_OK, q->get_qpn(qpn_pre));
    EXPECT_NE(0U, qpn_pre);

    EXPECT_EQ(DPCP_OK, q->destroy());

    void* wb = nullptr;
    EXPECT_EQ(DPCP_ERR_NO_MEMORY, q->get_wq_buf(wb));
    qp_db_rec* db = nullptr;
    EXPECT_EQ(DPCP_ERR_NO_MEMORY, q->get_dbrec(db));
    volatile void* up = nullptr;
    EXPECT_EQ(DPCP_ERR_NO_MEMORY, q->get_uar_page(up));
    uint64_t* bf = nullptr;
    EXPECT_EQ(DPCP_ERR_NO_SUPPORT, q->get_bf_reg(bf));

    uint32_t qpn_post = 0;
    EXPECT_EQ(DPCP_ERR_INVALID_ID, q->get_qpn(qpn_post));
    EXPECT_EQ(0U, qpn_post);

    uint32_t v = 0;
    EXPECT_EQ(DPCP_OK, q->get_cqn_snd(v));
    EXPECT_EQ(s_cqd_snd.cqn, v);

}

/**
 * @brief Group teardown_ - final suite-level cleanup.
 */

/**
 * @test dpcp_dma_mmo_qp.ti_27_destroy_idempotent
 * @brief
 *    Suite teardown: delete the adapter (releases CQs implicitly via owner
 *    tracking).
 * @details
 *    Final test in the fixture - also clears s_ad.
 */
TEST_F(dpcp_dma_mmo_qp, ti_27_destroy_idempotent)
{
    if (!s_dma_mmo_supported) {
        if (s_ad) {
            delete s_ad;
            s_ad = nullptr;
        }
        return;
    }
    ASSERT_NE(nullptr, s_ad);
    delete s_ad;
    s_ad = nullptr;
}
