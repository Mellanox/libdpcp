/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2020-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include "dpcp_base.h"

using namespace dpcp;

class dpcp_tir : public dpcp_base {};

/**
 * @test dpcp_tir.ti_01_Constructor
 * @brief
 *    Check constructor
 * @details
 */
TEST_F(dpcp_tir, ti_01_Constructor)
{
    status ret = DPCP_OK;
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    tir tir_obj(adapter_obj->get_ctx());

    uintptr_t handle = 0;
    ret = tir_obj.get_handle(handle);
    ASSERT_EQ(DPCP_ERR_CREATE, ret);
    ASSERT_EQ(0, handle);

    uint32_t id = 0;
    ret = tir_obj.get_id(id);
    ASSERT_EQ(DPCP_ERR_INVALID_ID, ret);
    ASSERT_EQ(0, id);

    delete adapter_obj;
}

/**
 * @test dpcp_tir.ti_02_create
 * @brief
 *    Check tir::create method with wrong parameters
 * @details
 *
 */
TEST_F(dpcp_tir, ti_02_create)
{
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    status ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = adapter_obj->get_td();
    ASSERT_NE(0U, tdn);

    striding_rq* srq_obj = open_str_rq(adapter_obj, m_rqp);
    ASSERT_NE(nullptr, srq_obj);

    uint32_t rqn = 0;
    ret = srq_obj->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rqn);
    log_trace("tdn: 0x%x rqn: 0x%x\n", tdn, rqn);

    tir tir_obj(adapter_obj->get_ctx());

    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = 0;
    ret = tir_obj.create(tir_attr);
    ASSERT_EQ(DPCP_ERR_CREATE, ret);

    delete srq_obj;
    delete adapter_obj;
}

/**
 * @test dpcp_tir.ti_03_create
 * @brief
 *    Check tir::create method
 * @details
 *
 */
TEST_F(dpcp_tir, ti_03_create)
{
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    status ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = adapter_obj->get_td();
    ASSERT_NE(0U, tdn);

    striding_rq* srq_obj = open_str_rq(adapter_obj, m_rqp);
    ASSERT_NE(nullptr, srq_obj);

    uint32_t rqn = 0;
    ret = srq_obj->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rqn);
    log_trace("tdn: 0x%x rqn: 0x%x\n", tdn, rqn);

    tir tir_obj(adapter_obj->get_ctx());

    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = TIR_ATTR_INLINE_RQN | TIR_ATTR_TRANSPORT_DOMAIN;
    tir_attr.inline_rqn = rqn;
    tir_attr.transport_domain = tdn;
    ret = tir_obj.create(tir_attr);
    ASSERT_EQ(DPCP_OK, ret);

    delete srq_obj;
    delete adapter_obj;
}

/**
 * @test dpcp_tir.ti_04_create_twice
 * @brief
 *    tir can not be created more than once
 * @details
 *
 */
TEST_F(dpcp_tir, ti_04_create_twice)
{
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    status ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = adapter_obj->get_td();
    ASSERT_NE(0U, tdn);

    striding_rq* srq_obj = open_str_rq(adapter_obj, m_rqp);
    ASSERT_NE(nullptr, srq_obj);

    uint32_t rqn = 0;
    ret = srq_obj->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rqn);
    log_trace("tdn: 0x%x rqn: 0x%x\n", tdn, rqn);

    tir tir_obj(adapter_obj->get_ctx());

    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = TIR_ATTR_INLINE_RQN | TIR_ATTR_TRANSPORT_DOMAIN;
    tir_attr.inline_rqn = rqn;
    tir_attr.transport_domain = tdn;
    ret = tir_obj.create(tir_attr);
    ASSERT_EQ(DPCP_OK, ret);

    ret = tir_obj.create(tir_attr);
    ASSERT_EQ(DPCP_ERR_INVALID_PARAM, ret);

    delete srq_obj;
    delete adapter_obj;
}

/**
 * @test dpcp_tir.ti_05_create_invalid
 * @brief
 *    Check tir creation with invalid parameters
 * @details
 *
 */
TEST_F(dpcp_tir, ti_05_create_invalid)
{
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    status ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    tir tir_obj(adapter_obj->get_ctx());

    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    ret = tir_obj.create(tir_attr);
    ASSERT_EQ(DPCP_ERR_CREATE, ret);

    tir_attr.flags = TIR_ATTR_INLINE_RQN;
    tir_attr.inline_rqn = 0xFF;
    ret = tir_obj.create(tir_attr);
    ASSERT_EQ(DPCP_ERR_CREATE, ret);

    delete adapter_obj;
}

/**
 * @test dpcp_tir.ti_06_query
 * @brief
 *    Check tir::query method
 * @details
 *
 */
