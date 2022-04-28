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
#ifndef SRC_DPCP_INTERNAL_H_
#define SRC_DPCP_INTERNAL_H_

#include <cstring>
#include <memory>
#include <map>
#include <mutex>
#include <vector>
#include <atomic>
#include "dcmd/dcmd.h"
#include "api/dpcp.h"

namespace dpcp {

class rq;

class pd : public obj {
protected:
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

    virtual status create() = 0;
};

class pd_devx : public pd {
public:
    pd_devx(dcmd::ctx* ctx)
        : pd(ctx)
    {
    }
    virtual ~pd_devx()
    {
    }

    status create();
};

class pd_ibv : public pd {
private:
    void* m_ibv_pd;

public:
    pd_ibv(dcmd::ctx* ctx)
        : pd(ctx)
        , m_ibv_pd(nullptr)
    {
    }
    virtual ~pd_ibv()
    {
    }
    void* get_ibv_pd()
    {
        return m_ibv_pd;
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
 * After release_uar() call the UAR allocate for q_num goes to free pool and
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

class packet_pacing : public obj {
private:
    pp_handle* m_pp_handle;
    qos_packet_pacing m_attr;
    uint32_t m_index;

public:
    packet_pacing(dcmd::ctx* ctx, qos_packet_pacing& attr)
        : obj(ctx)
        , m_pp_handle(nullptr)
        , m_attr(attr)
        , m_index(0)
    {
    }

    virtual ~packet_pacing()
    {
        if (m_pp_handle) {
            devx_free_pp(m_pp_handle);
        }
    }

    uint32_t get_index() const
    {
        return m_index;
    }

    status create();
};

/**
 * @brief: Flow action interface.
 */
class flow_action : public obj {
public:
    flow_action(dcmd::ctx* ctx) : obj(ctx) {}
    /**
     * @brief Copy constructor
     *
     * @note: Copy flow_action object is not allowed.
     */
    flow_action(const flow_action& action) = delete;
    /**
     * @brief Assignment operator
     *
     * @note: Copy flow_action object is not allowed.
     */
    flow_action& operator=(const flow_action& action) = delete;
    /**
     * @brief Apply action to rule, will fill the rule create in buffer.
     *
     * @param [out] mb: flow rule create in buff.
     *
     * @retval Returns @ref dpcp::status with the status code
     */
    virtual status apply(void* in) = 0;
    virtual ~flow_action() = default;
};


/**
 * @brief: Flow action reformat support multiple types of actions that allow to change the packet header by th NIC.
 *         check supported modify types in @ref flow_action_reformat_type.
 *         user need to fill the asosiated type atributes for the ref flow_action_reformat_type chosen.
 */
class flow_action_reformat : public flow_action {
private:
    flow_action_reformat_attr m_attr;
    bool m_is_valid;
    uint32_t m_reformat_id;

public:
    flow_action_reformat(dcmd::ctx* ctx, flow_action_reformat_attr& attr);
    virtual ~flow_action_reformat();
    virtual status apply(void* in);
    virtual status get_id(uint32_t& id) override;

private:
    // Help functions
    status alloc_reformat_insert_action(void*& in, size_t& in_len, flow_action_reformat_attr& attr);
};

/**
 * @brief: Flow action modify support multiple types of modify packet header on supported fields
 *         @ref flow_action_modify_fiel, check supported modify types in @ref flow_action_reformat_type.
 *         user need to fill the asosiated type atributes for the ref flow_action_modify_type chosen.
 */
class flow_action_modify : public flow_action {
private:
    flow_action_modify_attr m_attr;
    bool m_is_valid;
    uint32_t m_modify_id;

public:
    flow_action_modify(dcmd::ctx* ctx, flow_action_modify_attr& attr);
    virtual ~flow_action_modify();
    virtual status apply(void* in);
    status get_num_actions(size_t& num);
    status apply_root(dcmd::modify_action* modify_actions);
    virtual status get_id(uint32_t& id) override;

private:
    // Help functions.
    void apply_modify_set_action(void* in, flow_action_modify_type_attr& attr);
    status create_prm_modify();
};


/**
 * @brief: Flow action tag, implement interface @ref flow_action.
 *         Will set tag on the packets matched by the flow rule.
 */
class flow_action_tag : public flow_action {
private:
    uint32_t m_tag_id;

public:
    flow_action_tag(dcmd::ctx* ctx, uint32_t id);
    virtual ~flow_action_tag() = default;
    virtual status apply(void* in);
    uint32_t get_tag_id();
};

/**
 * @brief: Flow action forward table, implement interface @ref flow_action.
 *         Forward table will provide destination of type @ref flow_table to the flow rule.
 */
class flow_action_fwd : public flow_action {
private:
    std::vector<obj*> m_dests;

public:
    flow_action_fwd(dcmd::ctx* ctx, std::vector<obj*> dests);
    virtual ~flow_action_fwd() = default;
    virtual status apply(void* in);
    size_t get_dest_num();
    const std::vector<obj*>& get_dest_objs();

private:
    // Help functions.
    status get_dst_attr(obj* dest, uint32_t& type, uint32_t& id);
};

/**
 * @brief: Flow matcher attributes
 */
struct flow_matcher_attr {
    match_params_ex match_criteria; /*< Flow matcher mactch crateria, the masks of the used fields */
    uint8_t match_criteria_enabled; /*< Flag that set the enabled cratiria that will be used in match_criteria
                                        @ref MLX5_CREATE_FLOW_GROUP_IN_MATCH_CRITERIA_* */
};

/**
 * @brief: Flow matcher is applying the mask/value acording to the mach cratiria that is provided.
 *         This object is used both in the fow_group and the flow_rule to set the match_params.
 *
 */
class flow_matcher {
private:
    flow_matcher_attr m_attr;

public:
    /**
     * @brief: Flow matcher constructor.
     *
     * @param [in] attr: fflow matcher attributes.
     */
    flow_matcher(const flow_matcher_attr& attr);
    /**
     * @brief: Apply match value/mask acording to the match cratiria provided.
     *
     * @param [out] match_params: PRM match params buffer @ref mlx5_ifc_fte_match_param_bits.
     */
    status apply(void* match_params, const match_params_ex& match_value) const;

private:
    // help functions
    status set_prog_sample_fileds(void* match_params, const match_params_ex& match_value) const;
    status set_outer_header_fields(void* match_params, const match_params_ex& match_value) const;
    status set_outer_header_lyr_4_fields(void* outer, const match_params_ex& match_value) const;
    status set_outer_header_lyr_3_fields(void* outer, const match_params_ex& match_value) const;
    status set_outer_header_lyr_2_fields(void* outer, const match_params_ex& match_value) const;
};



} // namespace dpcp

#endif /* SRC_DPCP_INTERNAL_H_ */
