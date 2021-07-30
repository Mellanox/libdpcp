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
#pragma once

#include <string>
#include <vector>
#include <bitset>
#if __cplusplus < 201103L
#include <stdint.h>
#else
#include <cstdint>
#endif

static const char* dpcp_version = "1.0.0";

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
class ctx;
class device;
class flow;
class obj;
class provider;
class uar;
class umem;
class compchannel;
} // namespace dcmd

namespace dpcp {

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

enum dpcp_dpp_protocol {
    DPCP_DPP_2110 = 0x0, /**< 16 bit RTP sequence number */
    DPCP_DPP_2110_EXT = 0x1, /**< 32 bit RTP sequence number */
    DPCP_DPP_NOT_INITIALIZED
};

class obj {
    uint32_t m_id;
    dcmd::obj* m_obj_handle;
    dcmd::ctx* m_ctx;
    uint32_t m_last_status;
    uint32_t m_last_syndrome;

    obj(obj const&); // for VMA
    void operator=(obj const&); // for VMA

public:
    /**
     * @brief Object constructor, object is initialized but not created yet
     *
     * @param [in]  ctx           Pointer to adapter context
     *
     */
    obj(dcmd::ctx* ctx);
    virtual ~obj(); // destroy_handle handled in DTR

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
    // VMA doesn't support yet C++11
    //    obj(obj const&) = delete;
    //    void operator=(obj const&) = delete;

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

class pd;
class td;
class adapter;
class uar_collection;
struct uar_t;

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
    virtual status get_length(uint32_t& len) = 0;
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
    direct_mkey(adapter* ad, void* address, uint32_t length, mkey_flags flags);
    /**
     * @brief Registers User MEMory in driver and HW
     * Size and pointer were provided in CTR.
     *
     * @retval Returns DPCP_OK on success.
     */
    status reg_mem();
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
    virtual status get_length(uint32_t& len); // override;
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
    virtual status get_length(uint32_t& len)
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
 * Application can create a dpcp::pattern_mkey only via dpcp::adapter->create_pattern_mkey().
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
    virtual status get_length(uint32_t& len);
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
    uint32_t m_length;
    uint32_t m_idx; // memory key index
    reserved_mkey_type m_type;
    mkey_flags m_flags;

public:
    reserved_mkey(adapter* ad, reserved_mkey_type type, void* address, uint32_t length,
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
    virtual status get_length(uint32_t& len); // override;
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
 * @brief enum cq_attr_use - set name for attributes which are valid and to be used or modified
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
 * @brief enum cq_attr_use - set name for attributes which are valid and to be used or modified
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
                                      is enabled) breaks Completion Event Moderation.
                                      CQE causes immediate EQE generation.*/
    ATTR_CQ_OVERRUN_IGNORE_FLAG, /**< When set, overrun ignore is enabled.
                                 When set, updates of CQ consumer counter (poll for
                                 completion) or Request completion notifications
                                 (Arm CQ) DoorBells should not be rung on that CQ */
    ATTR_CQ_PERIOD_MODE_FLAG, /**< 0: upon_event - cq_period timer restarts upon
                              event generation. 1: upon_cqe - cq_period timer restarts
                              upon completion generation */
    ATTR_CQ_MAX_CNT_FLAG
};

/**
 * @brief struct cq_moderation - Describes CQ Moderation attributes, (PRM, sec.8.19.10, Table 171)
 *
 */
struct cq_moderation {
    uint32_t cq_period; /**< Event Generation moderation timer in 1 usec granularity,
                         0 - disabled */
    uint32_t cq_max_cnt; /**<Event Generation Moderation counter, 0 - disabled */
};

/**
 * @brief struct cq_attr - CQ Atrributes for create and modify CQ
 *
 */
struct cq_attr {
    uint32_t cq_sz; /**< size of CQ in CQE numbers (64bytes each), should be power of 2*/
    uint32_t eq_num; /**< CQ reports completion events to this Event Queue Id */
    cq_moderation moderation; /**< moderation attributes */
    std::bitset<ATTR_CQ_MAX_CNT_FLAG> flags; /**< CQ flags */
    std::bitset<CQ_ATTR_MAX_CNT> cq_attr_use; /**< OR'd mask of attribute types which should be
                                 applied and use */
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
    MEMORY_RQ_DPP = 2 /**< Dual Packet Placement memory */
};

enum wq_type {
    WQ_LINKED_LIST = 0x0,
    WQ_CYCLIC = 0x1,
    LINKED_LIST_STRIDING_WQ = 0x2,
    CYCLIC_STRIDING_WQ = 0x3
};

struct rq_attr {
    size_t buf_stride_sz;
    uint32_t buf_stride_num;
    uint32_t user_index;
    uint32_t cqn;
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
    rq(dcmd::ctx* ctx, rq_attr& attr);
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
 * @brief class striding_rq - Handles Striding ReceiveQueue
 *
 */
class striding_rq : public rq {
    friend class adapter;
    uar_t* m_uar;
    adapter* m_adapter;

