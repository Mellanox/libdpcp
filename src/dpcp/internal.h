/*
 * Copyright (C) Mellanox Technologies, Ltd. 2020. ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Mellanox Technologies, Ltd.
 * (the "Company") and all right, title, and interest in and to the software
 * product, including all associated intellectual property rights, are and shall
 * remain exclusively with the Company. All rights in or to the software product
 * are licensed, not sold. All rights not licensed are reserved.
 *
 * This software product is governed by the End User License Agreement provided
 * with the software product.
 */
#pragma once

#include <cstring>
#include <map>
#include <mutex>
#include <vector>
#include "dcmd/dcmd.h"
#include "api/dpcp.h"

namespace dpcp {

class rq;

class pd : public obj {
    uint32_t m_pd_id;

public:
    pd(dcmd::ctx* ctx)
        : obj(ctx)
        , m_pd_id(0)
    {
    }
    virtual ~pd()
    {
    }

    uint32_t get_pd_id() const
    {
        return m_pd_id;
    }

    status create();
};

class td : public obj {
    uint32_t m_td_id;

public:
    td(dcmd::ctx* ctx)
        : obj(ctx)
        , m_td_id(0)
    {
    }
    virtual ~td()
    {
    }

    uint32_t get_td_id() const
    {
        return m_td_id;
    }

    status create();
};

enum uar_type { SHARED_UAR, EXCLUSIVE_UAR };

struct uar_t {
    volatile void* m_page;
    volatile void* m_bf_reg;
    uint32_t m_page_id;

    bool operator==(const uar_t& u2) const
    {
        return !memcmp(this, &u2, sizeof(u2));
    }
};

typedef std::multimap<const void*, dcmd::uar*> excl_uar_map;
typedef std::vector<const void*> shar_uar_vec;

/**
 * @brief Internal class, responsible to handle UARs collection.
 * It uses lazy allocation, per get_uar() request for particular r q_num.
 * After release_uar() call the UAR allocatec for q_num goes to free pool and
 * net get_uar() call will reuse it.
 */
class uar_collection {
    std::mutex m_mutex;
    excl_uar_map m_ex_uars;
    shar_uar_vec m_sh_vc;
    dcmd::ctx* m_ctx;
    uar m_shared_uar;

    uar allocate();
    uar add_uar(const void* p_key, uar u);
    void free(uar u);

public:
    uar_collection(dcmd::ctx* ctx);
    virtual ~uar_collection();

    uar get_uar(const void* p_key, uar_type u_type = SHARED_UAR);

    status release_uar(const void* p_key);

    status get_uar_page(const uar u, uar_t& u_dsc);

    inline size_t num_uars(void)
    {
        return m_ex_uars.size() + (m_shared_uar ? 1 : 0);
    }
    inline uint32_t num_shared(void)
    {
        return (uint32_t)m_sh_vc.size();
    }

    uar_collection(uar_collection const&) = delete;
    void operator=(uar_collection const&) = delete;
};

/**
 * @brief Calculates log2 of integer argument
 *
 * @retval Returns log2 or -1 if argument <= 0.
 */
inline int ilog2(int n)
{
    int e = 0;

    if (n <= 0) {
        return -1;
    }
    while ((1 << e) < n) {
        ++e;
    }

    return e;
}
} // namespace dpcp