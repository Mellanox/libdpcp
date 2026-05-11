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
 *  Used by make_attr_sq_only / make_attr_sq_rq and the test bodies that
 *  assert what those helpers populated.
 * ---------------------------------------------------------------------------
 */
static constexpr uint8_t  TEST_PORT_NUM = 1; /**< First HCA port. */
static constexpr uint16_t TEST_PKEY_INDEX = 0;
static constexpr uint32_t TEST_USER_INDEX = 0;
static constexpr uint32_t TEST_SQ_WQE_NUM = 1024; /**< Must be power of 2. */
static constexpr uint32_t TEST_SQ_WQE_SZ = 64; /**< One WQEBB. */
static constexpr uint32_t TEST_RQ_WQE_NUM = 64; /**< Must be power of 2. */
static constexpr uint32_t TEST_RQ_WQE_SZ = 64; /**< Yields log_rq_stride = 2. */
static constexpr uint32_t TEST_BOGUS_CQN = 0xDEADBEEF;  /**< Unallocated CQN sentinel for negative-path tests. */

/**
 * @brief class TestQp - Minimal concrete qp subclass for unit tests.
 *
 * qp is abstract (virtual ~qp() = 0), so the tests need a concrete subclass.
 * The using-declarations elevate the listed methods and UMEM-id members from
 * protected (on qp) to public (on TestQp) so tests can drive qp::create()
 * directly without the adapter factory that normally registers the UMEMs.
 */
class TestQp : public qp {
public:
    /**
     * @brief TestQp constructor.
     *
     * @param [in] ad  Adapter to associate with this QP. Must be non-null and open.
     * @param [in] a   QP attributes. The caller is responsible for populating all
     *                 fields meaningfully, as TestQp doesn't override on_build_create()
     *                 to set any defaults.
     */
    TestQp(adapter* ad, const qp_attr& a) : qp(ad, a) {}
    virtual ~TestQp() = default;

    using qp::create;
    using qp::init;
    using qp::allocate_wq_buf;
    using qp::set_wq_buf;
    using qp::allocate_db_rec;
    using qp::set_db_rec;

    using qp::m_adapter;
    using qp::m_attr;
    using qp::m_state;
    using qp::m_qpn;
    using qp::m_uar;
    using qp::m_owned_wq_buf;
    using qp::m_wq_buf;
    using qp::m_wq_buf_umem;
    using qp::m_wq_buf_umem_id;
    using qp::m_owned_db_rec;
    using qp::m_db_rec;
    using qp::m_db_rec_umem;
    using qp::m_db_rec_umem_id;
};

/**
 * @brief class TestQpTraced - TestQp variant that counts on_build_* hook invocations.
 *
 * Used by the hooks_* test group to validate that qp::create() and the
 * RST->INIT->RTR->RTS modify chain each invoke their corresponding hook
 * exactly once. Each override increments its counter and chains to the
 * base qp:: implementation so QP creation/state transitions still succeed
 * normally.
 */
class TestQpTraced : public TestQp {
public:
    int hook_create = 0; /**< Times on_build_create() was invoked. */
    int hook_rst2init = 0; /**< Times on_build_rst2init() was invoked. */
    int hook_init2rtr = 0; /**< Times on_build_init2rtr() was invoked. */
    int hook_rtr2rts = 0; /**< Times on_build_rtr2rts() was invoked. */

    /**
     * @brief TestQpTraced constructor.
     *
     * @param [in] ad  Adapter to associate with this QP. Must be non-null and open.
     * @param [in] a   QP attributes. The caller is responsible for populating all
     *                 fields meaningfully, as TestQpTraced doesn't override on_build_create()
     *                 to set any defaults.
     */
    TestQpTraced(adapter* ad, const qp_attr& a) : TestQp(ad, a) {}
    virtual ~TestQpTraced() = default;

    status on_build_create(void* p_in, void* p_qpc, void* p_qpc_ext) override
    {
        ++hook_create;
        return qp::on_build_create(p_in, p_qpc, p_qpc_ext);
    }

    status on_build_rst2init(void* p_qpc) override
    {
        ++hook_rst2init;
        return qp::on_build_rst2init(p_qpc);
    }

    status on_build_init2rtr(void* p_qpc) override
    {
        ++hook_init2rtr;
        return qp::on_build_init2rtr(p_qpc);
    }

    status on_build_rtr2rts(void* p_qpc) override
    {
        ++hook_rtr2rts;
        return qp::on_build_rtr2rts(p_qpc);
    }
};

/*
 * ---------------------------------------------------------------------------
 *  Shared fixture state.
 *
 *  DPCP tests follow the convention of sharing file-static globals between
 *  sequential TEST_F cases (e.g, sq_tests.cpp / rq_tests.cpp). Earlier cases
 *  build state; later cases consume it; the last case tears it all down.
 *
 *  Lifecycle:
 *    - s_ad / s_cqd_snd / s_cqd_rcv : opened in ti_03_sq_only_create;
 *                                     destroyed in ti_59_illegal_rq_without_cqn_rcv.
 *    - s_uar_pool / s_uar           : lazy-initialized by ensure_uar_pool()
 *                                     on first setup_qp_chain() call;
 *                                     destroyed in ti_59.
 *    - s_test_qp                    : built in ti_03; destroyed in ti_21.
 *                                     Used by every sq_only_* case in between.
 *    - s_test_qp_rq                 : built in ti_23; destroyed in ti_38.
 *                                     Used by every sq_rq_* case in between.
 *    - s_test_qp_qpn                : captured in ti_05 for ti_25's
 *                                     distinct-qpn assertion.
 * ---------------------------------------------------------------------------
 */
static adapter* s_ad = nullptr; /**< Adapter handle (HW-bound). */
static cq_data s_cqd_snd = {}; /**< Send CQ for SQ-bearing QPs. */
static cq_data s_cqd_rcv = {}; /**< Receive CQ for RQ-bearing QPs. */
static std::unique_ptr<uar_collection> s_uar_pool; /**< Local UAR pool (separate from adapter's private m_uarpool). */
static uar_t s_uar = {}; /**< Shared UAR page for all setup_qp_chain() calls. */
static int s_uar_key = 0; /**< Arbitrary non-null lookup key for uar_collection::get_uar(). */

static std::unique_ptr<TestQp> s_test_qp; /**< Shared SQ-only QP (ti_03..ti_21). */
static std::unique_ptr<TestQp> s_test_qp_rq; /**< Shared SQ+RQ QP (ti_23..ti_38). */
static uint32_t s_test_qp_qpn = 0; /**< QPN of s_test_qp, captured by ti_05 so ti_25 can
                                        assert that s_test_qp_rq's QPN is distinct. */

/**
 * @brief class dpcp_qp - Test fixture for the qp_tests target.
 */
class dpcp_qp : public dpcp_base {
protected:
    void SetUp() override
    {
        if (errno) {
            log_trace("dpcp_qp::SetUp errno=%d\n", errno);
            errno = EOK;
        }
    }
};

/**
 * @brief Build a baseline SQ-only qp_attr for the universal RC-QP path.
 *
 * @param [in] cqn_snd Send-side CQ number (must be a valid CQN).
 * @param [in] cqn_rcv Receive-side CQ number; cached only - never written
 *                     to HW for SQ-only QPs (qp::create() guards on rq_wqe_num).
 *
 * @retval The populated qp_attr.
 */
