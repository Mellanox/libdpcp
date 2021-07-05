/*
 Copyright (C) Mellanox Technologies, Ltd. 2001-2021. ALL RIGHTS RESERVED.

 This software product is a proprietary product of Mellanox Technologies, Ltd.
 (the "Company") and all right, title, and interest in and to the software
 product, including all associated intellectual property rights, are and shall
 remain exclusively with the Company.  All rights in or to the software product
 are licensed, not sold.  All rights not licensed are reserved.

 This software product is governed by the End User License Agreement provided
 with the software product.
*/
#pragma once

#ifndef __MLX5VERBS_H__
#define __MLX5VERBS_H__

#include "mlx5devx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef u16 __be16;
typedef u32 __be32;
typedef u64 __be64;

enum ibv_atomic_cap {
    IB_ATOMIC_NONE,
    IB_ATOMIC_HCA,
    IB_ATOMIC_GLOB
};

enum {
    /*
    * Max wqe size for rdma read is 512 bytes, so this
    * limits our max_sge_rd as the wqe needs to fit:
    * - ctrl segment (16 bytes)
    * - rdma segment (16 bytes)
    * - scatter elements (16 bytes each)
    */
    MLX5_MAX_SGE_RD = (512 - 16 - 16) / 16
};

struct ibv_device_attr {
    char        fw_ver[64];
    uint64_t    node_guid;
    uint64_t    sys_image_guid;
    uint32_t    mtu_bytes;
    uint64_t    max_mr_size;
    uint64_t    page_size_cap;
    uint32_t    vendor_id;
    uint32_t    vendor_part_id;
    uint32_t    hw_ver;
    int         max_qp;
    int         max_qp_wr;
    uint32_t    device_cap_flags;
    int         max_send_sge;
    int         max_recv_sge;
    int         max_sq_desc_size;
    int         max_rq_desc_size;
    int         max_sge_rd;
    int         max_cq;
    int         max_cqe;
    int         max_mr;
    int         max_pd;
    int         max_qp_rd_atom;
    int         max_ee_rd_atom;
    int         max_res_rd_atom;
    int         max_qp_init_rd_atom;
    int         max_ee_init_rd_atom;
    enum ibv_atomic_cap	atomic_cap;
    int         max_ee;
    int         max_rdd;
    int         max_mw;
    int         max_raw_ipv6_qp;
    int         max_raw_ethy_qp;
    int         max_mcast_grp;
    int         max_mcast_qp_attach;
    int         max_total_mcast_qp_attach;
    int         max_ah;
    int         max_fmr;
    int         max_map_per_fmr;
    int         max_srq;
    int         max_srq_wr;
    int         max_srq_sge;
    uint16_t    max_pkeys;
    uint8_t     local_ca_ack_delay;
    uint8_t     phys_port_cnt;
};

//
// Note: unlike the devx functions the ibv
// ones return null. IS_ERR(ptr) not needed
//
#define ibv_context devx_device_ctx_s

//ibv_query_device
L2W_RETURN_ERRNO
ibv_query_device(
    __in  struct ibv_context* ctx,
    __out struct ibv_device_attr* attr
);

// ibv_pd
struct ibv_pd
{
    struct ibv_context* context;
    uint32_t	        handle;
};

__checkReturn _Ret_maybenull_
struct ibv_pd*
ibv_alloc_pd(
    __in struct ibv_context* context
);

int
ibv_dealloc_pd(
    __in struct ibv_pd* pd
);

enum ibv_access_flags
{
    IBV_ACCESS_LOCAL_WRITE   = (1 << 0),
    IBV_ACCESS_REMOTE_WRITE  = (1 << 1),
    IBV_ACCESS_REMOTE_READ   = (1 << 2),
    IBV_ACCESS_REMOTE_ATOMIC = (1 << 3),
    IBV_ACCESS_ZERO_BASED    = (1 << 5),
};

// ibv_mr
struct ibv_mr
{
    struct ibv_context* context;
    struct ibv_pd*      pd;
    void*               addr;
    size_t              length;
    uint32_t            handle;
    uint32_t            lkey;
    uint32_t            rkey;
};

// access - see ibv_access_flags
__checkReturn _Ret_maybenull_
struct ibv_mr*
ibv_reg_mr_iova2(
    __in             struct ibv_pd* pd,
    __bcount(length) void*          addr,
    __in             size_t         length,
    __in             uint64_t       iova,
    __in             unsigned       access
);

// access - see ibv_access_flags
__checkReturn _Ret_maybenull_
struct ibv_mr*
ibv_reg_mr(
    __in             struct ibv_pd* pd,
    __bcount(length) void*          addr,
    __in             size_t         length,
    __in             unsigned       access
);

int
ibv_dereg_mr(
    __in struct ibv_mr* mr
);

// ibv_cq
struct ibv_cq
{
    struct ibv_context*      context;
    struct ibv_comp_channel* channel;
    void*                    cq_context;
    uint32_t		     handle;
    int			     cqe;
    uint32_t		     comp_events_completed;
    uint32_t		     async_events_completed;
};

// ibv_comp_channel
// NO Linux file-descriptor (FD) in windows
// use ibv_comp_channel_set_non_blocking_mode
// to have winevent for WaitForMultipleObjects
struct ibv_comp_channel
{
    struct ibv_context* context;
    HANDLE              winevent;
    uint32_t            refcount;
};

__checkReturn _Ret_maybenull_
struct ibv_comp_channel*
ibv_create_comp_channel(
    __in struct ibv_context* context
);

int
ibv_destroy_comp_channel(
    __in struct ibv_comp_channel* channel
);

L2W_RETURN_ERRNO
ibv_comp_channel_set_non_blocking_mode(
    __in struct ibv_comp_channel* channel
);

// CQ events
L2W_RETURN_ERRNO
ibv_get_cq_event(
    __in        struct ibv_comp_channel* channel,
    __deref_out struct ibv_cq**          pcq,
    __out       void**                   pcq_context
);

void
ibv_ack_cq_events(
    __in struct ibv_cq* cq,
    __in unsigned       nevents
);

union ibv_gid
{
    uint8_t raw[16];
    struct
    {
        __be64  subnet_prefix;
        __be64  interface_id;
    }
    global;
};

C_ASSERT(sizeof(union ibv_gid) == 16);

struct ibv_global_route
{
    union ibv_gid dgid;
    uint32_t      flow_label;
    uint8_t       sgid_index;
    uint8_t       hop_limit;
    uint8_t       traffic_class;
};

C_ASSERT(sizeof(struct ibv_global_route) == 24);

struct ibv_ah_attr
{
    struct ibv_global_route grh;
    uint16_t                dlid;
    uint8_t                 sl;
    uint8_t                 src_path_bits;
    uint8_t                 static_rate;
    uint8_t                 is_global;
    uint8_t                 port_num;
};

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // __MLX5VERBS_H__

