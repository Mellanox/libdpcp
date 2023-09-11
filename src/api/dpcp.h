/*
 * Copyright (c) 2019-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef DPCP_H_
#define DPCP_H_

#include <bitset>
#include <string>
#include <cstring>
#include <vector>
#include <memory>
#include <unordered_set>
#include <typeinfo>
#include <typeindex>

#if __cplusplus < 201103L
#include <stdint.h>
#else
#include <cstdint>
#endif

#include <functional>
#include <unordered_map>

using std::function;
using std::unordered_map;

static const char* dpcp_version = "1.1.46";

#if defined(__linux__)
typedef void* LPOVERLAPPED;
#if !defined(EVENT_CHANNEL)
typedef int event_channel;
#define EVENT_CHANNEL
#endif
#else
#if !defined(EVENT_CHANNEL)
typedef HANDLE event_channel;
#define EVENT_CHANNEL
#endif
#endif

namespace dcmd {
// DCMD forward declarations.
class ctx;
class device;
class flow;
class obj;
class provider;
class uar;
class umem;
class compchannel;
struct modify_action;
struct fwd_dst_desc;
} // namespace dcmd

namespace dpcp {

// DPCP forward declarations
class adapter;
class flow_table;
class flow_group;
class flow_rule;
class flow_action;
class flow_rule_ex;
class flow_matcher;
class pd;
class td;
class uar_collection;
struct flow_table_attr;
struct flow_group_attr;
struct flow_rule_attr_ex;
struct uar_t;
struct adapter_hca_capabilities;

enum status {
    DPCP_OK = 0, /**< Operation finished successfully*/
    DPCP_ERR_NO_SUPPORT = -1, /**< Feature is not supported yet */
    DPCP_ERR_NO_PROVIDER = -2, /**< Provider wasn't found */
    DPCP_ERR_NO_DEVICES = -3, /**< No devices was found */
    DPCP_ERR_NO_MEMORY = -4, /**< No memory for operation */
    DPCP_ERR_OUT_OF_RANGE = -5, /**< Parameter is not in allowed range (out of limits) */
    DPCP_ERR_INVALID_ID = -6, /**< ID is invalid */
    DPCP_ERR_NO_CONTEXT = -7, /**< PRM Object has wrong context (not initialized) */
    DPCP_ERR_INVALID_PARAM = -8, /**< Invalid input parameter */
    DPCP_ERR_CREATE = -9, /**< Error on PRM Object create */
    DPCP_ERR_MODIFY = -10, /**< Error on PRM Object modify */
    DPCP_ERR_QUERY = -11, /**< Error on PRM Object query */
    DPCP_ERR_UMEM = -12, /**< Error with UMEM - allocation, mapping.. */
    DPCP_ERR_ALLOC_UAR = -13, /**< Error with UAR allocation */
    DPCP_ERR_NOT_APPLIED = -14 /**< Flow is different on HW vs current */
};

enum dpcp_ibq_protocol {
    DPCP_IBQ_2110 = 0x0, /**< 16 bit RTP sequence number */
    DPCP_IBQ_2110_EXT = 0x1, /**< 32 bit RTP sequence number */
    DPCP_IBQ_ORAN_ECPRI = 0x4, /**< ORAN eCPRI protocol */
    DPCP_IBQ_NOT_INITIALIZED
};

class obj {
    uint32_t m_id;
    dcmd::obj* m_obj_handle;
    dcmd::ctx* m_ctx;
    uint32_t m_last_status;
    uint32_t m_last_syndrome;

public:
    /**
     * @brief Object constructor, object is initialized but not created yet
     *
     * @param [in]  ctx           Pointer to adapter context
     *
     */
    obj(dcmd::ctx* ctx);
    virtual ~obj(); // destroy_handle handled in DTR

    obj(obj const&) = delete;
    void operator=(obj const&) = delete;

    /**
     * @brief Returns object Id
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_id(uint32_t& id);
    /**
     * @brief Returns object handle
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_handle(uintptr_t& handle) const;

    virtual dcmd::ctx* get_ctx()
    {
        return m_ctx;
    }

    /**
     * @brief Creates object in HW
     *
     * @param [in]  in              Pointer to input structure describing object
     * @param [in]  in_sz           Size of input structure in bytes
     * @param [in/out] out          Pointer to buffer with operation result
     * @param [in/out] out_sz       For input - size of output buffer, returns
     *                              real size of output buffer
     * @retval Returns DPCP_OK on success.
     */
    status create(void* in, size_t in_sz, void* out, size_t& out_sz);
    /**
     * @brief Modifies object in HW
     *
     * @param [in]  in              Pointer to input structure describing object
     * @param [in]  in_sz           Size of input structure in bytes
     * @param [in/out] out          Pointer to buffer with operation result
     * @param [in/out] out_sz       For input - size of output buffer, returns
     *                              real size of output buffer
     * @retval Returns DPCP_OK on success.
     */
    status modify(void* in, size_t in_sz, void* out, size_t& out_sz);
    /**
     * @brief Query object in HW
     *
     * @param [in]  in              Pointer to input structure describing object
     * @param [in]  in_sz           Size of input structure in bytes
     * @param [in/out] out          Pointer to buffer with operation result
     * @param [in/out] out_sz       For input - size of output buffer, returns
     *                              real size of output buffer
     * @retval Returns DPCP_OK on success.
     */
    status query(void* in, size_t in_sz, void* out, size_t& out_sz);
    virtual status destroy();
};

/**
 * @brief: Forward supported object.
 *
 * Represent an object that packets can be forwarded
 * to by @ref create_fwd, this means that this object can receive
 * forwarded packets as part of the steering e.g. @ref class flow_table, @ref class tir.
 * Any object that can support flow action forward should inherit from this class.
 */
class forwardable_obj : public obj {
public:
    /**
     * @brief Forwardable Object constructor.
     *
     * @param [in]  ctx           Pointer to adapter context.
     */
    forwardable_obj(dcmd::ctx* ctx);
    /**
     * @brief Get forward type.
     */
    virtual int get_fwd_type() const = 0;
    /**
     * @brief Get Forward object description.
     *
     * @param [out]  desc          Object description.
     */
    status get_fwd_desc(dcmd::fwd_dst_desc& desc);
    virtual ~forwardable_obj() = default;
};

typedef dcmd::uar* uar;

enum mkey_flags {
    MKEY_NONE = 0, // dummy flag
    MKEY_ZERO_BASED = 1 << 0 // mkey address space starts at 0
};

class mkey : public obj {
protected:
    mkey(dcmd::ctx* ctx)
        : obj(ctx)
    {
    }
    static void init_mkeys(void);

public:
    virtual status get_address(void*& address) = 0;
    virtual status get_length(size_t& len) = 0;
    virtual status get_flags(mkey_flags& flags) = 0;
    /**
     * @brief Returns global MKey counter
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_mkey_num(int& num);
    virtual ~mkey()
    {
    }
};

/**
 * @brief class direct_mkey - Represent direct Memory Key
 *
 */
class direct_mkey : public mkey {
    friend class provider;
    friend class adapter;
    adapter* m_adapter;
    dcmd::umem* m_umem;
    void* m_address;
    void* m_ibv_mem;
    size_t m_length;
    mkey_flags m_flags;
    uint32_t m_idx; // memory key index

    status destroy();

public:
    /**
     * @brief Constructor of direct_mkey
     * It receives memory region allocated by user.
     *
     * @param [in]  pd              Pointer to ProtectionDomain
     * @param [in]  address         Virtual Address
     * @param [in]  length          Address Length in bytes
     * @param [in]  mkey_flags      Flags
     */
    direct_mkey(adapter* ad, void* address, size_t length, mkey_flags flags);
    /**
     * @brief Registers User MEMory in driver and HW
     * Size and pointer were provided in CTR.
     *
     * @retval Returns DPCP_OK on success.
     */
    status reg_mem(void* verbs_pd = NULL);
    /**
     * @brief Creates Memory Key for registered UMEM region
     *
     * @retval Returns DPCP_OK on success.
     */
    status create();
    /**
     * @brief Returns virtual address of memory region.
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_address(void*& address); // override;
    /**
     * @brief Returns length of memory region
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_length(size_t& len); // override;
    /**
     * @brief Returns memory region flags
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_flags(mkey_flags& flags);
    /**
     * @brief Returns MKEY ID created by create()
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_id(uint32_t& id); // override;
    /**
     * @brief Deregister memory and delete MKey
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual ~direct_mkey();
};

class indirect_mkey : public mkey {

protected:
    indirect_mkey(adapter* ad);
    virtual ~indirect_mkey() {};
    /**
     * @brief Returns length of memory region
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_length(size_t& len)
    {
        len = 0;
        return DPCP_OK;
    }

public:
    virtual status get_mkeys_num(size_t& num) = 0;
    virtual status get_mkeys_lst(mkey*& lst) = 0;
};
/**
 * @brief struct pattern_mkey_bb - Pattern Mkey stride building block attributes
 *
 */
struct pattern_mkey_bb {
    mkey* m_key;
    size_t m_stride_sz;
    size_t m_length;
};
/**
 * @brief class pattern_mkey - Represent Pattern Memory Key
 *
 * Application can create a dpcp::pattern_mkey only via
 * dpcp::adapter->create_pattern_mkey().
 *
 * dpcp::adapter is responsible to create the dpcp::pattern_mkey instance.
 */
class pattern_mkey : public indirect_mkey {
    friend class provider;
    friend class adapter;
    adapter* m_adapter;
    pattern_mkey_bb* m_bbs_arr;
    mkey** m_mkeys_arr;
    void* m_address;
    size_t m_stride_sz;
    size_t m_stride_num;
    size_t m_bbs_num;
    mkey_flags m_flags;
    uint32_t m_idx; // memory key index

    pattern_mkey(pattern_mkey& p)
        : indirect_mkey(p.m_adapter)
    {
    }

public:
    pattern_mkey(adapter* ad, void* addr, mkey_flags flags, size_t stride_num, size_t bb_num,
                 pattern_mkey_bb* bbs);
    virtual ~pattern_mkey();
    /**
     * @brief Creates Memory Key
     *
     * @retval Returns DPCP_OK on success.
     */
    status create();
    /**
     * @brief Returns virtual address of memory region.
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_address(void*& address); // override;
    /**
     * @brief Returns length of memory region
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_length(size_t& len);
    /**
     * @brief Returns memory region flags
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_flags(mkey_flags& flags);
    /**
     * @brief Returns number of memory keys in pattern
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_mkeys_num(size_t& mkeys_num);
    /**
     * @brief Returns array of memory keys in pattern
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_mkeys_lst(mkey*& arr);
    /**
     * @brief Returns combined stride size
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_stride_sz(size_t& stride_sz);
    /**
     * @brief Returns pattern mkey strides number
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_stride_num(size_t& stride_num);
    /**
     * @brief Returns MKEY ID created by create()
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_id(uint32_t& id); // override;
};

enum reserved_mkey_type { MKEY_RESERVED_NONE = 0, MKEY_RESERVED_DUMP_AND_FILL = 1 };

class reserved_mkey : public mkey {
    friend class adapter;
    void* m_address;
    size_t m_length;
    uint32_t m_idx; // memory key index
    reserved_mkey_type m_type;
    mkey_flags m_flags;

public:
    reserved_mkey(adapter* ad, reserved_mkey_type type, void* address, size_t length,
                  mkey_flags flags);
    /**
     * @brief Creates Reserved Memory Key for given type
     *
     * @retval Returns DPCP_OK on success.
     */
    status create();
    /**
     * @brief Returns virtual address of memory region.
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_address(void*& address); // override;
    /**
     * @brief Returns length of memory region
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_length(size_t& len); // override;
    /**
     * @brief Returns flags
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_flags(mkey_flags& flags);
    /**
     * @brief Returns type of reserved MKey.
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_type(reserved_mkey_type& type);
    /**
     * @brief Returns MKEY ID created by create()
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_id(uint32_t& id); // override;
    /**
     * @brief Deregister memory and delete MKey
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual ~reserved_mkey();
};

/**
 * Base class for the reference-key types
 */
class base_ref_mkey : public mkey {
protected:
    friend class adapter;
    void* m_address;
    size_t m_length;
    uint32_t m_idx; // memory key index
    mkey_flags m_flags;

public:
    base_ref_mkey(adapter* ad, void* address, size_t length, uint32_t idx);