static qp_attr make_attr_sq_only(uint32_t cqn_snd, uint32_t cqn_rcv)
{
    qp_attr attr = {};
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
    attr.cqn_rcv = cqn_rcv;
    attr.rq_wqe_num = 0;
    attr.rq_wqe_sz = 0;
    return attr;
}

/**
 * @brief Build a baseline SQ+RQ qp_attr derived from make_attr_sq_only.
 *
 * Adds a minimal RQ (64 WQEs × 64 bytes). The RQ sizes are chosen so that
 * qp::create()'s log_rq_stride = ilog2(rq_wqe_sz) - LOG_RQ_STRIDE_SHIFT
 * produces a valid non-negative stride: log2(64) - 4 = 2.
 *
 * @param [in] cqn_snd Send-side CQ number.
 * @param [in] cqn_rcv Receive-side CQ number (now also written to HW since rq_wqe_num > 0).
 *
 * @retval The populated qp_attr.
 */
static qp_attr make_attr_sq_rq(uint32_t cqn_snd, uint32_t cqn_rcv)
{
    qp_attr attr = make_attr_sq_only(cqn_snd, cqn_rcv);
    attr.rq_wqe_num = TEST_RQ_WQE_NUM;
    attr.rq_wqe_sz = TEST_RQ_WQE_SZ;
    return attr;
}

/**
 * @brief Register a caller-owned buffer as a DEVX UMEM, returning the umem
 *        handle and its FW-assigned id.
 *
 * @param [in]  ctx     DEVX context (from adapter::get_ctx()).
 * @param [in]  buf     Buffer to register; must be non-null.
 * @param [in]  sz      Size in bytes; must be non-zero.
 * @param [out] umem    Receives the newly-created umem; ownership is held
 *                      by the unique_ptr and released before the buffer.
 * @param [out] mem_id  The FW-assigned UMEM id, suitable for writing into
 *                      qpc.wq_umem_id or qpc.dbr_umem_id.
 *
 * @retval DPCP_OK on success; DPCP_ERR_INVALID_PARAM on null/zero input;
 *         DPCP_ERR_UMEM on ctx->create_umem() failure.
 */
static status test_reg_mem(dcmd::ctx* ctx, void* buf, size_t sz,
                           std::unique_ptr<dcmd::umem>& umem, uint32_t& mem_id)
{
    if (!ctx || !buf || sz == 0) {
        log_error("test_reg_mem invalid param: ctx=%p buf=%p sz=%zu\n", ctx, buf, sz);
        return DPCP_ERR_INVALID_PARAM;
    }
    dcmd::umem_desc dscr = {buf, sz, 1};
    umem.reset(ctx->create_umem(&dscr));
    if (!umem) {
        log_error("test_reg_mem create_umem failed buf=%p sz=%zu\n", buf, sz);
        return DPCP_ERR_UMEM;
    }
    mem_id = umem->get_id();
    return DPCP_OK;
}

/**
 * @brief Lazy-initialize the file-static UAR pool and obtain a shared UAR page.
 *
 * Constructs s_uar_pool on first call (the adapter's own m_uarpool is private
 * and not reachable from the test surface) and populates s_uar with a single
 * shared UAR page that every TestQp instance binds to via setup_qp_chain().
 * Subsequent calls are no-ops.
 *
 * @retval DPCP_OK on success or if already initialized;
 *         DPCP_ERR_NO_MEMORY on uar_collection allocation failure;
 *         DPCP_ERR_ALLOC_UAR if uar_collection::get_uar() returns null;
 *         the status of uar_collection::get_uar_page() otherwise.
 */
static status ensure_uar_pool()
{
    if (s_uar_pool) {
        return DPCP_OK;
    }
    s_uar_pool.reset(new (std::nothrow) uar_collection(s_ad->get_ctx()));
    if (!s_uar_pool) {
        log_error("ensure_uar_pool: failed to allocate uar_collection\n");
        return DPCP_ERR_NO_MEMORY;
    }
    uar u = s_uar_pool->get_uar(&s_uar_key);
    if (!u) {
        log_error("ensure_uar_pool: get_uar returned null\n");
        return DPCP_ERR_ALLOC_UAR;
    }
    return s_uar_pool->get_uar_page(u, s_uar);
}

/**
 * @brief Drive a TestQp through the full create chain.
 *
 * @param [in,out] q              The TestQp to set up. Must be freshly
 *                                constructed (pre-create state).
 * @param [in]     ext_wq_buf     Optional caller-owned WQ buffer. If non-null,
 *                                set_wq_buf() is called instead of allocating
 *                                internally; the QP does not take ownership
 *                                and the dtor leaves the buffer untouched.
 * @param [in]     ext_db_rec     Optional caller-owned DB record. Same
 *                                semantics as ext_wq_buf.
 * @param [in]     ext_wq_buf_sz  Optional UMEM-registration size override for
 *                                the external WQ buffer. Defaults to
 *                                q.get_wq_buf_sz() (the natural SQ+RQ sum).
 *                                Used by ti_47_ext_wq_buf_undersized to
 *                                probe FW validation with a deliberately
 *                                smaller UMEM than the QP needs.
 *
 * @retval DPCP_OK on full-chain success; the first non-OK status from any
 *         step otherwise (allocation, UMEM registration, or init/create).
 */
static status setup_qp_chain(TestQp& q, void* ext_wq_buf = nullptr,
                             qp_db_rec* ext_db_rec = nullptr,
                             size_t ext_wq_buf_sz = 0)
{
    status ret = ensure_uar_pool();
    if (ret != DPCP_OK) {
        log_error("setup_qp_chain: ensure_uar_pool failed with status %d\n", ret);
        return ret;
    }

    /* WQ buffer */
    void* wq_buf = nullptr;
    size_t wq_sz = q.get_wq_buf_sz();
    if (ext_wq_buf) {
        wq_buf = ext_wq_buf;
        size_t reg_sz = ext_wq_buf_sz ? ext_wq_buf_sz : wq_sz;
        q.set_wq_buf(wq_buf);
        ret = test_reg_mem(s_ad->get_ctx(), wq_buf, reg_sz,
                           q.m_wq_buf_umem, q.m_wq_buf_umem_id);
        if (ret != DPCP_OK) {
            log_error("setup_qp_chain: test_reg_mem for ext_wq_buf failed with status %d\n", ret);
            return ret;
        }
    } else if (wq_sz > 0) {
        ret = q.allocate_wq_buf(wq_buf, wq_sz);
        if (ret != DPCP_OK) {
            log_error("setup_qp_chain: allocate_wq_buf failed with status %d\n", ret);
            return ret;
        }
        ret = test_reg_mem(s_ad->get_ctx(), wq_buf, wq_sz, q.m_wq_buf_umem, q.m_wq_buf_umem_id);
    }
    if (ret != DPCP_OK) {
        log_error("setup_qp_chain: WQ buffer setup failed with status %d\n", ret);
        return ret;
    }

    /* DB record */
    qp_db_rec* db_rec = nullptr;
    size_t db_sz = qp::get_db_rec_sz();
    if (ext_db_rec) {
        db_rec = ext_db_rec;
        q.set_db_rec(db_rec);
    } else {
        ret = q.allocate_db_rec(db_rec, db_sz);
        if (ret != DPCP_OK) {
            log_error("setup_qp_chain: allocate_db_rec failed with status %d\n", ret);
            return ret;
        }
    }
    ret = test_reg_mem(s_ad->get_ctx(), db_rec, db_sz, q.m_db_rec_umem, q.m_db_rec_umem_id);
    if (ret != DPCP_OK) {
        log_error("setup_qp_chain: test_reg_mem for DB record failed with status %d\n", ret);
        return ret;
    }

    return q.init(&s_uar);
}

