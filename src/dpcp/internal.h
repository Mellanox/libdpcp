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

// Forward declarations
class rq;

struct prm_match_params {
    size_t buf_sz;
    uint32_t buf[DEVX_ST_SZ_DW(fte_match_param)];
};

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
    bool m_is_external_ibv_pd;

public:
    pd_ibv(dcmd::ctx* ctx, void* ibv_pd = nullptr)
        : pd(ctx)
        , m_ibv_pd(ibv_pd)
        , m_is_external_ibv_pd(ibv_pd != nullptr)
    {
    }

    virtual ~pd_ibv()
    {
        if (!m_is_external_ibv_pd && m_ibv_pd) {
            ibv_dealloc_pd((struct ibv_pd*)m_ibv_pd);
        }
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
    flow_action(dcmd::ctx* ctx)
        : obj(ctx)
    {
    }
    /**
     * @brief Copy constructor.
     *
     * @note: Copy flow_action object is not allowed.
     */
    flow_action(const flow_action& action) = delete;
    /**
     * @brief Assignment operator.
     *
     * @note: Copy flow_action object is not allowed.
     */
    flow_action& operator=(const flow_action& action) = delete;
    /**
     * @brief Apply action to rule, will fill the rule create in buffer.
     *
     * @param [out] mb: flow rule create in buff.
     *
     * @retval Returns @ref dpcp::status with the status code.
     */
    virtual status apply(void* in) = 0;
    /**
     * @brief Apply action to rule, will fill the rule create in buffer.
     *
     * @param [out] flow_desc: flow rule description structure.
     *
     * @retval Returns @ref dpcp::status with the status code.
     */
    virtual status apply(dcmd::flow_desc& flow_desc) = 0;
    virtual ~flow_action() = default;
};

/**
 * @brief: Flow action reformat support multiple types of actions that allow to change the packet
 *         header by th NIC. check supported modify types in @ref flow_action_reformat_type.
 *         user need to fill the associated type attributes for the ref flow_action_reformat_type
 *         chosen.
 */
class flow_action_reformat : public flow_action {
private:
    flow_action_reformat_attr m_attr;
    bool m_is_valid;
    uint32_t m_reformat_id;

public:
    flow_action_reformat(dcmd::ctx* ctx, flow_action_reformat_attr& attr);
    virtual ~flow_action_reformat();
    virtual status apply(void* in) override;
    virtual status apply(dcmd::flow_desc& flow_desc) override;
    virtual status get_id(uint32_t& id) override;

private:
    // Help functions
    status alloc_reformat_insert_action(std::unique_ptr<uint8_t[]>& in_mem_guard, size_t& in_len,
                                        flow_action_reformat_attr& attr);
};

/**
 * @brief: Flow action modify support multiple types of modify packet header on supported fields
 *         @ref flow_action_modify_fiel, check supported modify types in
 *         @ref flow_action_reformat_type. user need to fill the associated type attributes
 *         for the @ref flow_action_modify_type chosen.
 */
class flow_action_modify : public flow_action {
private:
    flow_action_modify_attr m_attr;
    bool m_is_valid;
    uint32_t m_modify_id;
    std::unique_ptr<dcmd::modify_action, std::default_delete<dcmd::modify_action[]>> m_actions_root;
    uint32_t m_out[DEVX_ST_SZ_DW(alloc_modify_header_context_out)] {0};
    size_t m_outlen = sizeof(m_out);
    std::unique_ptr<uint8_t[]> m_in;
    size_t m_inlen = 0;

public:
    flow_action_modify(dcmd::ctx* ctx, flow_action_modify_attr& attr);
    virtual ~flow_action_modify();
    virtual status apply(void* in) override;
    virtual status apply(dcmd::flow_desc& flow_desc) override;
    status get_num_actions(size_t& num);
    virtual status get_id(uint32_t& id) override;

private:
    // Help functions.
    void apply_modify_set_action(void* in, flow_action_modify_type_attr& attr);
    void apply_modify_copy_action(void* in, flow_action_modify_type_attr& attr);
    status create_prm_modify();
    status prepare_flow_desc_buffs();
    status prepare_prm_modify_buff();
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
    virtual status apply(void* in) override;
    virtual status apply(dcmd::flow_desc& flow_desc) override;
};

/**
 * @brief: Flow action forward table, implement interface @ref flow_action.
 *         Forward table will provide destination of type @ref flow_table to the flow rule.
 */
class flow_action_fwd : public flow_action {
private:
    std::vector<forwardable_obj*> m_dests;
    std::unique_ptr<dcmd::action_fwd> m_root_action_fwd;

public:
    flow_action_fwd(dcmd::ctx* ctx, std::vector<forwardable_obj*> dests);
    virtual ~flow_action_fwd() = default;
    virtual status apply(void* in) override;
    virtual status apply(dcmd::flow_desc& flow_desc) override;
    size_t get_dest_num();
    const std::vector<forwardable_obj*>& get_dest_objs();

private:
    // Help functions.
    status create_root_action_fwd();
};

/**
 * @brief: Flow action reparse, triggers HW packet reparse.
 */
class flow_action_reparse : public flow_action {
public:
    flow_action_reparse(dcmd::ctx* ctx);
    virtual ~flow_action_reparse() = default;
    virtual status apply(void* in) override;
    virtual status apply(dcmd::flow_desc& flow_desc) override;
};

/**
 * @brief: Flow matcher attributes
 */
struct flow_matcher_attr {
    match_params_ex match_criteria; /*< Flow matcher match criteria, the masks of the
                                        used fields */
    uint8_t match_criteria_enabled; /*< Flag that set the enabled criteria that will be used in
                                       match_criteria
                                        @ref MLX5_CREATE_FLOW_GROUP_IN_MATCH_CRITERIA_* */
};