    /**
     * @brief Returns virtual address of memory region.
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_address(void*& address); // override;

    /**
     * @brief Returns length of memory region
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_length(size_t& len); // override;

    /**
     * @brief Returns memory region flags
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_flags(mkey_flags& flags); // override;

    /**
     * @brief Returns MKEY ID created by create()
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_id(uint32_t& id); // override;
};

/**
 * @brief class ref_mkey - References memory region associated with another (parent) memory key.
 *
 * This class is used to pass subregions of one or more registered memory
 * regions into @ref pattern_mkey. Creation method validates that memory region
 * specified by address and length is a subregion of parent's memory.
 */
class ref_mkey : public base_ref_mkey {
public:
    /**
     * @brief Constructor of ref_mkey
     *
     * @param [in]  ad              Pointer to Adapter
     * @param [in]  address         Virtual Address
     * @param [in]  length          Address Length in bytes
     */
    ref_mkey(adapter* ad, void* address, size_t length);

    /**
     * @brief Creates Memory Key referncing parent key memory
     *
     * @param [in]  parent          Parent memory key
     * @retval Returns DPCP_OK on success.
     */
    status create(mkey* parent);
};

/**
 * @brief Refers a memory key that is already registered
 *
 * Creates a valid {@link dpcp#mkey mkey} that refers an already
 * registered memory key that is associated with the specified memory range.
 */
class extern_mkey : public base_ref_mkey {
public:
    /**
     * @brief Constructor of extern_mkey
     *
     * @param [in]  ad              Pointer to Adapter
     * @param [in]  address         Virtual Address
     * @param [in]  length          Address Length in bytes
     * @param [in]  id              Valid id of externally registered key
     */
    extern_mkey(adapter* ad, void* address, size_t length, uint32_t id);
};

class crypto_mkey : public mkey {
    adapter* m_adapter;
    uint32_t m_idx; // memory key index
    const uint32_t m_max_sge; // max number of scatter gather elements

public:
    /**
     * @brief Constructor of crypto_mkey
     *
     * @param [in]  ad              Pointer to Adapter
     * @param [in]  max_sge         Max number of scatter gather elements
     */
    explicit crypto_mkey(adapter* ad, uint32_t max_sge);
    virtual ~crypto_mkey() = default;
    /**
     * @brief Creates Memory Key for given type
     *
     * @retval Returns DPCP_OK on success.
     */
    status create();
    /*
     * @brief Returns virtual address of memory region.
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_address(void*& address) override;
    /**
     * @brief Returns length of memory region
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_length(size_t& len) override;
    /**
     * @brief Returns memory region flags
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_flags(mkey_flags& flags) override;
    /**
     * @brief Returns MKEY ID created by create()
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_id(uint32_t& id) override;
};

/**
 * @brief enum cq_attr_use - set name for attributes which are valid and to be
 * used or modified
 *
 */
enum event_type {
    EVENT_TYPE_CQE = 0, /**< CQ ARM Completion */
    EVENT_TYPE_CQ, /**< CQ Events/errors */
    EVENT_TYPE_QP, /**< QP Events/errors */
    EVENT_TYPE_GEN /**< Generic Events/errors */
};

/**
 * @brief struct eq_context - provides storage for event queue data shared
 *                            between DPCP/DCMD internals and caller
 *
 */
struct eq_context {
    LPOVERLAPPED p_overlapped;
    uint32_t num_eqe;
};

class cq;
/**
 * @brief class eq - Base class for CompletionChannel and EventQueue
 *
 */
class eq : public obj {
    friend class adapter;

public:
    eq(dcmd::ctx* ctx)
        : obj(ctx)
    {
    }
    virtual ~eq()
    {
    }

    virtual status bind(cq& to_bind) = 0;
    virtual status unbind(cq& to_unbind) = 0;
    virtual status get_comp_channel(event_channel*& ch) = 0;
    virtual status request(cq& for_cq, eq_context& eq_ctx) = 0;
    virtual status flush(cq& for_cq) = 0;
};

/**
 * @brief class comp_channel - class for CompletionChannel implementation
 *
 */
class comp_channel : public eq {
    friend class adapter;
    dcmd::compchannel* m_cc;

public:
    comp_channel(adapter* ad);

    virtual ~comp_channel();
    /**
     * @brief Bind comp_channel with specific CQ
     * @param [in] to_bind      CQ to bind
     *
     * @retval Returns DPCP_OK on success.
     */
    status bind(cq& to_bind);
    /**
     * @brief UnBind comp_channel from CQ if possible
     * @param [in] to_unbind      Unbind the CQ
     *
     * @retval Returns DPCP_OK on success.
     */
    status unbind(cq& to_unbind);
    /**
     * @brief Returns Completion Channel
     * @param [out] ch      returned completion channel
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_comp_channel(event_channel*& ch);
    /**
     * @brief Request notification from completion channel
     * @param [in] for_cq      Request for specific CQ
     * @param [in/out] eq_ctx  EventQueue Context with shared parameters
     *
     * @retval Returns DPCP_OK on success.
     */
    status request(cq& for_cq, eq_context& eq_ctx);
    /**
     * @brief Flushes/acts events from completion channel
     * @param [in] for_cq      Request for specific CQ
     *
     * @retval Returns DPCP_OK on success.
     */
    status flush(cq& for_cq);
};

/**
 * @brief enum cq_attr_use - set name for attributes which are valid and to be
 * used or modified
 *
 */
enum cq_attr_use {
    CQ_SIZE = 0, /**< size of CQ will be set, mandatory for create */
    CQ_EQ_NUM, /**< HW EventQueue Id num, mandatory for create */
    CQ_MODERATION, /**< Sets CQ moderations attributes*/
    CQ_FLAGS, /**< Sets CQ context flags */
    CQ_ATTR_MAX_CNT
};

/**
 * @brief enum cq_flags - set name for specific flags of CQ context (see PRM)
 *
 */
enum cq_flags {
    ATTR_CQ_NONE_FLAG = 0,
    ATTR_CQ_CQE_COLLAPSED_FLAG, /**< If set, all CQEs are written (collapsed) to
                                first CQ entry */
    ATTR_CQ_BREAK_MODERATION_EN_FLAG, /**< When set, solicited CQE (CQE.SE flag
                                      is enabled) breaks Completion Event
                                      Moderation. CQE causes immediate EQE
                                      generation.*/
    ATTR_CQ_OVERRUN_IGNORE_FLAG, /**< When set, overrun ignore is enabled.
                                 When set, updates of CQ consumer counter (poll
                                 for      completion) or Request completion
                                 notifications      (Arm CQ) DoorBells should not
                                 be      rung on that CQ */
    ATTR_CQ_PERIOD_MODE_FLAG, /**< 0: upon_event - cq_period timer restarts upon
                              event generation. 1: upon_cqe - cq_period timer
                              restarts upon completion generation */
    ATTR_CQ_MAX_CNT_FLAG
};

/**
 * @brief struct cq_moderation - Describes CQ Moderation attributes, (PRM,
 * sec.8.19.10, Table 171)
 *
 */
struct cq_moderation {
    uint32_t cq_period; /**< Event Generation moderation timer in 1 usec
                         granularity,  0 - disabled */
    uint32_t cq_max_cnt; /**<Event Generation Moderation counter, 0 - disabled */
};

/**
 * @brief struct cq_attr - CQ Atrributes for create and modify CQ
 *
 */
struct cq_attr {
    uint32_t cq_sz; /**< size of CQ in CQE numbers (64bytes each), should be power
                       of 2*/
    uint32_t eq_num; /**< CQ reports completion events to this Event Queue Id */
    cq_moderation moderation; /**< moderation attributes */
    std::bitset<ATTR_CQ_MAX_CNT_FLAG> flags; /**< CQ flags */
    std::bitset<CQ_ATTR_MAX_CNT> cq_attr_use; /**< OR'd mask of attribute types
                                 which should be applied and use */
};

#if !defined(__linux__)
#if !defined(MLX5_CQ_DB_REQ_NOT_SOL)
#define MLX5_CQ_DB_REQ_NOT_SOL 1 << 24
#define MLX5_CQ_DB_REQ_NOT 0 << 24
#endif

#if !defined(MLX5_CQ_DOORBELL)
#define MLX5_CQ_DOORBELL 0x20
#endif
#endif // !define(__linux__)

#if defined(CQE_SIZE)
#undef CQE_SIZE
#endif
#define CQE_SIZE 64

/**
 * @brief class cq - Handles CompletionQueue
 *
 */
class cq : public obj {
    friend class adapter;
    cq_attr m_user_attr;
    uar_t* m_uar;
    adapter* m_adapter;

    void* m_cq_buf;
    dcmd::umem* m_cq_buf_umem;

    uint32_t* m_db_rec;
    uint32_t* m_arm_db;
    dcmd::umem* m_db_rec_umem;

    size_t m_cqe_num; // Number of CQEs in CQ, must be power of 2
    uint32_t m_cq_buf_sz_bytes; // CQ size in bytes, must be power of 2
    uint32_t m_cq_buf_umem_id;
    uint32_t m_db_rec_umem_id;
    uint32_t m_cqn;
    uint32_t m_eqn;

    cq(adapter* ad, const cq_attr& attr);

    status create();
    status init(const uar_t* cq_uar);
    status allocate_cq_buf(void*& buf, size_t sz);
    status release_cq_buf(void* buf);
    status allocate_db_rec(uint32_t*& db_rec, size_t& sz);
    status release_db_rec(uint32_t* db_rec);

public:
    virtual ~cq();
    /**
     * @brief Returns virtual address of CQ buffer
     * @param [out] cq_buf_addr      CQ buffer address
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_cq_buf(void*& cq_buf_addr);
    /**
     * @brief Returns virtual address of CQ DoorBell record
     * @param [out] db_rec      DB record address to be stored to
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_dbrec(uint32_t*& db_rec);
    /**
     * @brief Returns virtual address of RQ UAR page
     * @param [out] uar_page      RQ UAR page address
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_uar_page(volatile void*& uar_page);
    /**
     * @brief Returns CQEs number
     * @param [out] cqe_num      CQEs number
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_cqe_num(uint32_t& cqe_num);
    /**
     * @brief Returns CQ Element size in bytes
     *
     * @retval Returns CQE size.
     */
    inline size_t get_cqe_sz() const
    {
        return CQE_SIZE;
    }
    /**
     * @brief Returns CQ buffer size in bytes
     *
     * @retval Returns buffer size.
     */
    inline size_t get_cq_buf_sz() const
    {
        return m_cq_buf_sz_bytes;
    }

    virtual status destroy();
};

enum rq_state {
    RQ_RST = 0x0, /**< RQ in reset state */
    RQ_RDY = 0x1, /**< RQ in ready state */
    RQ_ERR = 0x3 /**<  RQ in error state */
};

enum rq_mem_type {
    MEMORY_RQ_INLINE = 0, /**< packet to be inline in RQ WQE */
    MEMORY_RQ_RMP = 1,
    MEMORY_RQ_IBQ = 2
};

enum wq_type {
    WQ_LINKED_LIST = 0x0,
    WQ_CYCLIC = 0x1,
    LINKED_LIST_STRIDING_WQ = 0x2,
    CYCLIC_STRIDING_WQ = 0x3
};

enum rq_ts_format {
    RQ_TS_FREE_RUNNING = 0x0,
    RQ_TS_DEFAULT = 0x1, /**< Selected by the device */
    RQ_TS_REAL_TIME = 0x2
};

struct rq_attr {
    size_t buf_stride_sz;
    uint32_t buf_stride_num;
    uint32_t user_index;
    uint32_t cqn;
    size_t wqe_num; // Number of WQEs in RQ, must be power of 2
    size_t wqe_sz; // WQE size, i.e. number of DS (16B) in each RQ WQE, must be power of 2
    uint8_t ts_format;
    uint8_t ibq_scatter_offset;
};

enum {
    MLX5_MAX_SINGLE_STRIDE_LOG_NUM_BYTES = 13,
    MLX5_MIN_SINGLE_STRIDE_LOG_NUM_BYTES = 6,
    MLX5_MAX_SINGLE_WQE_LOG_NUM_STRIDES = 16,
    MLX5_MIN_SINGLE_WQE_LOG_NUM_STRIDES = 9,
};

class rq : public obj {
protected:
    rq_attr m_attr;
    rq_state m_state;

public:
    rq(dcmd::ctx* ctx, const rq_attr& attr);
    /**
     * @brief Changes state of RQ
     *
     * @param [in] new_state The requested new state
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status modify_state(rq_state new_state);
    virtual status get_hw_buff_stride_sz(size_t& buff_stride_sz);
    virtual status get_hw_buff_stride_num(size_t& buff_stride_num);
    virtual status get_cqn(uint32_t& cqn);
};

/**
 * @brief class basic_rq - A base class for striding_rq and regular_rq
 *
 */
class basic_rq : public rq {
protected:
    friend class adapter;
    uar_t* m_uar;
    const adapter* m_adapter;