/**
 * @brief Group static_* - pure helpers, no QP needed.
 */

/**
 * @test dpcp_qp.ti_01_static_get_wq_buf_sz
 * @brief
 *    Static qp::get_wq_buf_sz returns rq_sz*rq_num + sq_sz*sq_num, with the RQ
 *    region rounded up to a 64B (WQEBB) boundary when an SQ follows it.
 * @details
 *    Probes the static helper independently of any QP instance.
 *    SQ-only: pass 0,0 for RQ params. Combined: both RQ and SQ contribute.
 *    Per PRM Work Queues Structure and Access section a trailing SQ must start
 *    WQEBB-aligned, so a sub-64B RQ region is padded up to 64B before the SQ
 *    is appended (a no-op for the common >=64B RQ). RQ-only is not padded.
 */
TEST_F(dpcp_qp, ti_01_static_get_wq_buf_sz)
{
    EXPECT_EQ(64U * 1024U, qp::get_wq_buf_sz(0, 0, 64, 1024));
    EXPECT_EQ(128U * 512U, qp::get_wq_buf_sz(0, 0, 128, 512));
    EXPECT_EQ(64U * 128U + 128U * 64U, qp::get_wq_buf_sz(64, 128, 128, 64));
    // RQ region already 64B-aligned: no padding, plain sum.
    EXPECT_EQ(64U * 1U + 64U * 4U, qp::get_wq_buf_sz(64, 1, 64, 4));
    // Sub-64B RQ region with an SQ following: RQ padded up to 64B.
    EXPECT_EQ(64U + 64U * 4U, qp::get_wq_buf_sz(16, 1, 64, 4)); // 16B RQ -> 64B
    EXPECT_EQ(64U + 64U * 4U, qp::get_wq_buf_sz(16, 2, 64, 4)); // 32B RQ -> 64B
    // RQ-only (no SQ): RQ region is not padded.
    EXPECT_EQ(16U * 1U, qp::get_wq_buf_sz(16, 1, 0, 0));
}

/**
 * @test dpcp_qp.ti_02_static_get_db_rec_sz
 * @brief
 *    Static qp::get_db_rec_sz() returns DB_REC_SIZE (64).
 * @details
 */
TEST_F(dpcp_qp, ti_02_static_get_db_rec_sz)
{
    EXPECT_EQ(static_cast<size_t>(DB_REC_SIZE), qp::get_db_rec_sz());
}

/**
 * @brief Group sq_only_* - SQ-only lifecycle on shared s_test_qp.
 */

/**
 * @test dpcp_qp.ti_03_sq_only_create
 * @brief
 *    Open adapter, create CQs, build SQ-only TestQp via setup_qp_chain.
 * @details
 *    Establishes s_test_qp for subsequent sq_only_* cases.
 */
TEST_F(dpcp_qp, ti_03_sq_only_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);
    s_ad = ad;

    ASSERT_EQ(DPCP_OK, (status)create_cq(s_ad, &s_cqd_snd));
    ASSERT_NE(0U, s_cqd_snd.cqn);
    ASSERT_EQ(DPCP_OK, (status)create_cq(s_ad, &s_cqd_rcv));
    ASSERT_NE(0U, s_cqd_rcv.cqn);

    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    s_test_qp.reset(new (std::nothrow) TestQp(s_ad, a));
    ASSERT_TRUE(s_test_qp != nullptr);

    ret = setup_qp_chain(*s_test_qp);
    ASSERT_EQ(DPCP_OK, ret);

    EXPECT_EQ(QP_RST, s_test_qp->m_state);
}

/**
 * @test dpcp_qp.ti_04_sq_only_get_state_rst
 * @brief
 *    get_state returns QP_RST after create.
 * @details
 *    qp::get_state returns cached m_state (no HW query).
 */
TEST_F(dpcp_qp, ti_04_sq_only_get_state_rst)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, s_test_qp->get_state(st));
    EXPECT_EQ(QP_RST, st);
}

/**
 * @test dpcp_qp.ti_05_sq_only_get_qpn
 * @brief
 *    get_qpn returns DPCP_OK with non-zero QPN.
 * @details
 */
TEST_F(dpcp_qp, ti_05_sq_only_get_qpn)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    uint32_t qpn = 0;
    ASSERT_EQ(DPCP_OK, s_test_qp->get_qpn(qpn));
    EXPECT_NE(0U, qpn);
    s_test_qp_qpn = qpn; // captured for ti_25 distinct-qpn check
}

/**
 * @test dpcp_qp.ti_06_sq_only_get_sq_wqe
 * @brief
 *    get_sq_wqe_sz / get_sq_wqe_num match attr.
 * @details
 */
TEST_F(dpcp_qp, ti_06_sq_only_get_sq_wqe)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    uint32_t sz = 0, num = 0;
    ASSERT_EQ(DPCP_OK, s_test_qp->get_sq_wqe_sz(sz));
    ASSERT_EQ(DPCP_OK, s_test_qp->get_sq_wqe_num(num));
    EXPECT_EQ(TEST_SQ_WQE_SZ, sz);
    EXPECT_EQ(TEST_SQ_WQE_NUM, num);
}

/**
 * @test dpcp_qp.ti_07_sq_only_get_rq_wqe_zero
 * @brief
 *    For SQ-only, get_rq_wqe_sz / get_rq_wqe_num return 0.
 * @details
 */
TEST_F(dpcp_qp, ti_07_sq_only_get_rq_wqe_zero)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    uint32_t sz = 1, num = 1;
    ASSERT_EQ(DPCP_OK, s_test_qp->get_rq_wqe_sz(sz));
    ASSERT_EQ(DPCP_OK, s_test_qp->get_rq_wqe_num(num));
    EXPECT_EQ(0U, sz);
    EXPECT_EQ(0U, num);
}

/**
 * @test dpcp_qp.ti_08_sq_only_get_cqn_snd
 * @brief
 *    get_cqn_snd matches attr.cqn_snd.
 * @details
 */
TEST_F(dpcp_qp, ti_08_sq_only_get_cqn_snd)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    uint32_t cqn = 0;
    ASSERT_EQ(DPCP_OK, s_test_qp->get_cqn_snd(cqn));
    EXPECT_EQ(s_cqd_snd.cqn, cqn);
}

/**
 * @test dpcp_qp.ti_09_sq_only_get_cqn_rcv_cached
 * @brief
 *    For SQ-only QP, get_cqn_rcv returns the cached attr.cqn_rcv.
 * @details
 *    qp::create() does NOT write cqn_rcv to HW when m_rq_wqe_num==0
 *    (guarded inside qp::create()). Accessor returns the cached attr
 *    value regardless.
 */
TEST_F(dpcp_qp, ti_09_sq_only_get_cqn_rcv_cached)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    uint32_t cqn = 0;
    ASSERT_EQ(DPCP_OK, s_test_qp->get_cqn_rcv(cqn));
    EXPECT_EQ(s_cqd_rcv.cqn, cqn);
}

