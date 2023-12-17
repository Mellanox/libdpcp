/*
 * Copyright (c) 2019-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
#include "common/sys.h"
#include "common/base.h"
#include "common/cmn.h"

#include "dpcp_base.h"

using namespace dpcp;

class dpcp_fr : /*public obj,*/ public dpcp_base {
public:
    static std::unique_ptr<adapter> s_ad;
    static std::unique_ptr<flow_rule> s_fr3t;
    static std::unique_ptr<flow_rule> s_fr4t;
    static std::unique_ptr<flow_rule> s_fr5t;
    static std::unique_ptr<flow_rule> s_fr5t_ipv6;

protected:
    match_params m_mask3 = { {}, 0xFFFF, 0, 0, 0, 0xFFFF, 0, 0xFF, 0xF }; // DMAC, SMAC, DST_IP, DST_PORT, Protocol, IP Version
    match_params m_mask4 = { {}, 0xFFFF, 0, 0, 0, 0xFFFF, 0, 0xFF, 0xF }; // DMAC, SMAC, DST_IP, SRC_IP, DST_PORT, Protocol, IP Version
    match_params m_mask5 = { {}, 0xFFFF, 0xFFFF, 0, 0, 0xFFFF, 0xFFFF, 0xFF, 0xF }; // DMAC, SMAC, ETHER_TYPE, VLAN_ID, DST_IP, SRC_IP, DST_PORT, SRC_PORT, Protocol, IP Version

    rq_params m_rqp;

    void SetUp() override
    {
        if (errno) {
            log_trace("dpcp_rq::SetUp errno= %d\n", errno);
            errno = EOK;
        }
        if (nullptr == s_ad) {
            s_ad.reset(OpenAdapter());
            ASSERT_NE(nullptr, s_ad);
        }
        memset(&m_mask3.dst, 0xff, sizeof(m_mask3.dst));
        memset(&m_mask3.src, 0x0, sizeof(m_mask3.src));
        if (nullptr == s_fr3t) {
            s_fr3t.reset(new (std::nothrow) flow_rule(s_ad->get_ctx(), 10, m_mask3));
        }
        memset(&m_mask4.dst, 0xff, sizeof(m_mask4.dst));
        memset(&m_mask4.src, 0xff, sizeof(m_mask4.src));
        if (nullptr == s_fr4t) {
            s_fr4t.reset(new (std::nothrow) flow_rule(s_ad->get_ctx(), 10, m_mask4));
        }
        memset(&m_mask5.dst, 0xff, sizeof(m_mask5.dst));
        memset(&m_mask5.src, 0xff, sizeof(m_mask5.src));
        if (nullptr == s_fr5t) {
            s_fr5t.reset(new (std::nothrow) flow_rule(s_ad->get_ctx(), 10, m_mask5));
        }
        if (nullptr == s_fr5t_ipv6) {
            s_fr5t_ipv6.reset(new (std::nothrow) flow_rule(s_ad->get_ctx(), 10, m_mask5));
        }
        m_rqp = {{2048, 16384, 0, 0}, 4, 0};

        m_rqp.wqe_sz = m_rqp.rq_at.buf_stride_num * m_rqp.rq_at.buf_stride_sz / 16; // in DS (16B)
    }
    void TearDown()
    {
    }
};

std::unique_ptr<adapter> dpcp_fr::s_ad;
std::unique_ptr<flow_rule> dpcp_fr::s_fr3t;
std::unique_ptr<flow_rule> dpcp_fr::s_fr4t;
std::unique_ptr<flow_rule> dpcp_fr::s_fr5t;
std::unique_ptr<flow_rule> dpcp_fr::s_fr5t_ipv6;

static striding_rq* s_srq1 = nullptr;
static striding_rq* s_srq2 = nullptr;
static tir* s_tir1 = nullptr;
static tir* s_tir2 = nullptr;

/**
 * @test dpcp_fr.ti_01_Constructor
 * @brief
 *    Check flow_rule constructor
 * @details
 *
 */