    void* m_wq_buf;
    dcmd::umem* m_wq_buf_umem;

    uint32_t* m_db_rec;
    dcmd::umem* m_db_rec_umem;

    uint32_t m_wq_buf_sz_bytes;
    uint32_t m_wq_buf_umem_id;
    uint32_t m_db_rec_umem_id;
    rq_mem_type m_mem_type;

    basic_rq(const adapter* ad, const rq_attr& attr);
    status allocate_wq_buf(void*& buf, size_t sz);
    status allocate_db_rec(uint32_t*& db_rec, size_t& sz);
    status init(const uar_t* rq_uar);

    virtual status create() = 0;

public:
    /**
     * @brief Returns virtual address of RQ WQ buffer
     * @param [out] wq_buf_addr      RQ WQ buffer address
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_wq_buf(void*& wq_buf_addr);
    /**
     * @brief Returns virtual address of RQ DoorBell record
     * @param [out] db_rec      DB record address to be stored to
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_dbrec(uint32_t*& db_rec);
    /**
     * @brief Returns virtual address of RQ UAR page
     * @param [out] uar_page      RQ UAR page address
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_uar_page(volatile void*& uar_page);
    /**
     * @brief Returns RQ WQEs number
     * @param [out] wqe_num      RQ WQEs number
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_wqe_num(uint32_t& wqe_num);
    /**
     * @brief Returns RQ WQ stride size in bytes
     * @param [out] wq_stride_sz      RQ WQE stride size in bytes
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_wq_stride_sz(uint32_t& wq_stride_sz);

    inline size_t get_wq_buf_sz() const
    {
        return m_wq_buf_sz_bytes;
    }

    virtual status destroy();

    virtual ~basic_rq();
};

/**
 * @brief class striding_rq - Handles Striding ReceiveQueue
 *
 */
class striding_rq : public basic_rq {
    friend class adapter;
    striding_rq(const adapter* ad, const rq_attr& attr);

    virtual status create() override;

public:
    virtual ~striding_rq()
    {
    }
};

/**
 * @brief class regular_rq - Handles Regular ReceiveQueue
 *
 */
class regular_rq : public basic_rq {
    friend class adapter;
    regular_rq(const adapter* ad, const rq_attr& attr);

    virtual status create() override;

public:
    virtual ~regular_rq()
    {
    }
};

/**
 * @brief class ibq_rq - Handles IBQ ReceiveQueue
 *
 */
class ibq_rq : public rq {
    friend class adapter;
    adapter* m_adapter;

    dpcp_ibq_protocol m_protocol;
    uint32_t m_mkey;

    ibq_rq(adapter* ad, rq_attr& attr);

    status create();
    status init(dpcp_ibq_protocol protocol, uint32_t mkey);

public:
    virtual ~ibq_rq();
    virtual status destroy();

    /**
     * @brief Returns how ibq extract the sequence number from the packet
     * @param [out] protocol      how ibq extract the sequence number from the
     * packet
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_ibq_protocol(dpcp_ibq_protocol& protocol);

    status get_mkey(uint32_t& mkey);
};

/**
 * @brief Represent and handles TIR object
 *
 */
enum {
    TIR_ATTR_EXTEND = (1 << 0), /**< Special bit to extend flags field */
    TIR_ATTR_LRO = (1 << 1),
    TIR_ATTR_INLINE_RQN = (1 << 2),
    TIR_ATTR_TRANSPORT_DOMAIN = (1 << 3),
    TIR_ATTR_TLS = (1 << 4),
    TIR_ATTR_NVMEOTCP_ZERO_COPY = (1 << 5),
    TIR_ATTR_NVMEOTCP_CRC = (1 << 6),
};

class tir : public forwardable_obj {
public:
    struct attr {
        uint32_t flags;
        struct {
            uint32_t timeout_period_usecs : 16;
            uint32_t enable_mask : 4;
            uint32_t max_msg_sz : 8;
        } lro;
        uint32_t inline_rqn : 24;
        uint32_t transport_domain : 24;
        uint32_t tls_en : 1;
        struct {
            uint32_t zerocopy_en : 1;
            uint32_t crc_en : 1;
            uint32_t tag_buffer_table_id;
        } nvmeotcp;
    };

public:
    /**
     * @brief TIR Object constructor, object is initialized but not created yet
     *
     * @param [in]  ctx          Pointer to adapter context
     *
     */
    tir(dcmd::ctx* ctx);
    virtual ~tir();
    /**
     * @brief Create TIR object using requested properties
     *
     * @param [in]  tir_attr     Object attributies
     */
    status create(const tir::attr& tir_attr);
    /**
     * @brief Modify TIR object in HW
     */
    status modify(const tir::attr& tir_attr);
    /**
     * @brief Query TIR object in HW
     */
    status query(tir::attr& tir_attr);
    /**
     * @brief Get TIR Number
     */
    inline uint32_t get_tirn() const
    {
        return m_tirn;
    }
    /**
     * @brief Get forward type
     */
    int get_fwd_type() const;

private:
    struct attr m_attr;
    uint32_t m_tirn;
};

/**
 * @brief Represent and handles TIS object
 */
enum {
    TIS_ATTR_EXTEND = (1 << 0), /**< Special bit to extend flags field */
    TIS_ATTR_TRANSPORT_DOMAIN = (1 << 1),
    TIS_ATTR_TLS = (1 << 2),
    TIS_ATTR_PD = (1 << 3),
    TIS_ATTR_NVMEOTCP = (1 << 4)
};

class tis : public obj {
public:
    struct attr {
        uint32_t flags;
        uint32_t tls_en : 1;
        uint32_t nvmeotcp : 1;
        uint32_t transport_domain : 24;
        uint32_t pd : 24;
    };

public:
    /**
     * @brief TIS Object constructor, object is initialized but not created yet
     *
     * @param [in]  ctx           Pointer to adapter context
     *
     */
    tis(dcmd::ctx* ctx);
    virtual ~tis();
    /**
     * @brief Create TIS object using requested properties
     *
     * @param [in]  tis_attr     Object attributies
     */
    status create(const tis::attr& tis_attr);
    /**
     * @brief Query TIR object in HW
     */
    status query(tis::attr& tis_attr);
    /**
     * @brief Get TIS number object in HW
     *
     * @param [out]  tisn         The returned TIS number
     *
     * @retval Returns @ref dpcp::status with the status code
     */
    inline status get_tisn(uint32_t& tisn) const
    {
        if (m_tisn == 0) {
            return DPCP_ERR_INVALID_ID;
        }
        tisn = m_tisn;
        return DPCP_OK;
    }

private:
    struct attr m_attr;
    uint32_t m_tisn;
};

/**
 * @brief: Represent flow table types
 */
enum flow_table_type {
    FT_RX = 0x0, /**< Flow table from type receive */
    FT_TX = 0x1, /**< Flow table from type transmit */
    FT_END,
};

/**
 * @brief: Represent flow table operation modes
 */
enum flow_table_op_mod {
    FT_OP_MOD_NORMAL = 0x0, /**< Regular flow table */
};

/**
 * @brief: Represent flow table action when packet missed all rules.
 */
enum flow_table_miss_action {
    FT_MISS_ACTION_DEF = 0x0, /**< Default miss table action according to table type default:
                                   FT_RX - drop packet.
                                   FT_TX - forward packet to NIC Vport. */
    FT_MISS_ACTION_FWD = 0x1, /**< Forward to specific table identified by @ref table_miss */
};

/**
 * @brief: Represent flow table flags.
 */
enum flow_table_flags {
    FT_EN_REFORMAT = (1UL << 0), /**< If set, flow table supports Reformat action */
    FT_EN_DECAP = (1UL << 1), /**< If set, flow table supports DECAP actions */
};

/**
 * @brief: Represent flow table attributes.
 */
struct flow_table_attr {
    uint64_t flags; /**< Flow table flags define in @ref flow_table_flags */
    std::shared_ptr<flow_table> table_miss; /**< Valid when @ref FT_MISS_ACTION_FWD is set.
                                            Identify the next table in case of miss in the current
                                            table lookup. Table type of miss_table_id must be the
                                            same as table type of current table. */
    uint8_t log_size; /**< Log 2 of the table size (given in number of flows) */
    uint8_t level; /**< Location in table chaining hierarchy, only root table can be 0.*/
    flow_table_type type;
    flow_table_op_mod op_mod;
    flow_table_miss_action def_miss_action;
    uint16_t modify_field_select; /**< Not suppoerted */

    flow_table_attr()
        : flags(0)
        , log_size(0)
        , level(0)
        , op_mod(flow_table_op_mod::FT_OP_MOD_NORMAL)
        , def_miss_action(flow_table_miss_action::FT_MISS_ACTION_DEF)
        , modify_field_select(0)
    {
    }
};

/**
 * @brief: flow table class, Abstract class.
 *
 * Packet processing by the device requires classifying them into Flows.
 * Each Flow may have a different processing path and may lead to a different destination.
 * Packet classification is done using the Flow Table mechanism.
 *
 * It may require an hierarchal approach. For that purpose, a Flow Rule with a
 * Forward action may specify another Flow Table as the next processing entity for the
 * packet. The destination Flow Table must be of the same type. A packet matching this flow
 * will continue to search for another match in the destination Flow Table starting at the
 * first Flow Rule. For every Flow Table type there is a single root table.
 *
 * The match criteria of a Flow Rule is defined before setting the actual Flow Rule. A set of
 * consecutive Flow Rules in a certain Flow Table with the same match criteria is considered
 * a Flow Groups. this will be done by creating new Flow group using
 * @ref flow_table::add_flow_group. Each Flow Table may consist of more then one Flow Groups,
 * the size of all Flow Groups should not exceed the size of the Flow Table.
 *
 * After creating a Flow Group that define the match criteria, Flow Rules can be add by using
 * @ref flow_group::add_flow_rule to create Flow Rules that share the same match criteria
 * that is define by the Flow Group. The Flow Rule will define the match values, this will
 * identify the packets that related to this specific Flow Rule, and a Flow Action to perform
 * on the packet. Flow actions can be added to the flow rule by creating @ref flow_action
 * concrete classes by using @ref flow_action_generator, and passing them to the
 * flow rule attributes.
 *
 * Example (pseudo code):
 * flow_table = adapter->create_flow_table();
 * flow_group = flow_table->add_flow_group();
 * flow_rule = flow_group->add_flow_rule(match_params, flow_actions);
 *
 * All packet processing begin in the Root Flow Table, this flow table is created by the
 * operation system. To create Rules on the Root Flow Table, get flow table from the
 * adapter @ref adapter::get_root_flow_table. All other steps are the same as for the
 * device Flow Tables.
 *
 * Example (pseudo code):
 * flow_table = adapter->get_root_table();
 * flow_group = flow_table->add_flow_group();
 * flow_rule = flow_group->add_flow_rule(match_params, flow_actions);
 *
 * @note class flow_table is not thread-safe, instance of that class should not be accessed
 * from different threads unless thread-safety measures were taken by the application.
 */
class flow_table : public forwardable_obj, public std::enable_shared_from_this<flow_table> {
protected:
    flow_table_type m_type;
    bool m_is_initialized;
    std::unordered_set<std::shared_ptr<flow_group>> m_groups;

public:
    /**
     * @brief Flow table constructor - This is abstract class, to create new table need to call
     *        @ref adapter::create_flow_table.
     *
     * @param [in] ctx: dcmd context.
     * @param [in] type: flow table type.
     *
     * @note: The constructor will not create the flow table object
     *        in the HW, need to call to @ref flow_table::create to allocate it on the HW.
     */
    flow_table(dcmd::ctx* ctx, flow_table_type type);
    /**
     * @brief Copy constructor of flow group.
     *
     * @note: Copy of flow table object is not allowed.
     */
    flow_table(const flow_table& table) = delete;
    /**
     * @brief Assignment operator of flow group.
     *
     * @note: Copy of flow table object is not allowed.
     */
    flow_table& operator=(const flow_table& table) = delete;
    /**
     * @brief Creates flow table object in HW.
     *
     * @retval Returns @ref dpcp::status with the status code.
     */
    virtual status create() = 0;
    /**
     * @brief Query flow table object.
     *
     * @retval Returns @ref dpcp::status with the status code.
     */
    virtual status query(flow_table_attr& attr) = 0;
    /**
     * @brief Get flow table level.
     *
     * @param [out] table_level: flow table level.
     *
     * @retval Returns @ref dpcp::status with the status code.
     */
    virtual status get_table_level(uint8_t& table_level) const = 0;
    /**
     * @brief Get flow table type.
     *
     * @param [out] table_type: flow table type.
     *
     * @retval Returns @ref dpcp::status with the status code.
     */
    status get_table_type(flow_table_type& table_type) const;
    /**
     * @brief Add flow group to flow table.
     *
     * @param [in] params: Flow group parameters.
     * @param [out] group: Pointer to added flow group.
     *
     * @retval Returns @ref dpcp::status with the status code.
     */
    virtual status add_flow_group(const flow_group_attr& attr,
                                  std::weak_ptr<flow_group>& group) = 0;
    /**
     * @brief Remove flow group from flow table.
     *
     * @param [in/out] group: Pointer to flow group.
     *
     * @retval Returns @ref dpcp::status with the status code.
     */
    status remove_flow_group(std::weak_ptr<flow_group>& group);
    /**
     * @brief Get forward type
     */
    int get_fwd_type() const;
    /**
     * @brief Flow table destructor
     */
    virtual ~flow_table() = default;

protected:
    template <class FG>
    status create_flow_group(const flow_group_attr& attr, std::weak_ptr<flow_group>& group);
    status get_flow_table_status() const;
};

/**
 * @brief: Represent flex parser sample field.
 */
struct parser_sample_field {
    uint32_t val; /**< Sample value to match/mask */
    uint32_t id; /**< Sample id received by @ref get_sample_ids at class @ref parser_graph_node */
};

/**
 * @brief: Represent layer 2 match params.
 */
struct match_params_lyr_2 {
    uint8_t src_mac[8]; /**< 6 bytes(48 bits) mac address + 2 bytes(16 bits) alignment to 64 bits */
    uint8_t dst_mac[8]; /**< 6 bytes(48 bits) mac address + 2 bytes(16 bits) alignment to 64 bits */
    uint16_t ethertype;
    uint16_t first_vlan_id;
};

/**
 * @brief: Represent layer 3 match params.
 */
struct match_params_lyr_3 {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t ip_protocol;
    uint8_t ip_version : 4;
    // TODO: Add ipv6 support
};

/**
 * @brief: Represent layer 4 types.
 */
enum match_params_lyr_4_type {
    NONE = 0x0,
    TCP,
    UDP,
};

/**
 * @brief: Represent layer 4 match params abstract.
 */
struct match_params_lyr_4 {
    match_params_lyr_4_type type;
    uint16_t src_port;
    uint16_t dst_port;
};

/**
 * @brief: Represent match params.
 */
struct match_params_ex {
    match_params_lyr_2 match_lyr2;
    match_params_lyr_3 match_lyr3;
    match_params_lyr_4 match_lyr4;
    std::vector<parser_sample_field> match_parser_sample_field_vec; /**< Samples received by
                                                                         @ref parser_graph_node. */
    uint32_t match_metadata_reg_c_0;