/**
 * @test dpcp_qp.ti_10_sq_only_get_wq_buf
 * @brief
 *    get_wq_buf non-null and page-aligned (qp::allocate_wq_buf uses
 *    aligned_alloc(get_page_size(), ...)).
 * @details
 */
TEST_F(dpcp_qp, ti_10_sq_only_get_wq_buf)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    void* buf = nullptr;
    ASSERT_EQ(DPCP_OK, s_test_qp->get_wq_buf(buf));
    ASSERT_NE(nullptr, buf);
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(buf) % get_page_size());
}

/**
 * @test dpcp_qp.ti_11_sq_only_get_dbrec
 * @brief
 *    get_dbrec non-null, cacheline-aligned, and zero-initialized.
 * @details
 *    qp::allocate_db_rec uses aligned_alloc(get_cacheline_size(), ...) and
 *    memsets the buffer to 0.
 */
TEST_F(dpcp_qp, ti_11_sq_only_get_dbrec)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    qp_db_rec* dbrec = nullptr;
    ASSERT_EQ(DPCP_OK, s_test_qp->get_dbrec(dbrec));
    ASSERT_NE(nullptr, dbrec);
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(dbrec) % get_cacheline_size());
    uint8_t zero_db[DB_REC_SIZE] = {};
    EXPECT_EQ(0, memcmp(dbrec, zero_db, qp::get_db_rec_sz()));
}

/**
 * @test dpcp_qp.ti_12_sq_only_get_uar_page
 * @brief
 *    get_uar_page non-null.
 * @details
 */
TEST_F(dpcp_qp, ti_12_sq_only_get_uar_page)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    volatile void* page = nullptr;
    ASSERT_EQ(DPCP_OK, s_test_qp->get_uar_page(page));
    EXPECT_NE(nullptr, const_cast<void*>(page));
}

/**
 * @test dpcp_qp.ti_13_sq_only_get_bf_reg
 * @brief
 *    get_bf_reg returns DPCP_OK with non-null bf_reg.
 * @details
 */
TEST_F(dpcp_qp, ti_13_sq_only_get_bf_reg)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    uint64_t* bf = nullptr;
    ASSERT_EQ(DPCP_OK, s_test_qp->get_bf_reg(bf));
    EXPECT_NE(nullptr, bf);
}

/**
 * @test dpcp_qp.ti_14_sq_only_get_wq_buf_sz_instance
 * @brief
 *    Instance get_wq_buf_sz() returns sq_wqe_sz*sq_wqe_num (RQ=0).
 * @details
 *    Inline overload sums get_wq_buf_sz(rq_*) + get_wq_buf_sz(sq_*); for
 *    SQ-only QPs the RQ contribution is zero.
 */
TEST_F(dpcp_qp, ti_14_sq_only_get_wq_buf_sz_instance)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    EXPECT_EQ(static_cast<size_t>(TEST_SQ_WQE_SZ) * TEST_SQ_WQE_NUM,
              s_test_qp->get_wq_buf_sz());
}

/**
 * @test dpcp_qp.ti_15_sq_only_modify_state_init
 * @brief
 *    modify_state(QP_INIT) -> DPCP_OK.
 * @details
 */
TEST_F(dpcp_qp, ti_15_sq_only_modify_state_init)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    ASSERT_EQ(DPCP_OK, s_test_qp->modify_state(QP_INIT));
}

/**
 * @test dpcp_qp.ti_16_sq_only_query_state_init
 * @brief
 *    query_state reads QP_INIT from HW.
 * @details
 */
TEST_F(dpcp_qp, ti_16_sq_only_query_state_init)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, s_test_qp->query_state(st));
    EXPECT_EQ(QP_INIT, st);
}

/**
 * @test dpcp_qp.ti_17_sq_only_modify_state_rtr
 * @brief
 *    modify_state(QP_RTR) -> DPCP_OK.
 * @details
 */
TEST_F(dpcp_qp, ti_17_sq_only_modify_state_rtr)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    ASSERT_EQ(DPCP_OK, s_test_qp->modify_state(QP_RTR));
}

/**
 * @test dpcp_qp.ti_18_sq_only_modify_state_rts
 * @brief
 *    modify_state(QP_RTS) -> DPCP_OK.
 * @details
 */
TEST_F(dpcp_qp, ti_18_sq_only_modify_state_rts)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    ASSERT_EQ(DPCP_OK, s_test_qp->modify_state(QP_RTS));
}

/**
 * @test dpcp_qp.ti_19_sq_only_query_state_rts
 * @brief
 *    query_state reads QP_RTS from HW.
 * @details
 */
TEST_F(dpcp_qp, ti_19_sq_only_query_state_rts)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, s_test_qp->query_state(st));
    EXPECT_EQ(QP_RTS, st);
}

/**
 * @test dpcp_qp.ti_20_sq_only_to_reset_from_rts
 * @brief
 *    to_reset() (via modify_state(QP_RST)) -> DPCP_OK; query reads QP_RST.
 * @details
 *    Per the QP state model, QP_2RST is valid from any state.
 *    qp::to_reset() is best-effort: the API always returns DPCP_OK and
 *    sets cached m_state = QP_RST regardless of the FW response.
 */
TEST_F(dpcp_qp, ti_20_sq_only_to_reset_from_rts)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    ASSERT_EQ(DPCP_OK, s_test_qp->modify_state(QP_RST));
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, s_test_qp->query_state(st));
    EXPECT_EQ(QP_RST, st);
}

/**
 * @test dpcp_qp.ti_21_sq_only_destroy
 * @brief
 *    destroy() -> DPCP_OK; release the shared QP.
 * @details
 */
TEST_F(dpcp_qp, ti_21_sq_only_destroy)
{
    ASSERT_TRUE(s_test_qp != nullptr);
    EXPECT_EQ(DPCP_OK, s_test_qp->destroy());
    s_test_qp.reset();
}

/**
 * @test dpcp_qp.ti_22_sq_only_transition_to_rts_oneshot
 * @brief
 *    Fresh local SQ-only TestQp; transition_to_rts() RST->RTS in one call.
 * @details
 */
TEST_F(dpcp_qp, ti_22_sq_only_transition_to_rts_oneshot)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    TestQp q(s_ad, a);
    ASSERT_EQ(DPCP_OK, setup_qp_chain(q));
    ASSERT_EQ(DPCP_OK, q.transition_to_rts());
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, q.query_state(st));
    EXPECT_EQ(QP_RTS, st);
}

/**
 * @brief Group sq_rq_* - SQ+RQ lifecycle on shared s_test_qp_rq.
 */

/**
 * @test dpcp_qp.ti_23_sq_rq_create
 * @brief
 *    Construct SQ+RQ TestQp; full build chain -> DPCP_OK; cached state RST.
 * @details
 */
TEST_F(dpcp_qp, ti_23_sq_rq_create)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_rq(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    s_test_qp_rq.reset(new (std::nothrow) TestQp(s_ad, a));
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    ASSERT_EQ(DPCP_OK, setup_qp_chain(*s_test_qp_rq));
    EXPECT_EQ(QP_RST, s_test_qp_rq->m_state);
}

/**
 * @test dpcp_qp.ti_24_sq_rq_get_state_rst
 * @brief
 *    get_state returns QP_RST.
 * @details
 */
TEST_F(dpcp_qp, ti_24_sq_rq_get_state_rst)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->get_state(st));
    EXPECT_EQ(QP_RST, st);
}