TEST_F(dpcp_tir, ti_06_query)
{
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    status ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = adapter_obj->get_td();
    ASSERT_NE(0U, tdn);

    striding_rq* srq_obj = open_str_rq(adapter_obj, m_rqp);
    ASSERT_NE(nullptr, srq_obj);

    uint32_t rqn = 0;
    ret = srq_obj->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rqn);
    log_trace("tdn: 0x%x rqn: 0x%x\n", tdn, rqn);

    tir tir_obj(adapter_obj->get_ctx());

    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = TIR_ATTR_INLINE_RQN | TIR_ATTR_TRANSPORT_DOMAIN;
    tir_attr.inline_rqn = rqn;
    tir_attr.transport_domain = tdn;
    ret = tir_obj.create(tir_attr);
    ASSERT_EQ(DPCP_OK, ret);

    memset(&tir_attr, 0, sizeof(tir_attr));
    ret = tir_obj.query(tir_attr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_TRUE((tir_attr.flags & TIR_ATTR_INLINE_RQN) != 0);
    ASSERT_EQ(rqn, tir_attr.inline_rqn);
    ASSERT_TRUE((tir_attr.flags & TIR_ATTR_TRANSPORT_DOMAIN) != 0);
    ASSERT_EQ(tdn, tir_attr.transport_domain);

    delete srq_obj;
    delete adapter_obj;
}

/**
 * @test dpcp_tir.ti_07_modify
 * @brief
 *    Check tir::modify method
 * @details
 *
 */
TEST_F(dpcp_tir, ti_07_modify)
{
    adapter* adapter_obj = OpenAdapter();
    ASSERT_NE(nullptr, adapter_obj);

    status ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = adapter_obj->get_td();
    ASSERT_NE(0U, tdn);

    striding_rq* srq_obj = open_str_rq(adapter_obj, m_rqp);
    ASSERT_NE(nullptr, srq_obj);

    uint32_t rqn = 0;
    ret = srq_obj->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rqn);
    log_trace("tdn: 0x%x rqn: 0x%x\n", tdn, rqn);

    tir tir_obj(adapter_obj->get_ctx());

    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = TIR_ATTR_INLINE_RQN | TIR_ATTR_TRANSPORT_DOMAIN;
    tir_attr.inline_rqn = rqn;
    tir_attr.transport_domain = tdn;
    ret = tir_obj.create(tir_attr);
    ASSERT_EQ(DPCP_OK, ret);

    memset(&tir_attr, 0, sizeof(tir_attr));
    ret = tir_obj.query(tir_attr);
    ASSERT_EQ(DPCP_OK, ret);

    adapter_hca_capabilities caps;
    ret = adapter_obj->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    if (caps.lro_cap) {
        uint32_t enable_mask_original = tir_attr.lro.enable_mask;
        tir_attr.flags = TIR_ATTR_LRO;
        tir_attr.lro.enable_mask = (enable_mask_original ? 0 : 1);
        ret = tir_obj.modify(tir_attr);
        ASSERT_EQ(DPCP_OK, ret);

        memset(&tir_attr, 0, sizeof(tir_attr));
        ret = tir_obj.query(tir_attr);
        ASSERT_EQ(DPCP_OK, ret);
        ASSERT_TRUE((tir_attr.flags & TIR_ATTR_LRO) != 0U);
        ASSERT_NE(enable_mask_original, tir_attr.lro.enable_mask);
    }

    delete srq_obj;
    delete adapter_obj;
}

/**
 * @test dpcp_tir.ti_08_create_tls
 * @brief
 *    Check tir::create with tls
 * @details
 *
 */
TEST_F(dpcp_tir, ti_08_create_tls)
{
    std::unique_ptr<adapter> adapter_obj(OpenAdapter());
    ASSERT_NE(nullptr, adapter_obj);

    status ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t tdn = adapter_obj->get_td();
    ASSERT_NE(0U, tdn);

    std::unique_ptr<striding_rq> srq_obj(open_str_rq(adapter_obj.get(), m_rqp));
    ASSERT_NE(nullptr, srq_obj);

    uint32_t rqn = 0;
    ret = srq_obj->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rqn);
    log_trace("tdn: 0x%x rqn: 0x%x\n", tdn, rqn);

    tir tir_obj(adapter_obj->get_ctx());

    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = TIR_ATTR_INLINE_RQN | TIR_ATTR_TRANSPORT_DOMAIN;
    tir_attr.inline_rqn = rqn;
    tir_attr.transport_domain = tdn;

    adapter_hca_capabilities caps;
    ret = adapter_obj->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);

    if (caps.tls_rx) {
        tir_attr.flags |= TIR_ATTR_TLS;
        tir_attr.tls_en = 1U;
        log_trace("TLS-RX supported\n");
    } else {
        log_info("TLS-RX NOT supported\n");
        return;
    }

    ret = tir_obj.create(tir_attr);
    ASSERT_EQ(DPCP_OK, ret);

    memset(&tir_attr, 0, sizeof(tir_attr));
    ret = tir_obj.query(tir_attr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(tir_attr.tls_en, 1);

    if (caps.tls_rx && caps.lro_cap) {
        uint32_t enable_mask_original = tir_attr.lro.enable_mask;
        tir_attr.flags = TIR_ATTR_LRO;
        tir_attr.lro.enable_mask = (enable_mask_original ? 0 : 1);
        tir_attr.lro.max_msg_sz = (32768U >> 8U);
        tir_attr.lro.timeout_period_usecs = 32U;
        ret = tir_obj.modify(tir_attr);
        ASSERT_EQ(DPCP_OK, ret);

        memset(&tir_attr, 0, sizeof(tir_attr));
        ret = tir_obj.query(tir_attr);
        ASSERT_EQ(DPCP_OK, ret);
        ASSERT_TRUE((tir_attr.flags & TIR_ATTR_LRO) != 0U);
        ASSERT_NE(enable_mask_original, tir_attr.lro.enable_mask);
        ASSERT_EQ(tir_attr.tls_en, 1);
    }
}