    match_params_ex()
        : match_metadata_reg_c_0(0)
    {
        memset(&match_lyr2, 0, sizeof(match_lyr2));
        memset(&match_lyr3, 0, sizeof(match_lyr3));
        memset(&match_lyr4, 0, sizeof(match_lyr4));
    }
};

/**
 * @brief: Represent match criteria enable flags.
 */
enum flow_group_match_criteria_enable {
    FG_MATCH_OUTER_HDR = 0x1, /**< Enable match on outer header fields */
    FG_MATCH_METADATA_REG_C_0 = 0x8, /**< Enable match on metadata register 0 */
    FG_MATCH_PARSER_FIELDS = 0x20, /**< Enable match on samples received by
                                        @ref parser_graph_node.*/
};

/**
 * @brief: Represent match params.
 */
// TODO: On root table we do not need all params.
struct flow_group_attr {
    uint32_t start_flow_index; /**< The first flow rule included in the group.*/
    uint32_t end_flow_index; /**< The last flow rule included in the group.*/
    uint8_t match_criteria_enable; /**< Bit-mask representing which of the headers and parameters in
                                        match_criteria are used in defining the Flow,
                                        @ref flow_group_match_criteria_enable. */
    match_params_ex match_criteria; /**< The match parameters defining all the flows belonging
                                         to the group. */

    flow_group_attr()
        : start_flow_index(0)
        , end_flow_index(0)
        , match_criteria_enable(0)
    {
    }
};

/**
 * @brief: Flow Group
 *
 * Represent Flow Group associated with @ref class flow_table.
 * A Flow Group define a set of Flow Rules that share the same match criteria
 * which set on which fields and masks the packet should be matched on.
 * see @ref flow_table for more information.
 *
 * @note class flow_group is not thread-safe, instance of that class should not be accessed
 * from different threads unless thread-safety measures were taken by the application.
 */
class flow_group : public obj, public std::enable_shared_from_this<flow_group> {
protected:
    flow_group_attr m_attr;
    std::weak_ptr<const flow_table> m_table;
    bool m_is_initialized;
    std::unordered_set<std::shared_ptr<flow_rule_ex>> m_rules;
    std::shared_ptr<flow_matcher> m_matcher;

public:
    /**
     * @brief Flow group constructor
     *
     * @param [in] ctx: dcmd context
     * @param [in] attr: flow group attributes
     * @param [in] table: flow table
     *
     * @note: Groups can be constructed only by @ref flow_table::add_flow_group.
     *        The constructor will not allocate the object in the HW, should call @ref
     *        flow_group::create().
     */
    flow_group(dcmd::ctx* ctx, const flow_group_attr& attr, std::weak_ptr<const flow_table> table);
    /**
     * @brief Copy constructor of flow group.
     *
     * @note: Copy of flow group object is not allowed.
     */
    flow_group(const flow_group& group) = delete;
    /**
     * @brief Assignment operator of flow group.
     *
     * @note: Copy of flow group object is not allowed.
     */
    flow_group& operator=(const flow_group&) = delete;
    /**
     * @brief Creates flow group object in HW
     *
     * @note the flow group configurations will not be applied in the HW until
     *       the create() will be called.
     *
     * @retval Returns @ref dpcp::status with the status code.
     */
    virtual status create() = 0;
    /**
     * @brief Add flow rule to group.
     *
     * @param [in] attr: flow rule attr.
     * @param [out] rule: flow rule object.
     *
     * @retval Returns @ref dpcp::status with the status code.
     */
    virtual status add_flow_rule(const flow_rule_attr_ex& attr,
                                 std::weak_ptr<flow_rule_ex>& rule) = 0;
    /**
     * @brief Remove flow rule from group.
     *
     * @param [in/out] rule: flow rule.
     *
     * @retval Returns @ref dpcp::status with the status code.
     */
    status remove_flow_rule(std::weak_ptr<flow_rule_ex>& rule);
    /**
     * @brief Get group match criteria.
     *
     * @param [out] match: match params.
     *
     * @retval Returns @ref dpcp::status with the status code
     */
    status get_match_criteria(match_params_ex& match) const;
    /**
     * @brief Destructor of flow group.
     */
    virtual ~flow_group() = default;

protected:
    // Help functions
    template <class FR>
    status create_flow_rule_ex(const flow_rule_attr_ex& attr, std::weak_ptr<flow_rule_ex>& rule);
};

enum flow_action_reformat_anchor {
    MAC_START = 0x1,
    IP_START = 0x7,
    TCP_UDP_START = 0x9,
};

/**
 * @brief: Flow Action modify supported fields.
 */
enum flow_action_modify_field {
    OUT_SMAC_47_16 = 0x1,
    OUT_SMAC_15_0 = 0x2,
    OUT_ETHERTYPE = 0x3,
    OUT_DMAC_47_16 = 0x4,
    OUT_DMAC_15_0 = 0x5,
    OUT_IP_DSCP = 0x6,
    OUT_TCP_FLAGS = 0x7,
    OUT_TCP_SPORT = 0x8,
    OUT_TCP_DPORT = 0x9,
    OUT_IP_TTL = 0xa,
    OUT_UDP_SPORT = 0xb,
    OUT_UDP_DPORT = 0xc,
    METADATA_REG_C_0 = 0x51,
    METADATA_REG_C_1 = 0x52,
};

/**
 * @brief: Flow Action modify type.
 */
enum flow_action_modify_type {
    SET = 0x1, /**< Write a specific data value a field */
    COPY = 0x3, /**< Copy a specify field to another one in a packet/metadata register */
};

/**
 * @brief: Flow Action reformat type.
 */
enum flow_action_reformat_type {
    INSERT_HDR = 0xf,
};

/**
 * @brief: Flow action reformat insert attributes
 */
struct flow_action_reformat_insert_attr {
    flow_action_reformat_type type; /**< Flow action reformat type, must be set to
                                         @ref flow_action_reformat_type::INSERT_HDR
                                         Note: this field should always be first.  */
    flow_action_reformat_anchor start_hdr; /**< Indicates the header used to reference the location
                                                of the inserted header */
    uint8_t offset; /**< Indicates the offset of the inserted header from the reference
                         point defined in @ref start_hdr, given in Bytes */
    uint16_t data_len : 10; /**< Data length to insert */
    void* data; /**< Data should hold the header to insert */
};

/**
 * @brief: Flow action reformat attributes.
 */
union flow_action_reformat_attr {
    flow_action_reformat_type type; /**< Flow action reformat type (insert, remove ...). */
    flow_action_reformat_insert_attr insert; /**< list of modify actions to perform */
};

/**
 * @brief: Flow action modify from type set attributes.
 */
struct flow_action_modify_set_attr {
    flow_action_modify_type type; /**< Flow action modify type, must be set to
                                       @ref flow_action_modify_type::SET
                                       Note: this field should always be first.*/
    flow_action_modify_field field; /**< Field to be modified */
    uint8_t offset : 5; /**< The offset inside the field */
    uint8_t length : 5; /**< Number of bits to be written starting from offset. 0
                                means length of 32 bits. */
    uint32_t data; /**< The data to be written on the specific field.
                         Data must be allocated starting from bit 0. */
};

/**
 * @brief: Flow action modify from type copy attributes.
 */
struct flow_action_modify_copy_attr {
    flow_action_modify_type type; /**< Flow action modify type, must be set to
                                       @ref flow_action_modify_type::COPY
                                       Note: this field should always be first.*/
    flow_action_modify_field src_field; /**< The source field of packet where data is copied from.
                                             The supported copy operations reported in Header
                                             Modify Capabilities per Flow Table that uses the
                                             modify action. */
    uint8_t src_offset : 5; /**< The start offset in the source field. */
    uint8_t length : 5; /**< Number of bits to be copied starting from offset, 0 means length of 32
                           bits. */
    flow_action_modify_field
        dst_field; /**< The destination field of packet where data is copied to.
                        The supported copy operations reported in Header
                        Modify Capabilities per Flow Table that uses the modify action. */
    uint8_t dst_offset : 5; /**< The start offset in the destination field. */
};

/**
 * @brief: Union represent flow_action_modify attributes by type.
 *         To support new modify types (copy, add) please add attributes
 *         structure to the union.
 *
 * @note: All modify types attributes bust have @flow_action_modify_type field first.
 */
union flow_action_modify_type_attr {
    flow_action_modify_type type; /**< Flow action modify type (set, add, copy) */
    flow_action_modify_set_attr set; /**< Flow action modify from type set attributes */
    flow_action_modify_copy_attr copy; /**< Flow action modify from type copy attributes */
};

/**
 * @brief: Flow action modify attributes.
 */
struct flow_action_modify_attr {
    flow_table_type table_type; /**< Flow table type that the action will be applied on. */
    std::vector<flow_action_modify_type_attr> actions; /**< list of modify actions to perform */
};

/**
 * @brief: Flow Action generator - see @ref flow_table for more information.
 *
 * @note flow_action instances created by this class are not thread safe unless specified,
 * Instance of that class should not be accessed from different threads unless thread-safety
 * measures were taken by the application.
 */
class flow_action_generator {
    friend class adapter;

private:
    dcmd::ctx* m_ctx;
    const adapter_hca_capabilities* m_caps;

public:
    /**
     * @brief Create flow action tag, each packet matched on the flow rule associated
     *        With this flow action will be marked with flow tag id that will be accessible
     *        in the completion queue element.
     *
     * @param [in] id: Flow tag id.
     *
     * @retval flow_action action pointer or nullptr.
     *
     * @note: Thread-safe
     */
    std::shared_ptr<flow_action> create_tag(uint32_t id);
    /**
     * @brief Create flow action forward, will forward the packets upon match to destination list.
     *        The destinations ca be from different type (tir, flow_table)
     *
     * @param [in] dests: Destination objects, currently support @ref tir, @ref flow_table.
     *
     * @retval flow_action action pointer or nullptr.
     */
    std::shared_ptr<flow_action> create_fwd(std::vector<forwardable_obj*> dests);
    /**
     * @brief Create flow action reformat, allow to change the packet header.
     *
     * @param [in] attr: Reformat action attributes.
     *
     * @retval flow_action action pointer or nullptr.
     */
    std::shared_ptr<flow_action> create_reformat(flow_action_reformat_attr& attr);
    /**
     * @brief Create flow action modify, allow to modify the packet header fields.
     *
     * @param [in] attr: Reformat action attributes.
     *
     * @retval flow_action action pointer or nullptr.
     */
    std::shared_ptr<flow_action> create_modify(flow_action_modify_attr& attr);
    /**
     * @brief Create flow action reparse, allow to trigger HW packet reparse.
     *
     * @retval flow_action action pointer or nullptr.
     */
    std::shared_ptr<flow_action> create_reparse();

private:
    // Should be created only by @ref class adapter
    flow_action_generator(dcmd::ctx* ctx, const adapter_hca_capabilities* caps);
};

/**
 * @brief: flow_rule_ex attributes.
 */
struct flow_rule_attr_ex {
    uint16_t priority; /*< flow rule priority */
    match_params_ex match_value; /*< flow rule match value, should be same fields as the masks
                                     provided to flow_group. */
    uint32_t flow_index; /*< The location of the rule on the flow table,
                             index 0 will matched first. */
    std::vector<std::shared_ptr<flow_action>> actions; /* Flow actions to perform on the packet
                                                          when rule matched */