/**
 * @test dpcp_qp.ti_25_sq_rq_get_qpn_distinct
 * @brief
 *    Non-zero QPN, distinct from s_test_qp's QPN captured in ti_05.
 * @details
 *    Validates independent QPN allocation per QP.
 */
TEST_F(dpcp_qp, ti_25_sq_rq_get_qpn_distinct)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    uint32_t qpn = 0;
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->get_qpn(qpn));
    EXPECT_NE(0U, qpn);
    EXPECT_NE(s_test_qp_qpn, qpn);
}

/**
 * @test dpcp_qp.ti_26_sq_rq_get_sq_wqe
 * @brief
 *    get_sq_wqe_sz / get_sq_wqe_num match attr.
 * @details
 */
TEST_F(dpcp_qp, ti_26_sq_rq_get_sq_wqe)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    uint32_t sz = 0, num = 0;
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->get_sq_wqe_sz(sz));
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->get_sq_wqe_num(num));
    EXPECT_EQ(TEST_SQ_WQE_SZ, sz);
    EXPECT_EQ(TEST_SQ_WQE_NUM, num);
}

/**
 * @test dpcp_qp.ti_27_sq_rq_get_rq_wqe
 * @brief
 *    get_rq_wqe_sz / get_rq_wqe_num match attr (non-zero, contra ti_07).
 * @details
 */
TEST_F(dpcp_qp, ti_27_sq_rq_get_rq_wqe)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    uint32_t sz = 0, num = 0;
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->get_rq_wqe_sz(sz));
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->get_rq_wqe_num(num));
    EXPECT_EQ(TEST_RQ_WQE_SZ, sz);
    EXPECT_EQ(TEST_RQ_WQE_NUM, num);
}

/**
 * @test dpcp_qp.ti_28_sq_rq_get_cqn_rcv_written
 * @brief
 *    get_cqn_rcv matches attr.cqn_rcv. This instance hits the
 *    if (m_rq_wqe_num > 0) branch in qp::create() that writes cqn_rcv to HW
 *    (the branch ti_09 skips). The accessor still returns cached attr.
 * @details
 */
TEST_F(dpcp_qp, ti_28_sq_rq_get_cqn_rcv_written)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    uint32_t cqn = 0;
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->get_cqn_rcv(cqn));
    EXPECT_EQ(s_cqd_rcv.cqn, cqn);
}

/**
 * @test dpcp_qp.ti_29_sq_rq_get_wq_buf
 * @brief
 *    get_wq_buf non-null and page-aligned.
 * @details
 */
TEST_F(dpcp_qp, ti_29_sq_rq_get_wq_buf)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    void* buf = nullptr;
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->get_wq_buf(buf));
    ASSERT_NE(nullptr, buf);
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(buf) % get_page_size());
}

/**
 * @test dpcp_qp.ti_30_sq_rq_get_wq_buf_sz_instance
 * @brief
 *    Instance get_wq_buf_sz() returns (sq_wqe_sz*sq_wqe_num)+(rq_wqe_sz*rq_wqe_num).
 * @details
 */
TEST_F(dpcp_qp, ti_30_sq_rq_get_wq_buf_sz_instance)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    EXPECT_EQ(static_cast<size_t>(TEST_SQ_WQE_SZ) * TEST_SQ_WQE_NUM +
              static_cast<size_t>(TEST_RQ_WQE_SZ) * TEST_RQ_WQE_NUM,
              s_test_qp_rq->get_wq_buf_sz());
}

/**
 * @test dpcp_qp.ti_31_sq_rq_get_dbrec
 * @brief
 *    get_dbrec non-null and zero-initialized.
 * @details
 */
TEST_F(dpcp_qp, ti_31_sq_rq_get_dbrec)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    qp_db_rec* dbrec = nullptr;
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->get_dbrec(dbrec));
    ASSERT_NE(nullptr, dbrec);
    uint8_t zero_db[DB_REC_SIZE] = {};
    EXPECT_EQ(0, memcmp(dbrec, zero_db, qp::get_db_rec_sz()));
}

/**
 * @test dpcp_qp.ti_32_sq_rq_modify_state_init
 * @brief
 *    modify_state(QP_INIT) -> DPCP_OK.
 * @details
 */
TEST_F(dpcp_qp, ti_32_sq_rq_modify_state_init)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->modify_state(QP_INIT));
}

/**
 * @test dpcp_qp.ti_33_sq_rq_modify_state_rtr
 * @brief
 *    modify_state(QP_RTR) -> DPCP_OK.
 * @details
 */
TEST_F(dpcp_qp, ti_33_sq_rq_modify_state_rtr)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->modify_state(QP_RTR));
}

/**
 * @test dpcp_qp.ti_34_sq_rq_modify_state_rts
 * @brief
 *    modify_state(QP_RTS) -> DPCP_OK.
 * @details
 */
TEST_F(dpcp_qp, ti_34_sq_rq_modify_state_rts)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->modify_state(QP_RTS));
}

/**
 * @test dpcp_qp.ti_35_sq_rq_query_state_rts
 * @brief
 *    query_state reads QP_RTS from HW.
 * @details
 */
TEST_F(dpcp_qp, ti_35_sq_rq_query_state_rts)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->query_state(st));
    EXPECT_EQ(QP_RTS, st);
}

/**
 * @test dpcp_qp.ti_36_sq_rq_to_reset_from_rts
 * @brief
 *    to_reset() (via modify_state(QP_RST)) -> DPCP_OK; query reads QP_RST.
 * @details
 */
TEST_F(dpcp_qp, ti_36_sq_rq_to_reset_from_rts)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->modify_state(QP_RST));
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, s_test_qp_rq->query_state(st));
    EXPECT_EQ(QP_RST, st);
}

/**
 * @test dpcp_qp.ti_37_sq_rq_transition_to_rts_oneshot
 * @brief
 *    Fresh local SQ+RQ TestQp; transition_to_rts() RST->RTS in one call.
 * @details
 */
TEST_F(dpcp_qp, ti_37_sq_rq_transition_to_rts_oneshot)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_rq(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    TestQp q(s_ad, a);
    ASSERT_EQ(DPCP_OK, setup_qp_chain(q));
    ASSERT_EQ(DPCP_OK, q.transition_to_rts());
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, q.query_state(st));
    EXPECT_EQ(QP_RTS, st);
}

/**
 * @test dpcp_qp.ti_38_sq_rq_destroy
 * @brief
 *    destroy() -> DPCP_OK; release the shared QP.
 * @details
 */
TEST_F(dpcp_qp, ti_38_sq_rq_destroy)
{
    ASSERT_TRUE(s_test_qp_rq != nullptr);
    EXPECT_EQ(DPCP_OK, s_test_qp_rq->destroy());
    s_test_qp_rq.reset();
}

/**
 * @brief Group reset_* - to_reset() from non-RTS source states.
 */

/**
 * @test dpcp_qp.ti_39_reset_from_init
 * @brief
 *    Fresh TestQp; drive to INIT; modify_state(QP_RST) -> DPCP_OK;
 *    query_state reads QP_RST.
 * @details
 *    QP_2RST is valid from any source state.
 */