TEST_F(dpcp_fr, ti_01_constructor)
{
    ASSERT_NE(nullptr, s_fr3t);
}

/**
 * @test dpcp_fr.ti_02_get_priority
 * @brief
 *    Check flow_rule::get_priority()
 * @details
 */
TEST_F(dpcp_fr, ti_02_get_priority)
{
    uint16_t prior = 0;
    status ret = s_fr3t->get_priority(prior);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(10, prior);
}

/**
 * @test dpcp_fr.ti_03_match_value
 * @brief
 *    Check flow_rule::set_match_value/get_match_value()
 * @details
 */
TEST_F(dpcp_fr, ti_03_match_value)
{
    match_params mv0 = {};
    match_params mvr = {};
    status ret = s_fr3t->get_match_value(mvr);
    ASSERT_EQ(DPCP_OK, ret);
    int i = memcmp(&mv0, &mvr, sizeof(mv0));
    ASSERT_EQ(0, i);

    match_params mv1;
    memset(&mv1, 0, sizeof(mv1));
    mv1.ethertype = 0x0800;
    mv1.dst_port = 0x4321;
    mv1.protocol = 0x11;
    mv1.ip_version = 4;
    mv1.dst.ipv4 = 0x12345678;
    ret = s_fr3t->set_match_value(mv1);
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr3t->get_match_value(mvr);
    ASSERT_EQ(DPCP_OK, ret);
    i = memcmp(&mv1, &mvr, sizeof(mv1));
    ASSERT_EQ(0, i);
}

/**
 * @test dpcp_fr.ti_04_flow_id
 * @brief
 *    Check flow_rule::set_flow_id/get_flow_id()
 * @details
 */