    flow_rule_attr_ex()
        : priority(0)
        , flow_index(0)
    {
    }
};

/**
 * @brief: Flow Rule extended see @ref flow_table for more information.
 *
 * @note class flow_rule_ex is not thread-safe, instance of that class should not be accessed
 * from different threads unless thread-safety measures were taken by the application.
 */
class flow_rule_ex : public obj {
    typedef unordered_map<std::type_index, std::shared_ptr<flow_action>> action_map_t;

protected:
    match_params_ex m_match_value;
    bool m_is_initialized;
    std::weak_ptr<const flow_table> m_table;
    std::weak_ptr<const flow_group> m_group;
    bool m_is_valid_actions;
    action_map_t m_actions; /*< unordered_map, key is the object type,
                                value is shared_ptr to obj */
    std::shared_ptr<const flow_matcher> m_matcher;

public:
    /**
     * @brief flow rule extended constructor.
     */
    flow_rule_ex(dcmd::ctx* ctx, const flow_rule_attr_ex& attr,
                 std::weak_ptr<const flow_table> table, std::weak_ptr<const flow_group> group,
                 std::shared_ptr<const flow_matcher> matcher);
    /**
     * @brief Copy constructor of flow rule.
     *
     * @note: Copy of flow rule object is not allowed.
     */
    flow_rule_ex(const flow_rule_ex& fr) = delete;
    /**
     * @brief Assignment operator of flow rule.
     *
     * @note: Copy of flow rule object is not allowed.
     */
    const flow_rule_ex& operator=(const flow_rule_ex& fr) = delete;
    /**
     * @brief Get flow_rule match values
     *
     * @param [out] match_val: Flow Rule Match Value
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_match_value(match_params_ex& match_val);
    /**
     * @brief Create flow rule HW object.
     *
     * @note: only after create is called the flow rule settings will actually configured
     *        on the HW
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status create() = 0;
    virtual ~flow_rule_ex() = default;

private:
    // Help functions
    bool verify_flow_actions(const std::vector<std::shared_ptr<flow_action>>& actions);
};

struct match_params {
    uint8_t dst_mac[8]; // 6 bytes + 2 (EOS+alignment)
    uint16_t ethertype;
    uint16_t vlan_id; // 12 bits
    uint32_t dst_ip; // deprecated
    uint32_t src_ip; // deprecated
    uint16_t dst_port;
    uint16_t src_port;
    uint8_t protocol;
    uint8_t ip_version; // 4 bits
    union {
        uint32_t ipv4;
        uint8_t ipv6[16];
    } dst;
    union {
        uint32_t ipv4;
        uint8_t ipv6[16];
    } src;
};

typedef std::vector<tir*> dst_tir_vec;
/**
 * @brief class flow_rule - Represent receive flow rule
 *
 * Receive Flow Rules: control which traffic should be accepted by the adapter
 * and dispatch the incoming traffic to the selected transport instances and
 * eventually receive queues
 */
class flow_rule : public obj {
    friend class adapter;
    match_params m_mask;
    match_params m_value;
    dst_tir_vec m_dst_tir;
    dcmd::flow* m_flow;
    uint32_t m_flow_id;
    uint16_t m_priority;
    bool m_changed;

public:
    flow_rule(dcmd::ctx* dcmd_ctx, uint16_t priority, match_params& match_criteria);
    /**
     * @brief Set flow_rule match values
     *
     * @param [in]  match_val       Flow Rule Match Value
     *
     * @retval Returns DPCP_OK on success.
     */
    status set_match_value(match_params& match_val);
    /**
     * @brief Get flow_rule match values
     *
     * @param [out]  match_val       Flow Rule Match Value
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_match_value(match_params& match_val);
    /**
     * @brief Set flow_rule match values
     * It receives memory region allocated by user.
     *
     * @param [in]  match_val       Flow Rule Match Value
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_priority(uint16_t& priority);
    /**
     * @brief Set flow ID (flow action tag) of the flow rule
     *
     * @param [in]  flow_id       Flow ID for this rule
     *
     * @retval Returns DPCP_OK on success.
     */
    status set_flow_id(uint32_t flow_id);
    /**
     * @brief Obtain flow ID (flow action tag) of the flow rule
     *
     * @param [in]  flow_id       Flow ID for this rule in range [0-0xFFFFF]
     *                            0 - Flow Id for this rule will be disabled
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_flow_id(uint32_t& flow_id);
    /**
     * @brief Add DPCP tir to flow rule destination list.
     *
     * @param [in]  dst_tir       Pointer to DPCP tir to add to this flow rule
     *
     * @retval Returns DPCP_OK on success.
     */
    status add_dest_tir(tir* dst_tir);
    /**
     * @brief Remove DPCP tir from flow rule destination list
     *
     * @param [in]  dst_tir       Pointer to DPCP tir to remove from this flow
     * rule
     *
     * @retval Returns DPCP_OK on success.
     */
    status remove_dest_tir(const tir* dst_tir);
    /**
     * @brief Returns numbers of tirs assotiated with flow rule.
     *
     * @param [in]  num_tirs      Pointer to numbers of DPCP tirs in flow rule
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_num_tirs(uint32_t& num_tirs);
    /**
     * @brief Returns DPCP tir per index in the range 0..num_tirs-1.
     *
     * @param [in]  index         DPCP tir index
     * @param [out] tr            Pointer to TIR object on success
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_tir(uint32_t index, tir*& tr);
    /**
     * @brief Apply flow settings that were configured for this flow rule.
     * Notice: only after apply is called flow settings will actually configured
     *on the HW.
     *
     * If flow rule is changed after the last apply call there rule state returned
     *by rule get methods will reflect only rule logical state and not the actual
     *HW state, in order to get rule HW state make sure to read it's state (by
     *rule get nethods) after apply call but before any set calls. Notice: By
     *default flow rule masks are zero so if no filters were applied, flow rule
     *action will be always miss for all the traffic. By default rule priority if
     *0.
     *
     * If apply is called and destination list is 0, apply will fail.
     *
     *
     * @retval Returns DPCP_OK on success.
     */
    status apply_settings();
    /**
     * @brief Revoke flow settings that were configured for this flow rule and
     * applied via apply. If apply was not called revoke_settings returns with
     * failure. call to m_flow->destroy_flow(m_dcmd_flow);
     *
     * @retval Returns DPCP_OK on success.
     */
    status revoke_settings();
    virtual ~flow_rule();
};

enum sq_state {
    SQ_RST = 0x0, /**< RQ in reset state */
    SQ_RDY = 0x1, /**< RQ in ready state */
    SQ_ERR = 0x3 /**<  RQ in error state */
};

/**
 * @brief Packet Pacing attributes
 *
 */
typedef struct qos_packet_pacing_s {
    uint32_t sustained_rate; /**< packet pacing sustained rate */
    uint32_t burst_sz; /**< burst size in packets */
    uint16_t packet_sz; /**< typical packet size */
} qos_packet_pacing;

/**
 * @brief Dek attributes
 *
 */
struct dek_attr {
    void* key_blob; /**< key buffer layout, depended on the key type:
                         plaintext tls key - key
                         plaintext ipsec key - key
                         plaintext aes_xts key - key1 + key2 + keytag(optional) */
    uint32_t key_blob_size; /**< size of key blob in bytes */
    uint32_t key_size; /**< size of key in bytes */
    uint32_t pd_id; /**< Protection domain id */
    uint64_t opaque; /**< Plaintext metadata to describe the key */
};

/**
 * @brief Represent and handles DEK object, Abstract class
 */
class dek : public obj {
public:
    /**
     * @brief DEK Object constructor, object is initialized but not created yet
     *
     * @param [in]  ctx           Pointer to adapter context
     *
     */
    explicit dek(dcmd::ctx* ctx);
    virtual ~dek() = default;

    /**
     * @brief Create DEK object using requested properties
     *
     * @param [in] dek_attr Object attributies
     */
    status create(const dek_attr& attr);

    /**
     * @brief Modify DEK object in HW
     */
    status modify(const dek_attr& attr);

    /**
     * @brief Query DEK object in HW
     */
    status query(dek_attr& attr);