TEST_F(dpcp_qp, ti_39_reset_from_init)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    TestQp q(s_ad, a);
    ASSERT_EQ(DPCP_OK, setup_qp_chain(q));
    ASSERT_EQ(DPCP_OK, q.modify_state(QP_INIT));
    ASSERT_EQ(DPCP_OK, q.modify_state(QP_RST));
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, q.query_state(st));
    EXPECT_EQ(QP_RST, st);
}

/**
 * @test dpcp_qp.ti_40_reset_from_rtr
 * @brief
 *    Fresh TestQp; drive to RTR; modify_state(QP_RST) -> DPCP_OK;
 *    query_state reads QP_RST.
 * @details
 */
TEST_F(dpcp_qp, ti_40_reset_from_rtr)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    TestQp q(s_ad, a);
    ASSERT_EQ(DPCP_OK, setup_qp_chain(q));
    ASSERT_EQ(DPCP_OK, q.modify_state(QP_INIT));
    ASSERT_EQ(DPCP_OK, q.modify_state(QP_RTR));
    ASSERT_EQ(DPCP_OK, q.modify_state(QP_RST));
    qp_state st = QP_ERR;
    ASSERT_EQ(DPCP_OK, q.query_state(st));
    EXPECT_EQ(QP_RST, st);
}

/**
 * @brief Group hooks_* - on_build_* invocation coverage.
 */

/**
 * @test dpcp_qp.ti_41_hooks_sq_only
 * @brief
 *    TestQpTraced SQ-only; create + transition_to_rts; each hook called 1x.
 * @details
 */
TEST_F(dpcp_qp, ti_41_hooks_sq_only)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    TestQpTraced q(s_ad, a);
    ASSERT_EQ(DPCP_OK, setup_qp_chain(q));
    ASSERT_EQ(DPCP_OK, q.transition_to_rts());
    EXPECT_EQ(1, q.hook_create);
    EXPECT_EQ(1, q.hook_rst2init);
    EXPECT_EQ(1, q.hook_init2rtr);
    EXPECT_EQ(1, q.hook_rtr2rts);
}

/**
 * @test dpcp_qp.ti_42_hooks_sq_rq
 * @brief
 *    TestQpTraced SQ+RQ; same 1x per hook - hooks fire regardless of RQ.
 * @details
 */
TEST_F(dpcp_qp, ti_42_hooks_sq_rq)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_rq(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    TestQpTraced q(s_ad, a);
    ASSERT_EQ(DPCP_OK, setup_qp_chain(q));
    ASSERT_EQ(DPCP_OK, q.transition_to_rts());
    EXPECT_EQ(1, q.hook_create);
    EXPECT_EQ(1, q.hook_rst2init);
    EXPECT_EQ(1, q.hook_init2rtr);
    EXPECT_EQ(1, q.hook_rtr2rts);
}

/**
 * @brief Group ext_* - caller-owned external buffer paths.
 */

/**
 * @test dpcp_qp.ti_43_ext_wq_buf_sq_only
 * @brief
 *    External wq_buf on SQ-only; get_wq_buf returns caller ptr; dtor skips free.
 * @details
 *    qp::destroy() does not free a caller-owned WQ buffer.
 */
TEST_F(dpcp_qp, ti_43_ext_wq_buf_sq_only)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    size_t sz = qp::get_wq_buf_sz(0, 0, a.sq_wqe_sz, a.sq_wqe_num);
    void* ext = ::aligned_alloc(get_page_size(), sz);
    ASSERT_NE(nullptr, ext);
    {
        TestQp q(s_ad, a);
        ASSERT_EQ(DPCP_OK, setup_qp_chain(q, ext, nullptr, sz));
        void* got = nullptr;
        ASSERT_EQ(DPCP_OK, q.get_wq_buf(got));
        EXPECT_EQ(ext, got);
        EXPECT_FALSE(q.m_owned_wq_buf);
    }
    /* If dtor mistakenly freed ext, this would double-free. */
    ::aligned_free(ext);
}

/**
 * @test dpcp_qp.ti_44_ext_dbrec_sq_only
 * @brief
 *    External db_rec on SQ-only; get_dbrec returns caller ptr; dtor skips free.
 * @details
 */
TEST_F(dpcp_qp, ti_44_ext_dbrec_sq_only)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    qp_db_rec* ext_db = static_cast<qp_db_rec*>(::aligned_alloc(get_cacheline_size(),
                                                                 qp::get_db_rec_sz()));
    ASSERT_NE(nullptr, ext_db);
    memset(ext_db, 0, qp::get_db_rec_sz());
    {
        TestQp q(s_ad, a);
        ASSERT_EQ(DPCP_OK, setup_qp_chain(q, nullptr, ext_db));
        qp_db_rec* got = nullptr;
        ASSERT_EQ(DPCP_OK, q.get_dbrec(got));
        EXPECT_EQ(ext_db, got);
        EXPECT_FALSE(q.m_owned_db_rec);
    }
    ::aligned_free(ext_db);
}

/**
 * @test dpcp_qp.ti_45_ext_wq_buf_sq_rq
 * @brief
 *    External wq_buf on SQ+RQ; sized for SQ+RQ sum; dtor does not free.
 * @details
 */
TEST_F(dpcp_qp, ti_45_ext_wq_buf_sq_rq)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_rq(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    size_t sz = qp::get_wq_buf_sz(a.rq_wqe_sz, a.rq_wqe_num, a.sq_wqe_sz, a.sq_wqe_num);
    void* ext = ::aligned_alloc(get_page_size(), sz);
    ASSERT_NE(nullptr, ext);
    {
        TestQp q(s_ad, a);
        ASSERT_EQ(DPCP_OK, setup_qp_chain(q, ext, nullptr, sz));
        void* got = nullptr;
        ASSERT_EQ(DPCP_OK, q.get_wq_buf(got));
        EXPECT_EQ(ext, got);
        EXPECT_FALSE(q.m_owned_wq_buf);
    }
    ::aligned_free(ext);
}

/**
 * @test dpcp_qp.ti_46_ext_dbrec_sq_rq
 * @brief
 *    External db_rec on SQ+RQ; dtor does not free.
 * @details
 */
TEST_F(dpcp_qp, ti_46_ext_dbrec_sq_rq)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_rq(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    qp_db_rec* ext_db = static_cast<qp_db_rec*>(::aligned_alloc(get_cacheline_size(),
                                                                qp::get_db_rec_sz()));
    ASSERT_NE(nullptr, ext_db);
    memset(ext_db, 0, qp::get_db_rec_sz());
    {
        TestQp q(s_ad, a);
        ASSERT_EQ(DPCP_OK, setup_qp_chain(q, nullptr, ext_db));
        qp_db_rec* got = nullptr;
        ASSERT_EQ(DPCP_OK, q.get_dbrec(got));
        EXPECT_EQ(ext_db, got);
        EXPECT_FALSE(q.m_owned_db_rec);
    }
    ::aligned_free(ext_db);
}

/**
 * @test dpcp_qp.ti_47_ext_wq_buf_undersized
 * @brief
 *    External wq_buf with undersized UMEM registration on SQ+RQ QP. DPCP
 *    itself does not validate buffer size, but FW rejects CREATE_QP when
 *    the UMEM is too small to cover log_sq_size + log_rq_size: observed
 *    return DPCP_ERR_CREATE.
 * @details
 *    Documents the validation boundary: DPCP passes the request through,
 *    but FW catches the undersized UMEM. Locks observed FW behavior.
 */