/**
 * @brief: Flow matcher is applying the mask/value according to the match criteria that is provided.
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
     * @param [in] attr: flow matcher attributes.
     */
    flow_matcher(const flow_matcher_attr& attr);
    /**
     * @brief: Apply match value/mask according to the match criteria provided.
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
    status set_metadata_registers_fields(void* match_params,
                                         const match_params_ex& match_value) const;
    status set_metadata_register_0_field(void* metadata_registers,
                                         const match_params_ex& match_value) const;
};

/**
 * @brief flow_table_prm class, implements flow_table interface.
 */
class flow_table_prm : public flow_table {
    friend class adapter;

private:
    uint32_t m_table_id;
    flow_table_attr m_attr;

public:
    virtual status create() override;
    virtual status query(flow_table_attr& attr) override;
    status get_table_id(uint32_t& table_id) const;
    virtual status get_table_level(uint8_t& table_level) const override;
    virtual status add_flow_group(const flow_group_attr& attr,
                                  std::weak_ptr<flow_group>& group) override;
    virtual ~flow_table_prm() = default;

private:
    /**
     * @brief flow_table_kernel - constructor is private should be called only from
     *        @ref adapter::create_flow_table.
     */
    flow_table_prm(dcmd::ctx* ctx, const flow_table_attr& attr);
    status set_miss_action(void* in);
};

/**
 * @brief flow_table_kernel class, implements flow_table abstract class.
 */
class flow_table_kernel : public flow_table {
    friend class adapter;

    // Kernel flow table level can be in range of 0-64.
    // We use the MAX level to avoid conflicts with other tables.
    // If we use flow rule to forward to other flow table from kernel table,
    // we must use higher level then 64 to avoid forwarding to lower level (not allowed)
    static constexpr uint8_t DEFAULT_LEVEL = 64;
    static constexpr uint8_t DEFAULT_LOG_SIZE = 16;

public:
    virtual status create() override;
    virtual status query(flow_table_attr& attr) override;
    virtual status get_table_level(uint8_t& table_level) const override;
    virtual status add_flow_group(const flow_group_attr& attr,
                                  std::weak_ptr<flow_group>& group) override;
    virtual ~flow_table_kernel() = default;

private:
    /**
     * @brief flow_table_kernel - constructor is private should be called only from
     *        @ref adapter::get_root_table.
     */
    flow_table_kernel(dcmd::ctx* ctx, flow_table_type type);
};

class flow_group_prm : public flow_group {
    friend class flow_table;

private:
    uint32_t m_group_id;

public:
    virtual status create() override;
    virtual status add_flow_rule(const flow_rule_attr_ex& attr,
                                 std::weak_ptr<flow_rule_ex>& rule) override;
    status get_group_id(uint32_t& group_id) const;
    status get_table_id(uint32_t& table_id) const;
    virtual ~flow_group_prm() = default;

private:
    /**
     * @brief flow_group_prm - constructor is private should be called only from
     *        @ref flow_table_prm::add_flow_group.
     */
    flow_group_prm(dcmd::ctx* ctx, const flow_group_attr& attr,
                   std::weak_ptr<const flow_table> table);
};

class flow_group_kernel : public flow_group {
    friend class flow_table;

public:
    virtual status create() override;
    virtual status add_flow_rule(const flow_rule_attr_ex& attr,
                                 std::weak_ptr<flow_rule_ex>& rule) override;
    virtual ~flow_group_kernel() = default;

private:
    /**
     * @brief flow_group_kernel - constructor is private should be called only from
     *        @ref flow_table_kernel::add_flow_group.
     */
    flow_group_kernel(dcmd::ctx* ctx, const flow_group_attr& attr,
                      std::weak_ptr<const flow_table> table);
};

class flow_rule_ex_prm : public flow_rule_ex {
    friend class flow_group;

private:
    uint32_t m_flow_index;

public:
    virtual status create() override;
    virtual ~flow_rule_ex_prm() = default;

private:
    /**
     * @brief flow_rule_ex_prm - constructor is private should be called only from
     *        @ref flow_group_prm::add_flow_rule.
     */
    flow_rule_ex_prm(dcmd::ctx* ctx, const flow_rule_attr_ex& attr,
                     std::weak_ptr<const flow_table> table, std::weak_ptr<const flow_group> group,
                     std::shared_ptr<const flow_matcher> matcher);

    // Help functions.
    status alloc_in_buff(size_t& in_len, std::unique_ptr<uint8_t[]>& in_mem_guard);
    status config_flow_rule(void* in);
};

class flow_rule_ex_kernel : public flow_rule_ex {
    friend class flow_group;

private:
    uint16_t m_priority;
    dcmd::flow* m_flow;

public:
    virtual status create() override;
    virtual ~flow_rule_ex_kernel();

private:
    /**
     * @brief flow_rule_ex_kernel - constructor is private should be called only from
     *        @ref flow_group_kernel::add_flow_rule.
     */
    flow_rule_ex_kernel(dcmd::ctx* ctx, const flow_rule_attr_ex& attr,
                        std::weak_ptr<const flow_table> table,
                        std::weak_ptr<const flow_group> group,
                        std::shared_ptr<const flow_matcher> matcher);

    // Help functions
    status set_match_params(dcmd::flow_desc& flow_desc, prm_match_params& criteria,
                            prm_match_params& values);
};

} // namespace dpcp

#endif /* SRC_DPCP_INTERNAL_H_ */