    /**
     * @brief Get key ID object in HW
     *
     * @note: Valid only when the @ref dek::create call
     * completed successfully with return status of @ref status::DPCP_OK
     *
     * @retval Returns the ID of the key in HW
     */
    inline uint32_t get_key_id() const
    {
        return m_key_id;
    }

private:
    uint32_t m_key_id;

protected:
    struct key_params {
        uint8_t type;
        uint8_t size;
        bool has_keytag;
        uint32_t offset;
    };

private:
    virtual status get_key_params(uint32_t key_blob_size, uint32_t key_size,
                                  dek::key_params& params) const;
    virtual uint8_t get_key_type() const = 0;

private:
    static status verify_attr(const dek_attr& attr);
    static uint32_t key_size_flag_to_bytes_size(uint8_t size_flag);
};

/**
 * @brief Represent and handles TLS DEK object
 */
class tls_dek : public dek {
public:
    /**
     * @brief TLS DEK Object constructor, object is initialized but not created yet
     *
     * @param [in]  ctx           Pointer to adapter context
     *
     */
    explicit tls_dek(dcmd::ctx* ctx);
    virtual ~tls_dek() = default;

private:
    virtual uint8_t get_key_type() const override;
};

/**
 * @brief Represent and handles IPSEC DEK object
 */
class ipsec_dek : public dek {
public:
    /**
     * @brief IPSEC DEK Object constructor, object is initialized but not created yet
     *
     * @param [in]  ctx           Pointer to adapter context
     *
     */
    explicit ipsec_dek(dcmd::ctx* ctx);
    virtual ~ipsec_dek() = default;

private:
    virtual uint8_t get_key_type() const override;
};

/**
 * @brief Represent and handles AES_XTS DEK object
 */
class aes_xts_dek : public dek {
public:
    /**
     * @brief AES_XTS DEK Object constructor, object is initialized but not created yet
     *
     * @param [in]  ctx           Pointer to adapter context
     *
     */
    explicit aes_xts_dek(dcmd::ctx* ctx);
    virtual ~aes_xts_dek() = default;

private:
    virtual status get_key_params(uint32_t key_blob_size, uint32_t key_size,
                                  dek::key_params& params) const override;
    virtual uint8_t get_key_type() const override;

private:
    static constexpr uint32_t key_size_to_blob_size(uint32_t key_size, bool has_keytag)
    {
        return key_size * 2 + has_keytag * 8;
    }
};

/*
 * @breif Represents suppoted header fields that can be used by specific operation
 *        e.g @ref class flow_action_modify, which fields can be modified.
 */
struct flow_table_fields_capabilities {
    bool outer_ethertype; /**< When set, match on Ethernet type supported */
    bool outer_udp_dport; /**< When set, match on UDP destination port supported */
    bool prog_sample_field; /**< When set, match on Flex programmable parser fields supported */
    bool metadata_reg_c_0; /**< When set, match on metadata reg_c_0 supported */
    bool metadata_reg_c_1; /**< When set, match on metadata reg_c_1 supported */
};

/*
 * @breif Flow Action modify capabilities @ref class flow_action_modify
 */
struct modify_flow_action_capabilities {
    uint8_t max_obj_log_num; /**< Total number of Flow Action modify user can create */
    uint32_t max_obj_in_flow_rule; /** Total number of Flow Action modify can be applied to single
                                       Flow Rule */
    uint8_t log_max_num_header_modify_argument; /**< Log (base 2) of the maximum number of supported
                                                   Header Modify Argument Object objects. */
    uint8_t log_header_modify_argument_granularity; /**< Log (base 2) of the minimum allocation
                                                       granularity of Header Modify Argument Object,
                                                       given in 64[B] units. */
    uint8_t log_header_modify_argument_max_alloc; /**< Log (base 2) of the maximum allocation
                                                     granularity of Header Modify Argument Object,
                                                     given in 64[B] units. */
    flow_table_fields_capabilities set_fields_support; /**< Supported fields for Flow Action
                                                            modify from type Set. */
    flow_table_fields_capabilities copy_fields_support; /**< Supported fields for Flow Action
                                                             modify from type Copy. */
    // flow_table_fields_support add_fields_support;
};

/*
 * @breif Flow Action reformat capabilities @ref class flow_action_reformat
 */
struct reformat_flow_action_capabilities {
    uint8_t max_size_reformat_insert_buff; /**< Maximum buffer size in reformat insert action */
    uint8_t max_reformat_insert_offset; /**< Maximum offset from anchor in reformat insert action */
    uint8_t max_log_num_of_packet_reformat; /**< Maximum number of size in reformat insert action */
};

/*
 * @breif Flow Table capabilities that can be different for each type (e.g receive, transmit)
 */
struct flow_table_type_capabilities {
    bool is_flow_table_supported;
    bool is_flow_action_tag_supported;
    bool is_flow_action_modify_supported;
    bool is_flow_action_reformat_supported;
    bool is_flow_action_reformat_from_type_insert_supported; /**< flow action reformat from type
                                                                  insert header can be supported
                                                                  even if reformat is not
                                                                  supported */
    bool
        is_flow_action_non_tunnel_reformat_and_fwd_to_flow_table; /**< Is packet non-tunnel reformat
                                                                       types @ref
                                                                       flow_action_reformat_type::INSERT_HDR
                                                                       or @ref
                                                                       flow_action_reformat_type::REMOVE_HDR
                                                                       with Forward to Table action
                                                                       in the same rule supported.
                                                                   */
    bool is_flow_action_reformat_and_modify_supported; /**< is reformat and modify supported
                                                            together for the same Flow Rule */
    bool is_flow_action_reformat_and_fwd_to_flow_table; /**< Is reformat and forward to flow table
                                                             supported
                                                             together for the same Flow Rule */
    uint32_t max_steering_depth; /**< Indicates a limit to the longest path of packet traversing
                                 through the NIC receive steering table. The field is given in
                                 device specific units*/
    uint8_t max_log_size_flow_table; /**< Maximum log size of flow table in Flow Rules */
    uint32_t max_flow_table_level;
    uint8_t max_log_num_of_flow_table; /**< Maximum log number of Flow Table that can be created */
    uint8_t max_log_num_of_flow_rule; /**< Maximum log number of Flow Rules that can be created */
    bool is_flow_action_reparse_supported; /**< When set, this Flow Table type supports Flow Table
                                                Entries with reparse indication or Rule Table
                                                Context(RTC) with always reparse mode.
                                            */
    modify_flow_action_capabilities modify_flow_action_caps; /**< Flow Action modify capabilities */
    flow_table_fields_capabilities ft_field_support; /**< Supported fields for the table. */
};

/*
 * @breif Flow Table capabilities that are common for all types
 */
struct flow_table_capabilities {
    reformat_flow_action_capabilities reformat_flow_action_caps;

    flow_table_type_capabilities receive;
};

/*
 * @breif NVMe/TCP capabilities
 */
struct nvmeotcp_capabilities {
    bool enabled; /**< If set, NVMEoTCP offload is supported. */
    bool zerocopy; /**< If set, zero copy is supported. */
    bool crc_rx; /**< If set, CRC32 for received data is supported. */
    bool crc_tx; /**< If set, CRC32 for transmitted data is supported. */
    uint8_t version; /**< Bitmask indicates the supported NVMEoTCP vestions as reported in ICresp.
                        Bit0: version_0 */
    uint8_t log_max_nvmeotcp_tag_buffer_table; /**< Log(base 2) of the maximum support tag buffer
                                                  tables */
    uint8_t log_max_nvmeotcp_tag_buffer_size; /**< Log(base 2) of the maximum support tag buffer
                                                 table size in granularity of 16B */
};

/*
 * @breif Represents HCA capabilities
 *
 * @note: The type of the fields shouldn't be changed.
 */
typedef struct adapter_hca_capabilities {
    uint32_t device_frequency_khz; /**< Internal device frequency given in KHz.
                                      Valid only if non-zero. */
    bool tls_tx; /**< If set, TLS offload for transmitted traffic is supported */
    bool tls_rx; /**< If set, TLS offload for received traffic is supported */
    bool tls_1_2_aes_gcm_128; /**< If set, aes_gcm cipher with TLS 1.2 and 128 bit
                                 key is supported */
    bool tls_1_2_aes_gcm_256; /**< If set, aes_gcm cipher with TLS 1.2 and 256 bit
                                 key is supported */
    bool general_object_types_encryption_key; /**< If set, creation of encryption
                                               * keys is supported
                                               */
    bool synchronize_dek; /**< If set, SYNC_CRYPTO command is required before modifying
                             and reusing a previously used DEK. */
    uint8_t log_max_num_deks; /**< Log (base 2) of maximal number of DEKs supported
                                 [Internal] This replaces the one in HCA_CAP. */
    uint8_t log_max_dek; /**< Log (base 2) of maximum DEK Objects that are
                            supported, 0 means not supported */
    bool crypto_enable; /**< no prm description. if set, crypto capabilites are supported */
    bool aes_xts_multi_block_le_tweak;
    bool aes_xts_tweak_inc_64; /**< Tweak is limited to 0-2^64-2. To have the tweak increment
                                  by (1<<64) between blocks */
    bool aes_xts_single_block_le_tweak; /**<  indicates multi stx block per mkey is not supported,
                                         and trying to encrypt/decrypt multiple blocks in this case
                                         will result in non-complient output blocks. */
    bool aes_xts_multi_block_be_tweak; /**< the tweak must be supplied as big-endian */
    bool aes_xts_tweak_inc_shift; /**< tweak incremented according to tweak_increment_shift */
    uint8_t sq_ts_format; /**< Indicates the supported ts_format in SQ Context.
                             0x0: FREE_RUNNING_TS
                             0x1: REAL_TIME_TS
                             0x2: FREE_RUNNING_AND_REAL_TIME_TS - both
                             free running real time timestamps are supported.*/
    uint8_t rq_ts_format; /**< Indicates the supported ts_format in RQ Context.
                             0x0: FREE_RUNNING_TS
                             0x1: REAL_TIME_TS
                             0x2: FREE_RUNNING_AND_REAL_TIME_TS - both
                             free running real time timestamps are supported.*/
    bool lro_cap; /**< indicates LRO support */
    bool lro_psh_flag; /**< indicate LRO support for segments with PSH flag */
    bool lro_time_stamp; /**< indicate LRO support for segments with TCP timestamp option */
    uint8_t lro_max_msg_sz_mode; /**< indicate reports which LRO max message size mode
                                    the device supports.
                                    0x0 - TCP header + TCP payload
                                    0x1 - L2 + L3 + TCP header + TCP payload */
    uint16_t lro_min_mss_size; /**< the minimal size of TCP segment required for coalescing */
    uint8_t lro_timer_supported_periods[4]; /**< Array of supported LRO timer periods in
                                               microseconds. */
    bool ibq; /** <indicates Inline Buffer Queue capability (IBQ) */
    uint64_t ibq_wire_protocol; /**< List of supported protocols for IBQ @ref dpcp_ibq_protocol */
    uint16_t ibq_max_scatter_offset; /**< IBQ maximum supported scatter offset */
    bool general_object_types_parse_graph_node; /**< If set, creation of programmable parse graph
                                                   node is supported. */
    uint32_t parse_graph_node_in; /**< Bitmask for the supported protocol headers that programmable
                                     parse graph may use as existing nodes in the parse graph and
                                     define an input arcs. See @ref
                                     parse_graph_arc_node_index ENUM. */
    uint16_t
        parse_graph_header_length_mode; /**< Bitmask indicating which modes are supported
                                             for @ref parser_graph_node_attr.header_length_mode
                                             in parser graph node object. Set bit indicates it is
                                             supported. See @ref parse_graph_node_len_mode ENUM. */
    uint16_t
        parse_graph_flow_match_sample_offset_mode; /**< Bitmask indicating which modes are supported
                                                        for flow_match_sample_offset_mode in parser
                                                        graph node object. Set bit indicates it is
                                                        supported. See @ref
                                                        parse_graph_flow_match_sample_offset_mode
                                                        ENUM. */
    uint8_t max_num_parse_graph_arc_in; /**< Maximal number of input arcs supported for a single
                                           parser graph node object. */
    uint8_t
        max_num_parse_graph_flow_match_sample; /**< Maximal number of flow match samples supported
                                                    for a single parser graph node object. */
    bool parse_graph_flow_match_sample_id_in_out; /**< If set, the device supports setting the value
                                                     of the @ref
                                                     parse_graph_flow_match_sample_attr.field_id.
                                                       If set the device will do best effort to use
                                                     the same field id. */
    uint16_t
        max_parse_graph_header_length_base_value; /**< Maximal value for the header length base. */
    uint8_t
        max_parse_graph_flow_match_sample_field_base_offset_value; /**< Maximal value for match
                                                                      sample field base offset. */
    uint8_t
        parse_graph_header_length_field_mask_width; /**< Number of valid bits in @ref
                                                       parser_graph_node_attr.header_length_field_mask,
                                                       For example, value 5 indicates bits[4:0]
                                                       are valid*/
    bool is_flow_table_caps_supported; /**< Capability to query flow table HCH.cap */
    flow_table_capabilities flow_table_caps; /**< Flow table from type receive capabilities */
    nvmeotcp_capabilities nvmeotcp_caps; /**< NVMe/TCP capabilities flags */
} adapter_hca_capabilities;

typedef std::unordered_map<int, void*> caps_map_t;
typedef std::function<void(adapter_hca_capabilities* external_hca_caps, const caps_map_t& caps_map)>
    cap_cb_fn;

typedef enum {
    QOS_NONE,
    QOS_PACKET_PACING // PacketPacing parameters
} QOS_TYPE;

typedef struct _QOS_attributes {
    QOS_TYPE qos_type;
    union {
        qos_packet_pacing packet_pacing_attr;
    } qos_attr;
} qos_attributes;

struct sq_attr {
    qos_attributes* qos_attrs;
    uint32_t qos_attrs_sz;
    uint32_t tis_num;
    uint32_t cqn;
    uint32_t wqe_num; // Number of WQEs in SQ, must be power of 2
    uint32_t wqe_sz; // WQE size, in bytes
    uint32_t user_index;
};

class sq : public obj {
protected:
    sq_attr m_attr;
    sq_state m_state;
    uint32_t m_wqe_num; // should be **2
    uint32_t m_wqe_sz; // should be 64 bytes

public:
    sq(dcmd::ctx* ctx, sq_attr& attr);
    /**
     * @brief Changes state of RQ
     *
     * @param [in] new_state The requested new state
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status modify_state(sq_state new_state);
    /**
     * @brief Returns SQ WQEe size in bytes
     * @param [out] wq_sz      SQ WQE size in bytes
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_wqe_sz(uint32_t& wqe_sz);
    /**
     * @brief Returns SQ WQEs number
     * @param [out] wqe_num      SQ WQEs number
     *
     * @retval Returns DPCP_OK on success.
     */
    virtual status get_wqe_num(uint32_t& wqe_num);
    virtual status get_cqn(uint32_t& cqn);
};

/**
 * @brief class pp_sq - Handles Send Queue with Packet Pacing rate
 *
 */
class pp_sq : public sq {
    friend class adapter;

private:
    uar_t* m_uar;
    adapter* m_adapter;

    void* m_wq_buf;
    dcmd::umem* m_wq_buf_umem;

    uint32_t* m_db_rec;
    dcmd::umem* m_db_rec_umem;

    void* m_pp;