TEST_F(dpcp_qp, ti_47_ext_wq_buf_undersized)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_rq(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    /* Size only for the SQ portion - deliberately undersized for SQ+RQ. */
    size_t sq_only_sz = qp::get_wq_buf_sz(0, 0, a.sq_wqe_sz, a.sq_wqe_num);
    size_t full_sz = qp::get_wq_buf_sz(a.rq_wqe_sz, a.rq_wqe_num, a.sq_wqe_sz, a.sq_wqe_num);
    /* Allocate the larger size to keep the heap valid; UMEM is registered
     * at the undersized value to probe FW validation. */
    void* ext = ::aligned_alloc(get_page_size(), full_sz);
    ASSERT_NE(nullptr, ext);
    {
        TestQp q(s_ad, a);
        status ret = setup_qp_chain(q, ext, nullptr, sq_only_sz);
        log_trace("ti_47 status=%d (FW rejects undersized UMEM)\n", ret);
        EXPECT_EQ(DPCP_ERR_CREATE, ret);
    }
    ::aligned_free(ext);
}

/**
 * @test dpcp_qp.ti_48_ext_buf_sentinel_after_destroy
 * @brief
 *    Caller writes 0xAB sentinel byte before create; verifies sentinel
 *    intact after destroy and after the QP is released; proves the dtor
 *    did not free the buffer.
 * @details
 *    QP only goes through state transitions - no WQEs posted - so the FW
 *    will not write to the buffer; sentinel byte 0 should be preserved.
 */
TEST_F(dpcp_qp, ti_48_ext_buf_sentinel_after_destroy)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    size_t sz = qp::get_wq_buf_sz(0, 0, a.sq_wqe_sz, a.sq_wqe_num);
    uint8_t* ext = static_cast<uint8_t*>(::aligned_alloc(get_page_size(), sz));
    ASSERT_NE(nullptr, ext);
    /* Sentinel at the very end of the buffer (FW won't touch unused tail). */
    ext[sz - 1] = 0xAB;
    {
        auto q = std::unique_ptr<TestQp>(new (std::nothrow) TestQp(s_ad, a));
        ASSERT_TRUE(q != nullptr);
        ASSERT_EQ(DPCP_OK, setup_qp_chain(*q, ext, nullptr, sz));
        EXPECT_EQ(DPCP_OK, q->destroy());
        EXPECT_EQ((uint8_t)0xAB, ext[sz - 1]);
        q.reset();
        EXPECT_EQ((uint8_t)0xAB, ext[sz - 1]);
    }
    ::aligned_free(ext);
}

/**
 * @brief Group null_safe_* - accessor null-safety before init and after destroy.
 */

/**
 * @test dpcp_qp.ti_49_null_safe_pre_create
 * @brief
 *    TestQp constructed; init/create never called. Per accessor the
 *    returns differ: get_qpn -> DPCP_ERR_INVALID_ID;
 *    get_wq_buf/get_dbrec/get_uar_page -> DPCP_ERR_NO_MEMORY;
 *    get_bf_reg -> DPCP_ERR_NO_SUPPORT; cached-attr accessors -> DPCP_OK.
 *    Dtor handles partial state cleanly.
 * @details
 */
TEST_F(dpcp_qp, ti_49_null_safe_pre_create)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    {
        TestQp q(s_ad, a);

        /* Error-returning accessors. */
        uint32_t qpn = 99;
        EXPECT_EQ(DPCP_ERR_INVALID_ID, q.get_qpn(qpn));

        void* wb = reinterpret_cast<void*>(0x1);
        EXPECT_EQ(DPCP_ERR_NO_MEMORY, q.get_wq_buf(wb));

        qp_db_rec* db = reinterpret_cast<qp_db_rec*>(0x1);
        EXPECT_EQ(DPCP_ERR_NO_MEMORY, q.get_dbrec(db));

        volatile void* up = reinterpret_cast<void*>(0x1);
        EXPECT_EQ(DPCP_ERR_NO_MEMORY, q.get_uar_page(up));

        uint64_t* bf = reinterpret_cast<uint64_t*>(0x1);
        EXPECT_EQ(DPCP_ERR_NO_SUPPORT, q.get_bf_reg(bf));

        /* Cached-attr accessors return OK with cached values. */
        qp_state st = QP_ERR;
        EXPECT_EQ(DPCP_OK, q.get_state(st));
        EXPECT_EQ(QP_RST, st);

        uint32_t v = 0;
        EXPECT_EQ(DPCP_OK, q.get_sq_wqe_sz(v));
        EXPECT_EQ(TEST_SQ_WQE_SZ, v);
        EXPECT_EQ(DPCP_OK, q.get_sq_wqe_num(v));
        EXPECT_EQ(TEST_SQ_WQE_NUM, v);
        EXPECT_EQ(DPCP_OK, q.get_rq_wqe_sz(v));
        EXPECT_EQ(0U, v);
        EXPECT_EQ(DPCP_OK, q.get_rq_wqe_num(v));
        EXPECT_EQ(0U, v);
        EXPECT_EQ(DPCP_OK, q.get_cqn_snd(v));
        EXPECT_EQ(s_cqd_snd.cqn, v);
        EXPECT_EQ(DPCP_OK, q.get_cqn_rcv(v));
        EXPECT_EQ(s_cqd_rcv.cqn, v);
    }
    /* Dtor on never-created TestQp must not crash (qp::destroy
     * skips frees when pointers are null). */
    SUCCEED();
}

/**
 * @test dpcp_qp.ti_50_null_safe_post_destroy
 * @brief
 *    Fresh TestQp; setup_qp_chain + destroy(); accessors safe but with
 *    asymmetric returns: ptr-returning accessors -> NO_MEMORY / NO_SUPPORT,
 *    qpn accessor -> INVALID_ID (destroy() zeros m_qpn),
 *    attr accessors (cqn_snd) still return OK with stale values.
 * @details
 */
TEST_F(dpcp_qp, ti_50_null_safe_post_destroy)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    TestQp q(s_ad, a);
    ASSERT_EQ(DPCP_OK, setup_qp_chain(q));

    uint32_t qpn_pre = 0;
    ASSERT_EQ(DPCP_OK, q.get_qpn(qpn_pre));
    EXPECT_NE(0U, qpn_pre);

    EXPECT_EQ(DPCP_OK, q.destroy());

    /* After destroy: m_uar / m_wq_buf / m_db_rec are nullptr. */
    void* wb = nullptr;
    EXPECT_EQ(DPCP_ERR_NO_MEMORY, q.get_wq_buf(wb));
    qp_db_rec* db = nullptr;
    EXPECT_EQ(DPCP_ERR_NO_MEMORY, q.get_dbrec(db));
    volatile void* up = nullptr;
    EXPECT_EQ(DPCP_ERR_NO_MEMORY, q.get_uar_page(up));
    uint64_t* bf = nullptr;
    EXPECT_EQ(DPCP_ERR_NO_SUPPORT, q.get_bf_reg(bf));

    /* m_qpn is zeroed by destroy(); get_qpn() returns INVALID_ID. */
    uint32_t qpn_post = 0;
    EXPECT_EQ(DPCP_ERR_INVALID_ID, q.get_qpn(qpn_post));
    EXPECT_EQ(0U, qpn_post);

    /* Attr accessors return cached values (destroy() does not zero m_attr). */
    uint32_t v = 0;
    EXPECT_EQ(DPCP_OK, q.get_cqn_snd(v));
    EXPECT_EQ(s_cqd_snd.cqn, v);
}

