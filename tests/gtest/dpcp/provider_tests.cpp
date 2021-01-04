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
#include "common/sys.h"
#include "common/base.h"

#include "dpcp_base.h"

using namespace dpcp;

class dpcp_provider : public dpcp_base {
};

/**
 * @test dpcp_provider.ti_1
 * @brief
 *    Check singleton
 * @details
 */
TEST_F(dpcp_provider, ti_1)
{
    provider* provider1;
    status ret = provider1->get_instance(provider1);
    ASSERT_EQ(DPCP_OK, ret);

    dpcp::provider* provider2;
    ret = provider2->get_instance(provider2);
    ASSERT_EQ(DPCP_OK, ret);

    ASSERT_EQ((uintptr_t)provider1, (uintptr_t)provider2);
}

/**
 * @test dpcp_provider.ti_2
 * @brief
 *    Check get_adapter_info_lst
 * @details
 *
 */
TEST_F(dpcp_provider, ti_2)
{
    provider* p;
    status ret = p->get_instance(p);
    ASSERT_EQ(DPCP_OK, ret);

    size_t num = 0;
    ret = p->get_adapter_info_lst(nullptr, num);

    ASSERT_EQ(DPCP_ERR_OUT_OF_RANGE, ret);
    log_trace("Adapters: %zd\n", num);

    ret = p->get_adapter_info_lst(nullptr, num);

    ASSERT_EQ(DPCP_ERR_OUT_OF_RANGE, ret);
    adapter_info* ai = new (std::nothrow) adapter_info[num];

    ret = p->get_adapter_info_lst(ai, num);
    ASSERT_EQ(DPCP_OK, ret);

    for (unsigned i = 0; i < num; i++) {
        log_trace("[%u] id: %s name: %s\n", i, ai->id.c_str(), ai->name.c_str());
        ai++;
    }
}

/**
 * @test dpcp_provider.ti_3
 * @brief
 *    Check create_adapter
 * @details
 *
 */
TEST_F(dpcp_provider, ti_3)
{
    provider* p;
    status ret = p->get_instance(p);
    ASSERT_EQ(DPCP_OK, ret);

    adapter* ad1 = nullptr;
    ret = p->open_adapter("", ad1);
    ASSERT_EQ(DPCP_ERR_INVALID_ID, ret);
    ASSERT_EQ(nullptr, ad1);

    size_t num = 0;
    ret = p->get_adapter_info_lst(nullptr, num);
    adapter_info* p_ainfo = new (std::nothrow) adapter_info[num];
    ret = p->get_adapter_info_lst(p_ainfo, num);
    ASSERT_EQ(DPCP_OK, ret);

    adapter_info* ai = p_ainfo;

    ret = p->open_adapter(ai->id, ad1);
    ASSERT_EQ(DPCP_OK, ret);
    log_trace("id: %s name: %s adapter: %p\n", ai->id.c_str(), ai->name.c_str(), ad1);

    ai = p_ainfo + 1;
    adapter* ad2 = nullptr;
    ret = p->open_adapter(ai->id, ad2);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(ad1, ad2);
    log_trace("id: %s name: %s adapter: %p\n", ai->id.c_str(), ai->name.c_str(), ad2);
}