    void* m_wq_buf;
    dcmd::umem* m_wq_buf_umem;

    uint32_t* m_db_rec;
    dcmd::umem* m_db_rec_umem;

    size_t m_wqe_num; // Number of WQEs in RQ, must be power of 2
    size_t m_wqe_sz; // WQE size, i.e. number of DS (16B) in each RQ WQE, must be power of 2
    uint32_t m_wq_buf_sz_bytes;
    uint32_t m_wq_buf_umem_id;
    uint32_t m_db_rec_umem_id;
    rq_mem_type m_mem_type;
    wq_type m_wq_type;

    striding_rq(adapter* ad, rq_attr& attr, size_t rq_num, size_t wq_sz);

    status create();
    status init(const uar_t* rq_uar);
    status allocate_wq_buf(void*& buf, size_t sz);
    status allocate_db_rec(uint32_t*& db_rec, size_t& sz);

public:
    virtual ~striding_rq();
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
};

/**
 * @brief class dpp_rq - Handles DPP ReceiveQueue
 *
 */
class dpp_rq : public rq {
    friend class adapter;
    adapter* m_adapter;

    dpcp_dpp_protocol m_protocol;
    uint32_t m_mkey;

    dpp_rq(adapter* ad, rq_attr& attr);

    status create();
    status init(dpcp_dpp_protocol protocol, uint32_t mkey);

public:
    virtual ~dpp_rq();
    virtual status destroy();

    /**
     * @brief Returns how dpp extract the sequence number from the packet
     * @param [out] protocol      how dpp extract the sequence number from the
     *packet
     *
     * @retval Returns DPCP_OK on success.
     */
    status get_dpp_protocol(dpcp_dpp_protocol& protocol);

    status get_mkey(uint32_t& mkey);
};

/**
 * @brief Represent and handles TIR object
 *
 */
class tir : public obj {
    rq* m_rq;
    td* m_td;
    uint32_t m_tirn;
    bool m_uc_self_loopback;
    bool m_mc_self_loopback;

public:
    /**
     * @brief TIR Object constructor, object is initialized but not created yet
     *
     * @param [in]  ctx           Pointer to adapter context
     *
     */
    tir(dcmd::ctx* ctx);
    virtual ~tir();

    /**
     * @brief Creates TIR object in HW
     *
     * @param [in]  td           Pointer to TransportDomain object
     * @param [in]  rq           Pointer to ReceiveQueu object
     */
    status create(uint32_t td_id, uint32_t rqn);
    /**
     * @brief Modifies TIR object in HW
     */
    status modify();
    /**
     * @brief Query TIR object in HW
     */
    status query();

    bool set_uc_self_loopback(bool set);
    bool get_uc_self_loopback();