/**
 * @brief Group illegal_* - degenerate configs and invalid attr.
 */

/**
 * @test dpcp_qp.ti_51_illegal_bad_cqn_snd
 * @brief
 *    Bogus cqn_snd (TEST_BOGUS_CQN) -> create() returns non-OK; teardown safe.
 * @details
 */
TEST_F(dpcp_qp, ti_51_illegal_bad_cqn_snd)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(TEST_BOGUS_CQN, s_cqd_rcv.cqn);
    TestQp q(s_ad, a);
    status ret = setup_qp_chain(q);
    EXPECT_NE(DPCP_OK, ret);
}

/**
 * @test dpcp_qp.ti_52_illegal_rq_only_no_sq
 * @brief
 *    sq_wqe_num=0, rq_wqe_num>0. qp::create() sets no_sq=1 AND configures
 *    the RQ. RC QP without an SQ is a valid HW configuration.
 *    Expected: create() succeeds.
 * @details
 *    Mis-labelled "illegal_*" for grouping; actually probes a legal-but-
 *    unusual config.
 */
TEST_F(dpcp_qp, ti_52_illegal_rq_only_no_sq)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_rq(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    a.sq_wqe_num = 0;
    a.sq_wqe_sz = 0;
    TestQp q(s_ad, a);
    status ret = setup_qp_chain(q);
    log_trace("ti_52 (rq_only no_sq) status=%d\n", ret);
    EXPECT_EQ(DPCP_OK, ret);
}

/**
 * @test dpcp_qp.ti_53_illegal_both_zero
 * @brief
 *    sq_wqe_num=0 AND rq_wqe_num=0. qp::create() sets no_sq=1 AND
 *    rq_type=ZERO_SIZE. Outcome is FW-dependent; documents current behavior.
 * @details
 */
TEST_F(dpcp_qp, ti_53_illegal_both_zero)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    a.sq_wqe_num = 0;
    a.sq_wqe_sz = 0;
    a.rq_wqe_num = 0;
    a.rq_wqe_sz = 0;
    TestQp q(s_ad, a);
    status ret = setup_qp_chain(q);
    log_trace("ti_53 (both zero) status=%d\n", ret);
    /* Lock current behavior - accept either outcome. */
    ASSERT_TRUE(ret == DPCP_OK || ret == DPCP_ERR_CREATE);
    if (ret == DPCP_OK) {
        uint32_t qpn = 0;
        EXPECT_EQ(DPCP_OK, q.get_qpn(qpn));
        EXPECT_NE(0U, qpn);
    }
}

/**
 * @test dpcp_qp.ti_54_illegal_double_create
 * @brief
 *    Successful create() then a second create() on same TestQp must return
 *    non-OK. The correct contract is that calling create() on an already-
 *    created QP is an error.
 * @details
 */
TEST_F(dpcp_qp, ti_54_illegal_double_create)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    TestQp q(s_ad, a);
    ASSERT_EQ(DPCP_OK, setup_qp_chain(q));
    status ret = q.create();
    log_trace("ti_54 (double_create) status=%d\n", ret);
    EXPECT_NE(DPCP_OK, ret);
}

/**
 * @test dpcp_qp.ti_55_illegal_double_init
 * @brief
 *    Two consecutive init(&uar) calls must surface non-OK on the second
 *    call. The correct contract is that init() is idempotent or rejects
 *    re-entry.
 * @details
 */
TEST_F(dpcp_qp, ti_55_illegal_double_init)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    TestQp q(s_ad, a);
    ASSERT_EQ(DPCP_OK, setup_qp_chain(q));
    status ret = q.init(&s_uar);
    log_trace("ti_55 (double_init) status=%d\n", ret);
    EXPECT_NE(DPCP_OK, ret);
}

/**
 * @test dpcp_qp.ti_56_illegal_modify_state_invalid_enum
 * @brief
 *    modify_state(static_cast<qp_state>(0xFF)) -> DPCP_ERR_NO_SUPPORT
 *    (the default arm of qp::modify_state's switch); cached m_state
 *    unchanged.
 * @details
 */
TEST_F(dpcp_qp, ti_56_illegal_modify_state_invalid_enum)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    TestQp q(s_ad, a);
    ASSERT_EQ(DPCP_OK, setup_qp_chain(q));
    qp_state st_before = QP_ERR;
    ASSERT_EQ(DPCP_OK, q.get_state(st_before));
    EXPECT_EQ(DPCP_ERR_NO_SUPPORT,
              q.modify_state(static_cast<qp_state>(0xFF)));
    qp_state st_after = QP_ERR;
    ASSERT_EQ(DPCP_OK, q.get_state(st_after));
    EXPECT_EQ(st_before, st_after);
}

/**
 * @test dpcp_qp.ti_57_illegal_bad_mtu
 * @brief
 *    attr.mtu=qp_mtu(0). MTU is consumed inside qp::on_build_init2rtr()
 *    and thus only reaches FW on the INIT2RTR transition. Expected:
 *    create() OK, RST2INIT OK, INIT2RTR fails.
 * @details
 */
TEST_F(dpcp_qp, ti_57_illegal_bad_mtu)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    a.mtu = static_cast<qp_mtu>(0);
    TestQp q(s_ad, a);
    ASSERT_EQ(DPCP_OK, setup_qp_chain(q));
    EXPECT_EQ(DPCP_OK, q.modify_state(QP_INIT));
    status ret = q.modify_state(QP_RTR);
    log_trace("ti_57 (bad_mtu) RTR status=%d\n", ret);
    EXPECT_NE(DPCP_OK, ret);
}

/**
 * @test dpcp_qp.ti_58_illegal_bad_service_type
 * @brief
 *    attr.st=qp_service_type(0xFF) must be rejected. The correct contract
 *    is that DPCP validates the service type before forwarding to FW.
 * @details
 */
TEST_F(dpcp_qp, ti_58_illegal_bad_service_type)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_only(s_cqd_snd.cqn, s_cqd_rcv.cqn);
    a.st = static_cast<qp_service_type>(0xFF);
    TestQp q(s_ad, a);
    status ret = setup_qp_chain(q);
    log_trace("ti_58 (bad_st) status=%d\n", ret);
    EXPECT_NE(DPCP_OK, ret);
}

/**
 * @test dpcp_qp.ti_59_illegal_rq_without_cqn_rcv
 * @brief
 *    rq_wqe_num>0 with cqn_rcv=TEST_BOGUS_CQN (bogus, not allocated).
 *    qp::create() forwards cqn_rcv to FW; FW rejects in CREATE_QP.
 * @details
 *    Final test - also tears down s_ad and s_uar_pool here.
 */
TEST_F(dpcp_qp, ti_59_illegal_rq_without_cqn_rcv)
{
    ASSERT_NE(nullptr, s_ad);
    qp_attr a = make_attr_sq_rq(s_cqd_snd.cqn, TEST_BOGUS_CQN);
    {
        TestQp q(s_ad, a);
        status ret = setup_qp_chain(q);
        log_trace("ti_59 (bad cqn_rcv) status=%d\n", ret);
        EXPECT_NE(DPCP_OK, ret);
    }
    /* Suite-level cleanup: this is the last case in the fixture. */
    s_uar_pool.reset();
    delete s_ad;
    s_ad = nullptr;
}