    size_t m_wqe_num; // Number of WQEs in SQ, must be power of 2
    size_t m_wqe_sz; // WQE size, i.e. number of DS (16B) in each SQ WQE, must be
                     // power of 2
    uint32_t m_wq_buf_sz_bytes;
    uint32_t m_wq_buf_umem_id;
    uint32_t m_db_rec_umem_id;
    uint32_t m_pp_idx; // Packet Pacing index
    wq_type m_wq_type;

    pp_sq(adapter* ad, sq_attr& attr);

    status create();
    status init(const uar_t* sq_uar);
    status allocate_wq_buf(void*& buf, size_t sz);
    status allocate_db_rec(uint32_t*& db_rec, size_t& sz);

public:
    virtual ~pp_sq();
    /**
     * @brief Returns virtual address of RQ WQ buffer
     * @param [out] wq_buf_addr      RQ WQ buffer address
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_wq_buf(void*& wq_buf_addr);
    /**
     * @brief Returns virtual address of SQ DoorBell record
     * @param [out] db_rec      DB record address to be stored to
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_dbrec(uint32_t*& db_rec);
    /**
     * @brief Returns virtual address of BlueFlame register
     * @param [out] bd_reg      BF register address
     * @param [in] offset       BF register offset
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_bf_reg(uint64_t*& bf_reg, size_t offset = 0);
    /**
     * @brief Returns virtual address of RQ UAR page
     * @param [out] uar_page      RQ UAR page address
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_uar_page(volatile void*& uar_page);
    /**
     * @brief Returns Send Queue WQ buffer size in bytes
     *
     * @retval WQ buffer size.
     */
    inline size_t get_wq_buf_sz() const
    {
        return m_wq_buf_sz_bytes;
    }
    /**
     * @brief Modifies Send Queue for new Packet Pacing rate
     * @param [in] attr  Send Queue attributes, qos_attributes with packet pacing is mandatory
     *
     * @retval Returns DPCP_OK on success.
     */
    status modify(sq_attr& attr);
    virtual status destroy();
};

/**
 * @brief: Header tunneling type for parser graph node sampling.
 *
 * The same parser node can be used for parsing a header that
 * will be in an outer part of a tunnel or in inner of a tunnel.
 * Those are the indicators which of the options will be sampled.
 */
enum parse_graph_flow_match_sample_tunnel_mode {
    PARSE_GRAPH_FLOW_MATCH_SAMPLE_TUNNEL_OUTER = 0x0, /**< The outer part of the tunneled header. */
    PARSE_GRAPH_FLOW_MATCH_SAMPLE_TUNNEL_INNER = 0x1, /**< The inner part of the tunneled header. */
    PARSE_GRAPH_FLOW_MATCH_SAMPLE_TUNNEL_FIRST = 0x2 /**< Not a tunneled header. */
};

/**
 * @brief: Length mode of parser graph node.
 *
 * Defines the mode in which header length is calculated.
 */
enum parse_graph_node_len_mode {
    PARSE_GRAPH_NODE_LEN_FIXED =
        0x0, /**< The header has a fixed size defined by the protocol standard;
                  Header length = @ref parser_graph_node_attr.header_length_base_value */
    PARSE_GRAPH_NODE_LEN_FIELD =
        0x1, /**< The header includes a field indicating the exact header length;
                  Header length = @ref parser_graph_node_attr.header_length_base_value +
                  <length field> << @ref parser_graph_node_attr.header_length_field_shift */
    PARSE_GRAPH_NODE_LEN_BITMASK =
        0x2 /**< The header includes a set of flags indicating which optional fields
                 are included;
                 Header length = @ref parser_graph_node_attr.header_length_base_value +
                 <number of set flags> << @ref parser_graph_node_attr.header_length_field_shift */
};

/**
 * @brief: Offset mode for parser graph node samples.
 *
 * Defines the mode in which the offset of the sampled field for flow match is calculated.
 */
enum parse_graph_flow_match_sample_offset_mode {
    PARSE_GRAPH_SAMPLE_OFFSET_FIXED =
        0x0, /**< The field has a fixed offset relative to the start of the header
                  defined by the protocol standard;
                  Sample offset = @ref parse_graph_flow_match_sample_attr.field_base_offset */
    PARSE_GRAPH_SAMPLE_OFFSET_FIELD =
        0x1, /**< The header includes a field indicating the offset of the sampled field;
                  Sample offset = @ref parse_graph_flow_match_sample_attr.field_base_offset +
                  <offset field> << @ref parse_graph_flow_match_sample_attr.field_offset_shift */
    PARSE_GRAPH_SAMPLE_OFFSET_BITMASK =
        0x2 /**< The header includes a set of flags indicating which optional fields
                 are included, the sample field offset depends on the number of
                 existing optional fields;
                 Sample offset = @ref parse_graph_flow_match_sample_attr.field_base_offset +
                 <number of set flags> << @ref parse_graph_flow_match_sample_attr.field_offset_shift
              */
};

/**
 * @brief: Parser graph node index for an input/output arc.
 */
enum parse_graph_arc_node_index {
    PARSE_GRAPH_ARC_NODE_NULL = 0x0,
    PARSE_GRAPH_ARC_NODE_HEAD = 0x1,
    PARSE_GRAPH_ARC_NODE_MAC = 0x2,
    PARSE_GRAPH_ARC_NODE_IP = 0x3,
    PARSE_GRAPH_ARC_NODE_GRE = 0x4,
    PARSE_GRAPH_ARC_NODE_UDP = 0x5,
    PARSE_GRAPH_ARC_NODE_MPLS = 0x6,
    PARSE_GRAPH_ARC_NODE_TCP = 0x7,
    PARSE_GRAPH_ARC_NODE_VXLAN_GPE = 0x8,
    PARSE_GRAPH_ARC_NODE_GENEVE = 0x9,
    PARSE_GRAPH_ARC_NODE_IPSEC_ESP = 0xa,
    PARSE_GRAPH_ARC_NODE_IPV4 = 0xb,
    PARSE_GRAPH_ARC_NODE_IPV6 = 0xc,
    PARSE_GRAPH_ARC_NODE_PROGRAMMABLE = 0x1f,
};

/**
 * @brief: Parser graph node arc attributes.
 */
struct parse_graph_arc_attr {
    uint16_t compare_condition_value; /**< The parser will follow this arc if the data in this field
                                         is equal to the field indicating the next header type in
                                         the source header. Bits beyond the size of the next header
                                         field are reserved*/
    bool start_inner_tunnel; /**< When set, the source header is considered the end of an
                                encapsulation header, and the following header will be considered
                                part of the encapsulated (inner) packet. When cleared, the
                                encapsulation status (inner/outer) of the
                                  header is unmodified (from previous header). */
    uint8_t arc_parse_graph_node; /**< For an input arc, indicates the parse graph node leading to
                                     the programmed node. For an output arc, indicates the parse
                                     graph node to which the programmed node leads.
                                       See node indexes in @ref parse_graph_arc_node_index. */
    uint32_t parse_graph_node_handle; /**< Programmable parse graph node handle. Valid when @ref
                                         arc_parse_graph_node is @ref
                                         PARSE_GRAPH_ARC_NODE_PROGRAMMABLE. */
};

/**
 * @brief: Parser graph node flow match sample attributes.
 */
struct parse_graph_flow_match_sample_attr {
    bool enabled; /**< When set, the parser should sample an additional field used for
                       flow matching and packet header modifications. */
    uint16_t field_offset; /**< Offset of the field indicating the sample field offset (explicit
                              offset or bitmask), from the start of the header. Given in bits. Valid
                              only if @ref enabled is set. For @ref offset_mode @ref
                              PARSE_GRAPH_SAMPLE_OFFSET_FIXED, this field is reserved. */
    std::bitset<4> offset_mode; /**< Defines the mode in which the offset of the sampled field for
                                   flow match is calculated, see modes in @ref
                                   parse_graph_flow_match_sample_offset_mode. */
    uint32_t field_offset_mask; /**< Bitmask for the field indicating the sample field location
                                     (explicit offset or bitmask). Cleared bits in the mask will
                                   clear the corresponding bits in the field location.
                                     Valid only if @ref enabled is set. */
    std::bitset<4>
        field_offset_shift; /**< Indicates the ratio between the sample field location (explicit
                                 offset or calculated by bitmask) units and bytes. Offset will be
                                 multiplied by 2^field_offset_shift to get the sample field offset
                               in bytes. Valid only if @ref enabled is set. */
    uint8_t
        field_base_offset; /**< For @ref offset_mode @ref PARSE_GRAPH_SAMPLE_OFFSET_FIXED,
                                this is the sample field offset, and must non-negative.
                                For other @ref offset_mode this value will be added to the
                              calculated offset. Value is signed and given in bytes. Valid only if
                              @ref enabled is set. */
    std::bitset<3> tunnel_mode; /**< As the same parser node can be used for parsing a header that
                                     will be in an outer part of a tunnel or in inner of a tunnel.
                                     This field indicates which option will be sampled,
                                     see @ref parse_graph_flow_match_sample_tunnel_mode.
                                     Valid only if @ref enabled is set. */
    uint32_t
        field_id; /**< The ID used for defining a flow match criteria using the sampled header. */
};

/**
 * @brief: Parser graph node attributes.
 */
struct parser_graph_node_attr {
    uint16_t header_length_base_value; /**< For @ref PARSE_GRAPH_NODE_LEN_FIXED this is the header
                                          length, and must be non-negative. For other @ref
                                          parse_graph_node_len_mode this value will be added to
                                          calculated header length. Value is signed and given in
                                          bytes. */
    uint16_t header_length_field_offset; /**< Offset of the field indicating the header length
                                            (explicit length or bitmask), from the start of the
                                            header. Given in bits. For @ref
                                            PARSE_GRAPH_NODE_LEN_FIXED, this field is reserved. */
    uint32_t
        header_length_field_mask; /**< Bitmask for the field indicating the header length (explicit
                                     length or bitmask). Cleared bits in the mask will clear the
                                     corresponding bits in the header length field. Valid bits in
                                     the mask are indicated by
                                       @ref
                                     adapter_hca_capabilities.parse_graph_header_length_field_mask_width.
                                       All other bits are reserved. */
    std::bitset<4> header_length_mode; /**< Defines the mode in which header length is calculated,
                                            see @ref parse_graph_node_len_mode. */
    std::bitset<4> header_length_field_shift; /**< Indicates the ratio between the header length
                                                 field (explicit length or calculated by bitmask)
                                                 units and bytes. Header length will be multiplied
                                                 by 2^header_length_field_shift to get the header
                                                 length in bytes. */
    std::vector<parse_graph_flow_match_sample_attr>
        samples; /**< Indicates the parameters of the flow matching and packet
                      header modifications. */
    std::vector<parse_graph_arc_attr> in_arcs; /**< Indicates the parameters of the arcs pointing
                                               from the parse graph to the programmed node. */
};

/**
 * @brief: Parser graph node.
 *
 * This class implements the programmable parser graph node, also called Flex Parser.
 * It allows to sample fields from custom protocol headers. Those fields can be later
 * referred in the packet steering process.
 */
class parser_graph_node : public obj {
private:
    parser_graph_node_attr m_attrs;
    std::vector<uint32_t> m_sample_ids;
    uint32_t m_parser_graph_node_id;

public:
    /**
     * @brief: Parser graph node constructor.
     *
     * @param [in] ctx - DCMD context.
     * @param [in] attrs - Parser graph node attributes.
     */
    parser_graph_node(dcmd::ctx* ctx, const parser_graph_node_attr& attrs);
    ~parser_graph_node();
    parser_graph_node(const parser_graph_node&) = delete;
    void operator=(const parser_graph_node&) = delete;
    /**
     * @brief: Returns number of samples in this parser graph node.
     *
     * @return: Number of samples in the node.
     */
    uint16_t get_num_of_samples() const
    {
        return static_cast<uint16_t>(m_sample_ids.size());
    }
    /**
     * @brief: Returns sample IDs.
     *
     * @note: In order to get updated sample IDs, the user should first run @ref query.
     *
     * @return: Sample IDs vector.
     */
    const std::vector<uint32_t>& get_sample_ids() const
    {
        return m_sample_ids;
    }
    /**
     * @brief: Creates parser graph node in the HW using the object initialized attributes.
     *
     * @retval: Status of the operation.
     */
    status create();
    /**
     * @brief: Queries parser graph node.
     *
     * The method will query and set the sample IDs of the node.
     *
     * @return: Status of the operation.
     */
    status query();
    /**
     * @brief: Returns parser graph node ID.
     *
     * @note: The ID is valid after a successful call to @ref create.
     *
     * @retval: Returns DPCP_OK on success.
     */
    virtual status get_id(uint32_t& id) override;
};

class tag_buffer_table_obj : public obj {
public:
    struct attr {
        uint32_t modify_field_select;
        uint32_t log_tag_buffer_table_size;
    };

public:
    tag_buffer_table_obj(dcmd::ctx* ctx);
    virtual ~tag_buffer_table_obj();
    status create(const tag_buffer_table_obj::attr& tag_buffer_table_obj_attr);
    status query(tag_buffer_table_obj::attr& tag_buffer_table_obj_attr);
    inline uint32_t get_key_id() const
    {
        return m_key_id;
    }

private:
    uint32_t m_key_id;
};

struct adapter_info {
    std::string name;
    std::string id;
    uint32_t vendor_id; /**< PCI Vendor Id */
    uint32_t vendor_part_id; /**< PCI Vendor Device Id */
};

class adapter {
private:
    status query_hca_caps();
    void set_external_hca_caps();