TEST_F(dpcp_fr, ti_04_flow_id)
{
    uint32_t fid0 = 0;
    uint32_t fidr = 0;
    status ret = s_fr3t->get_flow_id(fidr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(0, fidr);

    fid0 = 1;
    ret = s_fr3t->set_flow_id(fid0);
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr3t->get_flow_id(fidr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(fidr, fid0);

    fid0 = 0xFFFFF;
    ret = s_fr3t->set_flow_id(fid0);
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr3t->get_flow_id(fidr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(fidr, fid0);

    fid0 = 0xFFFFF + 1;
    ret = s_fr3t->set_flow_id(fid0);
    ASSERT_EQ(DPCP_ERR_OUT_OF_RANGE, ret);

    ret = s_fr3t->get_flow_id(fidr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(fidr, fid0 - 1);

    ret = s_fr3t->set_flow_id(0);
    ASSERT_EQ(DPCP_OK, ret);
    ret = s_fr3t->get_flow_id(fidr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(0, fidr);
}

/**
 * @test dpcp_fr.ti_05_set_tir
 * @brief
 *    Check flow_rule::set_dest_tir
 * @details
 */
TEST_F(dpcp_fr, ti_05_set_dest_tir)
{
    status ret = s_ad->open();
    ASSERT_EQ(DPCP_OK, ret);
    s_srq1 = open_str_rq(s_ad.get(), m_rqp);
    ASSERT_NE(s_srq1, nullptr);
    uint32_t rq_id = 0;
    ret = s_srq1->get_id(rq_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rq_id);

    uint32_t nt = 0;
    ret = s_fr3t->get_num_tirs(nt);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(0, nt);

    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = TIR_ATTR_INLINE_RQN | TIR_ATTR_TRANSPORT_DOMAIN;
    tir_attr.inline_rqn = rq_id;
    tir_attr.transport_domain = s_ad->get_td();
    ret = s_ad->create_tir(tir_attr, s_tir1);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, s_tir1);

    ret = s_fr3t->add_dest_tir(s_tir1);
    ASSERT_EQ(DPCP_OK, ret);
    ret = s_fr3t->get_num_tirs(nt);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(1, nt);

    s_srq2 = open_str_rq(s_ad.get(), m_rqp);
    ASSERT_NE(nullptr, s_srq2);
    uint32_t rq2_id = 0;
    ret = s_srq2->get_id(rq2_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rq2_id);

    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = TIR_ATTR_INLINE_RQN | TIR_ATTR_TRANSPORT_DOMAIN;
    tir_attr.inline_rqn = rq2_id;
    tir_attr.transport_domain = s_ad->get_td();
    ret = s_ad->create_tir(tir_attr, s_tir2);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(nullptr, s_tir2);

    ret = s_fr3t->add_dest_tir(s_tir2);
    ASSERT_EQ(DPCP_OK, ret);
    ret = s_fr3t->get_num_tirs(nt);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(2, nt);
}

/**
 * @test dpcp_fr.ti_06_remove_dest_tir
 * @brief
 *    Check flow_rule::remove_dest_tir
 * @details
 */
TEST_F(dpcp_fr, ti_06_remove_dest_tir)
{
    uint32_t nt = 0;
    status ret = s_fr3t->get_num_tirs(nt);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(2, nt);

    ret = s_fr3t->remove_dest_tir(s_tir2);
    ASSERT_EQ(DPCP_OK, ret);
    ret = s_fr3t->get_num_tirs(nt);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(1, nt);

    ret = s_fr3t->remove_dest_tir(s_tir2);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);
    ret = s_fr3t->get_num_tirs(nt);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(1, nt);

    ret = s_fr3t->remove_dest_tir(s_tir1);
    ASSERT_EQ(DPCP_OK, ret);
    ret = s_fr3t->get_num_tirs(nt);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(0, nt);

    ret = s_fr3t->remove_dest_tir(s_tir1);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);
    ret = s_fr3t->get_num_tirs(nt);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(0, nt);
}

/**
 * @test dpcp_fr.ti_07_apply_settings
 * @brief
 *    Check flow_rule::apply_settings
 * @details
 */
TEST_F(dpcp_fr, ti_07_apply_settings)
{
#ifdef __linux__
    SKIP_TRUE(sys_rootuser(), "This test requires root permissions (RAW_NET)\n");
#endif
    status ret = s_fr3t->add_dest_tir(s_tir2);
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr3t->apply_settings();
    ASSERT_EQ(DPCP_OK, ret);
}

/**
 * @test dpcp_fr.ti_08_revoke_settings
 * @brief
 *    Check flow_rule::revoke_settings
 * @details
 *
 */
TEST_F(dpcp_fr, ti_08_revoke_settings)
{
    status ret = s_fr3t->revoke_settings();
    ASSERT_EQ(DPCP_OK, ret);
}

/**
* @test dpcp_fr.ti_09_4_tuple
* @brief
*    Check flow_rule::apply_settings for 4 tuple rule
* @details
*/
TEST_F(dpcp_fr, ti_09_4_tuple)
{
#ifdef __linux__
    SKIP_TRUE(sys_rootuser(), "This test requires root permissions (RAW_NET)\n");
#endif
    match_params mv0 = {};
    match_params mvr = {};
    status ret = s_fr4t->get_match_value(mvr);
    ASSERT_EQ(DPCP_OK, ret);
    int i = memcmp(&mv0, &mvr, sizeof(mv0));
    ASSERT_EQ(0, i);

    match_params mv1;
    memset(&mv1, 0, sizeof(mv1));
    mv1.ethertype = 0x0800;
    mv1.dst_port = 0x4321;
    mv1.protocol = 0x11;
    mv1.ip_version = 4;
    mv1.dst.ipv4 = 0x12345678;
    mv1.src.ipv4 = 0x87654321;
    ret = s_fr4t->set_match_value(mv1);
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr4t->get_match_value(mvr);
    ASSERT_EQ(DPCP_OK, ret);
    i = memcmp(&mv1, &mvr, sizeof(mv1));
    ASSERT_EQ(0, i);

    ret = s_fr4t->add_dest_tir(s_tir2);
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr4t->apply_settings();
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr4t->revoke_settings();
    ASSERT_EQ(DPCP_OK, ret);
}

/**
* @test dpcp_fr.ti_10_5_tuple
* @brief
*    Check flow_rule::apply_settings for 5 tuple rule
* @details
*/
TEST_F(dpcp_fr, ti_10_5_tuple)
{
#ifdef __linux__
    SKIP_TRUE(sys_rootuser(), "This test requires root permissions (RAW_NET)\n");
#endif
    match_params mv0 = {};
    match_params mvr = {};
    status ret = s_fr5t->get_match_value(mvr);
    ASSERT_EQ(DPCP_OK, ret);
    int i = memcmp(&mv0, &mvr, sizeof(mv0));
    ASSERT_EQ(0, i);

    match_params mv1;
    memset(&mv1, 0, sizeof(mv1));
    mv1.ethertype = 0x0800;
    mv1.vlan_id = 0x0004;
    mv1.dst_port = 0x4321;
    mv1.src_port = 0x4322;
    mv1.protocol = 0x11;
    mv1.ip_version = 4;
    mv1.dst.ipv4 = 0x12345678;
    mv1.src.ipv4 = 0x87654321;
    ret = s_fr5t->set_match_value(mv1);
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr5t->get_match_value(mvr);
    ASSERT_EQ(DPCP_OK, ret);
    i = memcmp(&mv1, &mvr, sizeof(mv1));
    ASSERT_EQ(0, i);

    ret = s_fr5t->add_dest_tir(s_tir2);
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr5t->apply_settings();
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr5t->revoke_settings();
    ASSERT_EQ(DPCP_OK, ret);
}

/**
* @test dpcp_fr.ti_11_5t_ipv6
* @brief
*    Check flow_rule::apply_settings for 5 tuple rule ipv6
* @details
*/
TEST_F(dpcp_fr, ti_11_5t_ipv6)
{
#ifdef __linux__
    SKIP_TRUE(sys_rootuser(), "This test requires root permissions (RAW_NET)\n");

    match_params mv0 = {};
    match_params mvr = {};
    status ret = s_fr5t_ipv6->get_match_value(mvr);
    ASSERT_EQ(DPCP_OK, ret);
    int i = memcmp(&mv0, &mvr, sizeof(mv0));
    ASSERT_EQ(0, i);

    match_params mv1;
    uint8_t dst[16] = {0x11, 0x12, 0x13, 0x14, 0x21, 0x22, 0x23, 0x24, 0x31, 0x32, 0x33, 0x34, 0x41, 0x42, 0x43, 0x44};
    uint8_t src[16] = {0x11, 0x12, 0x13, 0x14, 0x21, 0x22, 0x23, 0x24, 0x61, 0x62, 0x63, 0x64, 0x71, 0x72, 0x73, 0x74};
    memset(&mv1, 0, sizeof(mv1));
    mv1.ethertype = 0x86DD;
    mv1.vlan_id = 0x0004;
    mv1.dst_port = 0x4321;
    mv1.src_port = 0x4322;
    mv1.protocol = 0x11;
    mv1.ip_version = 6;
    memcpy(&mv1.dst.ipv6, dst, sizeof(dst));
    memcpy(&mv1.src.ipv6, src, sizeof(src));
    ret = s_fr5t_ipv6->set_match_value(mv1);
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr5t_ipv6->get_match_value(mvr);
    ASSERT_EQ(DPCP_OK, ret);
    i = memcmp(&mv1, &mvr, sizeof(mv1));
    ASSERT_EQ(0, i);

    ret = s_fr5t_ipv6->add_dest_tir(s_tir2);
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr5t_ipv6->apply_settings();
    ASSERT_EQ(DPCP_OK, ret);

    ret = s_fr5t_ipv6->revoke_settings();
    ASSERT_EQ(DPCP_OK, ret);
#endif

    /* Keep these cleanup in last test */
    delete s_tir2;
    delete s_tir1;
    delete s_srq2;
    delete s_srq1;
}