    bool set_mc_self_loopback(bool set);
    bool get_mc_self_loopback();
};

struct match_params {
    uint8_t dst_mac[8]; // 6 bytes + 2 (EOS+alignment)
    uint16_t ethertype;
    uint16_t vlan_id; // 12 bits
    uint32_t dst_ip;
    uint32_t src_ip;
    uint16_t dst_port;
    uint16_t src_port;
    uint8_t protocol;
    uint8_t ip_version; // 4 bits
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
     * @param [in]  dst_tir       Pointer to DPCP tir to remove from this flow rule
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
     * Notice: only after apply is called flow settings will actually configured on the HW.
     *
     * If flow rule is changed after the last apply call there rule state returned by rule get
     *methods
     * will reflect only rule logical state and not the actual HW state, in order to get rule HW
     *state
     * make sure to read it's state (by rule get nethods) after apply call but before any set calls.
     * Notice: By default flow rule masks are zero so if no filters were applied, flow rule action
     * will be always miss for all the traffic. By default rule priority if 0.
     *
     * If apply is called and destination list is 0, apply will fail.
     *
     *
     * @retval Returns DPCP_OK on success.
     */
    status apply_settings();
    /**
     * @brief Revoke flow settings that were configured for this flow rule and applied via apply.
     * If apply was not called revoke_settings returns with failure.
     * call to m_flow->destroy_flow(m_dcmd_flow);
     *
     * @retval Returns DPCP_OK on success.
     */
    status revoke_settings();
    virtual ~flow_rule();
};

struct adapter_info {
    std::string name;
    std::string id;
};

class adapter {
private:
    status query_hca_caps();

    dcmd::device* m_dcmd_dev;
    dcmd::ctx* m_dcmd_ctx;
    td* m_td;
    pd* m_pd;
    uar_collection* m_uarpool;
    uint32_t m_pd_id;
    uint32_t m_td_id;
    uint32_t m_eqn;
    bool m_is_caps_available;
    void* m_caps;

public:
    adapter(dcmd::device* dev, dcmd::ctx* ctx);
    ~adapter();

    std::string get_name();

    status set_td(uint32_t tdn);
    inline uint32_t get_td()
    {
        return m_td_id;
    }

    status set_pd(uint32_t pdn);
    inline uint32_t get_pd()
    {
        return m_pd_id;
    }

    dcmd::ctx* get_ctx()
    {
        return m_dcmd_ctx;
    }

    void* get_ibv_context();

    status open();
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
     * @param [in]  rq_num          Number of WQEs in RQ, must be power of 2
     * @param [in]  wqe_sz          WQE size, i.e. number of DS (16B) in each RQ WQE, must be power
     *of 2
     * @param [out] rq              On Success created striding_rq
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_striding_rq(rq_attr& rq_attr, size_t rq_num, size_t wqe_sz, striding_rq*& rq);
    /**
     * @brief Creates and returns dpp_rq
     *
     * @param [in]  rq_attr        RQ attributes
     * @param [in]  dpp_protocol   How to extract the sequence number from the
     *packet.
     * @param [in]  mkey           Direct Placement mkey of the buffer
     *of 2
     * @param [out] rq              On Success created dpp_rq
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_dpp_rq(rq_attr& rq_attr, dpcp_dpp_protocol dpp_protocol, uint32_t mkey,
                         dpp_rq*& rq);
    /**
     * @brief Creates and returns DPCP TIR
     *
     * @param [in]  rqn             RQ number (index)
     * @param [out] tr              Pointer to TIR object on success
     *
     * @retval      Returns DPCP_OK on success
     */
    status create_tir(uint32_t rqn, tir*& tr);
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

    status create_comp_channel(comp_channel*& cch);

    status query_eqn(uint32_t& eqn, uint32_t cpu_vector = 0);

    /**
     * @brief Get general hca caps
     */
    status get_hca_caps_frequency_khz(uint32_t& freq);
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