    dcmd::device* m_dcmd_dev;
    dcmd::ctx* m_dcmd_ctx;
    td* m_td;
    pd* m_pd;
    uar_collection* m_uarpool;
    void* m_ibv_pd;
    uint32_t m_pd_id;
    uint32_t m_td_id;
    uint32_t m_eqn;
    bool m_is_caps_available;
    caps_map_t m_caps;
    adapter_hca_capabilities* m_external_hca_caps;
    std::vector<cap_cb_fn> m_caps_callbacks;
    bool m_opened;
    flow_action_generator m_flow_action_generator;
    std::shared_ptr<flow_table> m_root_table_arr[flow_table_type::FT_END];
    status prepare_basic_rq(basic_rq& srq);
    status verify_flow_table_receive_attr(const flow_table_attr& attr);

public:
    adapter(dcmd::device* dev, dcmd::ctx* ctx);
    ~adapter();

    std::string get_name();

    status set_td(uint32_t tdn);
    inline uint32_t get_td() const
    {
        return m_td_id;
    }

    status set_pd(uint32_t pdn, void* verbs_pd);

    inline uint32_t get_pd() const
    {
        return m_pd_id;
    }

    /**
     * @brief Returns ibv_pd* as void*
     *
     * @param [out] p_ibv_pd       On Success will be set ibv_pd* pointer
     *
     * @retval      Returns DPCP_OK on success
     *              Returns DPCP_ERR_NO_CONTEXT if ibv_pd* was not initialized
     */
    inline status get_ibv_pd(void*& p_ibv_pd)
    {
        if (m_ibv_pd) {
            p_ibv_pd = m_ibv_pd;
            return DPCP_OK;
        }
        return DPCP_ERR_NO_CONTEXT;
    }

    dcmd::ctx* get_ctx() const
    {
        return m_dcmd_ctx;
    }

    void* get_ibv_context();

    /**
     * @brief Get real time for device (supported starting from ConnextX6)
     *
     * @paramp [out] real_time     On success will be set real_time value
     *
     * @retval      Returns DPCP_OK on success
     *              Returns DPCP_ERR_NO_CONTEXT if initial_segment was not
     * initialized
     */
    status get_real_time(uint64_t& real_time);

    /**
     * @brief Perform opening adapter for real line operations
     *
     *
     * @retval      Returns DPCP_OK on success
     *              Returns DPCP_ERR_NO_CONTEXT if initial_segment was not
     *              initialized
     *              Returns DPCP_ERR_NO_MEMORY in case issues with memory
     *              Can return other valid values as result of underlying
     *              object operarions
     */
    status open();

    /**
     * @brief Perform check was adapter opened or not
     *
     *
     * @retval      true if adapter was opened
     *              false if not
     */
    bool is_opened() const
    {
        return m_opened;
    }

    /**
     * @brief Creates and returns direct_mkey
     *
     * @param [in]  address         Virtual Address
     * @param [in]  length          Address Length in bytes
     * @param [in]  flags           Modification flags for Direct Mkey
     * @param [out] mkey            On Success created direct_mkey
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_direct_mkey(void* address, size_t length, mkey_flags flags, direct_mkey*& mkey);

    /**
     * @brief Creates and returns pattern_mkey
     *
     * @param [in]  address         Virtual Address
     * @param [in]  flags           Modification flags for Pattern Mkey
     * @param [in]  stride_num      Number of Mkey strides
     * @param [in]  bb_num          Number of building blocks of the Mkey stride
     *(sets the bb_arr
     *size)
     * @param [in]  bb_arr          Stride building blocks array, size of array
     *must be == bb_num
     * @param [out] mkey            On Success created pattern_mkey
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_pattern_mkey(void* address, mkey_flags flags, size_t stride_num, size_t bb_num,
                               pattern_mkey_bb bb_arr[], pattern_mkey*& mkey);
    /**
     * @brief Creates and returns reserved_mkey
     *
     * @param [in]  type            Reserved Mkey type
     * @param [in]  address         Virtual Address
     * @param [in]  length          Address Length in bytes
     * @param [in]  mkey_flags      Flags
     * @param [out] mkey            On Success created direct_mkey
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_reserved_mkey(reserved_mkey_type type, void* addr, size_t length,
                                mkey_flags flags, reserved_mkey*& mkey);
    /**
     * @brief Creates and returns ref_mkey
     *
     * @param [in]  parent          Parent Memory Key to reference
     * @param [in]  address         Virtual Address
     * @param [in]  length          Address Length in bytes
     * @param [out] mkey            On Success created ref_mkey
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_ref_mkey(mkey* parent, void* address, size_t length, ref_mkey*& mkey);

    /**
     * @brief Creates and returns an extern_mkey
     *
     * @param [in]  address         Virtual Address
     * @param [in]  length          Address Length in bytes
     * @param [in]  id              Valid id of externally registered key
     * @param [out] mkey            On Success created extern_mkey
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_extern_mkey(void* address, size_t length, uint32_t id, extern_mkey*& mkey);

    status create_crypto_mkey(crypto_mkey*& mkey, uint32_t max_sge);

    /**
     * @brief Creates and returns CQ
     *
     * @param [in]  attr            CQ attributes for create
     * @param [out] cq              On Success created cq
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_cq(const cq_attr& attr, cq*& cq);

    /**
     * @brief Creates and returns striding_rq
     *
     * @param [in]  rq_attr         RQ attributes
     * @param [out] rq              On Success created striding_rq
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_striding_rq(const rq_attr& rq_attr, striding_rq*& rq);

    /**
     * @brief Creates and returns regular_rq
     *
     * @param [in]  rq_attr         RQ attributes
     * @param [out] rq              On Success created regular_rq
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_regular_rq(const rq_attr& rq_attr, regular_rq*& rq);

    /**
     * @brief Creates and returns ibq_rq
     *
     * @param [in]  rq_attr         RQ attributes
     * @param [in]  ibq_protocol    How to extract the sequence number from the
     * packet.
     * @param [in]  mkey            Buffer mkey value
     * @param [out] rq              On Success created ibq_rq
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_ibq_rq(rq_attr& rq_attr, dpcp_ibq_protocol ibq_protocol, uint32_t mkey,
                         ibq_rq*& rq);

    /**
     * @brief Creates and returns DPCP TIR
     *
     * @param [in]  tir_attr        Object attributes
     * @param [out] tir_obj         Pointer to TIR object on success
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_tir(const tir::attr& tir_attr, tir*& tir_obj);

    /**
     * @brief Creates and returns DPCP TIS
     *
     * @param [in]  tis_attr        Object attributes
     * @param [out] ts              Pointer to TIS object on success
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_tis(const tis::attr& tis_attr, tis*& tis_obj);

    /**
     * @brief Get root flow table by type
     *
     * @param [in] type: Flow table type
     *
     * @retval Returns pointer to @ref flow_table or nullptr
     */
    std::shared_ptr<flow_table> get_root_table(flow_table_type type);

    /**
     * @brief Creates and returns flow_rule
     *
     * @param [in]  attr            Flow table attributes
     * @param [out] flow_table      Flow table object on success
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_flow_table(flow_table_attr& attr, std::shared_ptr<flow_table>& flow_table);

    /**
     * @brief Creates and returns flow_rule
     *
     * @param [in]  priority        Flow rule priority
     * @param [in]  match_criteria  Rule match criteria
     * @param [out] flow_rule       Flow rule object on success
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_flow_rule(uint16_t priority, match_params& match_criteria, flow_rule*& flow_rule);

    /**
     * @brief Creates Completion Channel
     *
     * @param [out] cch       Completion Channel on success
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_comp_channel(comp_channel*& cch);

    status query_eqn(uint32_t& eqn, uint32_t cpu_vector = 0);

    status get_hca_caps_frequency_khz(uint32_t& freq); // TODO: Deprecate.

    /**
     * @brief Creates and returns pp_sq (PacketPacing SendQueue)
     *
     * @param [in]  sq_attr         SQ attributes
     * @param [out] sq              On Success created pp_sq
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_pp_sq(sq_attr& sq_attr, pp_sq*& sq);

    /**
     * @brief Get general HCA capabilities
     *
     * @param [out]  caps           Reference to the capabilities structure
     *
     * @retval Returns @ref dpcp::status with the status code
     */
    inline status get_hca_capabilities(adapter_hca_capabilities& caps) const
    {
        if (m_is_caps_available) {
            caps = *m_external_hca_caps;
            return DPCP_OK;
        }
        return DPCP_ERR_QUERY;
    }

    /**
     * @brief Creates and returns DPCP TLS DEK
     *
     * @param [in]  dek_attr        Object attributes
     * @param [out] dek_obj         Pointer to DEK object on success
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_tls_dek(const dek_attr& attr, tls_dek*& tls_dek_obj);

    /**
     * @brief Creates and returns DPCP IPSEC DEK
     *
     * @param [in]  dek_attr        Object attributes
     * @param [out] dek_obj         Pointer to DEK object on success
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_ipsec_dek(const dek_attr& attr, ipsec_dek*& ipsec_dek_obj);

    /**
     * @brief Creates and returns DPCP AES XTS DEK
     *
     * @param [in]  dek_attr        Object attributes
     * @param [out] dek_obj         Pointer to DEK object on success
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_aes_txs_dek(const dek_attr& attr, aes_xts_dek*& aes_txs_dek_obj);

    /**
     * @brief Creates Protection Domain for the Adapter.
     *
     * @param [in] ibv_pd      ibv_pd* as void*, default value is null, in this case,
     *                         the adapter will allocate its own Protection Domain, otherwise
     *                         it will use the one provided by the user.
     *
     * @retval      Returns DPCP_OK on success
     *              Returns DPCP_ERR_NO_MEMORY if ibv_pd* was not allocated successfully
     */
    status create_ibv_pd(void* ibv_pd = nullptr);

    /**
     * @brief Creates own_pd for m_pd
     *
     *
     * @retval      Returns DPCP_OK on success
     *              Returns DPCP_ERR_NO_MEMORY if devx_pd* was not allocated successfully
     */
    status create_own_pd();

    /**
     * @brief Creates and returns DPCP Parser Graph Node.
     *
     * @param [in] attributes           Reference to parser graph node attributes.
     * @param [out] _parser_graph_node  Pointer to Parser graph node object on success.
     *
     * @note: The call supported when @ref
     * adapter_hca_capabilities::general_object_types_parse_graph_node is on.
     *
     * @retval: Returns @ref dpcp::status with the status code.
     */
    status create_parser_graph_node(const parser_graph_node_attr& attributes,
                                    parser_graph_node*& _parser_graph_node);

    /**
     * @brief Flushed all DEK objects cache.
     *
     * @retval: Returns DPCP_ERR_MODIFY on error, otherwise DPCP_OK.
     */
    status sync_crypto_tls();

    /**
     * @brief Returns flow action generator.
     *
     * @retval: Returns @ref dpcp::flow_action_generator.
     */
    flow_action_generator& get_flow_action_generator()
    {
        return m_flow_action_generator;
    }

    status create_tag_buffer_table_obj(const tag_buffer_table_obj::attr& tag_buffer_table_obj_attr,
                                       tag_buffer_table_obj*& tag_buffer_table_object);
};

class provider {

    dcmd::device** m_devices; // change to vector?
    size_t m_num_devices;
    dcmd::provider* m_dcmd_provider;
    const char* m_version;

    provider();

    provider(provider const&);
    void operator=(provider const&);

public:
    static status get_instance(provider*& provider, const char* version = dpcp_version);
    const char* get_version()
    {
        return m_version;
    }
    status get_adapter_info_lst(adapter_info* lst, size_t& adapter_num);
    status open_adapter(std::string id, adapter*& adapter);

    //    provider(provider const&) = delete;
    //    void operator=(provider const&) = delete;
};
} // namespace dpcp

#endif /* DPCP_H_ */