/**
 * @test dpcp_tir.ti_09_create_nvmeotcp
 * @brief
 *    Check tir::create with nvmeotcp
 * @details
 *
 */
TEST_F(dpcp_tir, ti_09_create_nvmeotcp)
{
    std::unique_ptr<adapter> adapter_obj(OpenAdapter());
    ASSERT_NE(nullptr, adapter_obj);

    status ret = adapter_obj->open();
    ASSERT_EQ(DPCP_OK, ret);

    adapter_hca_capabilities caps;
    ret = adapter_obj->get_hca_capabilities(caps);
    ASSERT_EQ(DPCP_OK, ret);
    bool nvmeotcp_en = caps.nvmeotcp_caps.enabled;
    bool crc_en = caps.nvmeotcp_caps.crc_rx;
    bool zerocopy_en = caps.nvmeotcp_caps.zerocopy;
    uint32_t buffer_size_cap = caps.nvmeotcp_caps.log_max_nvmeotcp_tag_buffer_size;
    log_trace("NVMEoTCP %s\n", nvmeotcp_en ? "supported" : "not supported");
    log_trace("crc_rx %s\n", crc_en ? "supported" : "not supported");
    log_trace("zerocopy %s\n", zerocopy_en ? "supported" : "not supported");
    if (!nvmeotcp_en) {
        return;
    }

    uint32_t tdn = adapter_obj->get_td();
    ASSERT_NE(0U, tdn);

    std::unique_ptr<striding_rq> srq_obj(open_str_rq(adapter_obj.get(), m_rqp));
    ASSERT_NE(nullptr, srq_obj);

    uint32_t rqn = 0;
    ret = srq_obj->get_id(rqn);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0U, rqn);
    log_trace("tdn: 0x%x rqn: 0x%x\n", tdn, rqn);

    tag_buffer_table_obj tag_buff(adapter_obj->get_ctx());
    tir tir_obj(adapter_obj->get_ctx());
    struct tir::attr tir_attr;
    memset(&tir_attr, 0, sizeof(tir_attr));
    tir_attr.flags = TIR_ATTR_INLINE_RQN | TIR_ATTR_TRANSPORT_DOMAIN;
    tir_attr.inline_rqn = rqn;
    tir_attr.transport_domain = tdn;

    if (crc_en) {
        tir_attr.flags |= TIR_ATTR_NVMEOTCP_CRC;
        tir_attr.nvmeotcp.crc_en = 1U;
    }

    if (zerocopy_en) {
        // Create tag buffer table object
        struct tag_buffer_table_obj::attr tag_buff_attr = {};
        tag_buff_attr.log_tag_buffer_table_size = buffer_size_cap;
        ret = tag_buff.create(tag_buff_attr);
        ASSERT_EQ(DPCP_OK, ret);

        // Verify create
        ret = tag_buff.query(tag_buff_attr);
        ASSERT_EQ(DPCP_OK, ret);
        ASSERT_EQ(buffer_size_cap, tag_buff_attr.log_tag_buffer_table_size);

        log_trace("Tag Buffer Table (index=%d) attr:\n", tag_buff.get_key_id());
        log_trace("          modify_field_select=0x%x\n", tag_buff_attr.modify_field_select);
        log_trace("          log_tag_buffer_table_size=0x%x\n",
                  tag_buff_attr.log_tag_buffer_table_size);

        // Use tag buffer table object for TIR
        tir_attr.flags |= TIR_ATTR_NVMEOTCP_ZERO_COPY;
        tir_attr.nvmeotcp.zerocopy_en = 1U;
        tir_attr.nvmeotcp.tag_buffer_table_id = tag_buff.get_key_id();
    }

    ret = tir_obj.create(tir_attr);
    ASSERT_EQ(DPCP_OK, ret);

    ret = tir_obj.query(tir_attr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(tir_attr.nvmeotcp.crc_en, crc_en ? 1U : 0U);
    ASSERT_EQ(tir_attr.nvmeotcp.zerocopy_en, zerocopy_en ? 1U : 0U);
}
