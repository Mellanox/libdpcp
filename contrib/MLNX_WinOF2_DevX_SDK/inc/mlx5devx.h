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

#ifndef __DEVX_H__
#define __DEVX_H__

#if defined(MP_NOTHROW) && !defined(__MLX5_DV_WIN_H__)
#define MP_WINDOWS_BUILD_DEVX
#endif

#ifndef MP_KERNEL_MODE_CODE
#include <stdint.h>
#endif

#include "mlx5_ifc_devx.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// L2W_RETURN_ERRNO
// error are C errors negative values (e.g. -EINVAL, see errno.h)
// to check for error for function returning pointers use IS_ERR
// and PTR_ERR to extract the error code (negative)
// Note that ibv_functions return NULL on error w/ devx_errno
//
#define L2W_RETURN_ERRNO   __checkReturn __success(return == 0) int

__checkReturn _Ret_notnull_ int*
devx_errno_ptr(void);

#define devx_errno (*devx_errno_ptr())

#define MLX5_DEVX_DEVICE_NAME_SIZE 128
#define MLX5_DEVX_DEVICE_PNP_SIZE 128

#define HAVE_MLX5_WIN_VF_COMM_CHANNEL 1 //VF comm channel supported in this devx.

// bus, device and function
struct devx_device_bdf
{
    uint32_t bus_id;
    uint32_t dev_id;
    uint32_t fnc_id;
    uint32_t reserved;
};

struct devx_device
{
    // BDF - e.g. 2:1.0
    // VF slot/serial - raw BDF below
    struct devx_device_bdf bdf;

    // NET_LUID - e.g. 0x0006008003000000
    uint64_t        net_luid;

    // NET_IFINDEX
    uint32_t        net_if_index;

    // Mtu - including L2
    uint32_t        mtu_bytes;

    // MAC - e.g. 00-15-5D-6D-01-0D
    uint8_t         eth_mac[6];

    // name - e.g. "Mellanox ConnectX-4 Virtual Adapter"
    char            name[MLX5_DEVX_DEVICE_NAME_SIZE];

    // NetCfgId - e.g. "{9DC6AB4E-ECF7-4ACD-ADA1-D91D3ABD0AE3}"
    // adapter miniport ID
    char            net_cfg_id[48];

    // PnP ID - "PCI\VEN_15B3&DEV_1014&SUBSYS_000815B3&REV_80\6&c1a15f4&0&2"
    char            dev_pnp_id[MLX5_DEVX_DEVICE_PNP_SIZE];

    uint16_t        preferred_numa_node;

    // see MLX5_DEVX_FS_RULES bitmask
    uint32_t        supported_fs_rules;

    // same as NET_IF_MEDIA_CONNECT_STATE
    // 1 connected, 2 disconnected and 0 unknown
    uint8_t         link_state;

    // link speed in bits/sec
    uint64_t        link_speed;

    // versions - provider, firmware and driver
    uint64_t        provider_ver;
    uint64_t        driver_ver;
    uint64_t        fw_ver;

    // raw BDF (not the same on VF)
    struct devx_device_bdf raw_bdf;

    uint8_t         reserved_1[198];
};

//
// devx_get_device_list
// *c_devices = number of adapters in p_devices
// free p_devices using devx_free_device_list
//
L2W_RETURN_ERRNO
devx_get_device_list(
    __out                          size_t*                  c_devices,
    __deref_out_ecount(*c_devices) struct devx_device_bdf** p_devices
);

//
// devx_free_device_list
//
void
devx_free_device_list(
    __in struct devx_device_bdf* devices
);

//
// devx_query_device
//
L2W_RETURN_ERRNO
devx_query_device(
    __in  const struct devx_device_bdf* bdf,
    __out struct devx_device*           info
);

typedef struct devx_device_ctx_s devx_device_ctx;

//
// devx_open_device
// Note: devx function returning pointer should
// be checked for error using IS_ERR/PTR_ERR
// except for ibv functions that return NULL
// w/ devx_errno
//
__checkReturn _Ret_maybenull_
devx_device_ctx*
devx_open_device(
    __in const struct devx_device_bdf* bdf
);

//
// devx_close_device
//
L2W_RETURN_ERRNO
devx_close_device(
    __in devx_device_ctx* device
);

//
// devx_uar_handle
// single physical write-combined page
//
typedef struct devx_uar_handle_s
{
    void*       uar_page;
    uint32_t    uar_index;
    uint32_t    reserved[7];
}
devx_uar_handle;

enum DEVX_ALLOC_UAR_FLAGS
{
    // blue-flame - devx_alloc_uar will error if
    // write-combined memory is not supported
    // (e.g. legacy VMs)
    DEVX_ALLOC_UAR_FLAG_BF = (1 << 0),

    DEVX_ALLOC_UAR_FLAG_NC = (1 << 1),
};

//
// devx_alloc_uar
// flags - see enum DEVX_ALLOC_UAR_FLAGS
//
__checkReturn _Ret_maybenull_
devx_uar_handle*
devx_alloc_uar(
    __in devx_device_ctx* device,
    __in uint32_t         flags
);

//
// devx_free_uar
//
void
devx_free_uar(
    __in devx_uar_handle* uar
);

//
// devx_query_eqn
//
L2W_RETURN_ERRNO
devx_query_eqn(
    __in  devx_device_ctx* device,
    __in  uint32_t         vector,
    __out uint32_t*        eqn
);

struct devx_obj_handle;

//
// devx_cmd
//
L2W_RETURN_ERRNO
devx_cmd(
    __in devx_device_ctx*      device,
    __in_bcount(inlen) void*   in,
    __in size_t                inlen,
    __out_bcount(outlen) void* out,
    __in size_t                outlen
);

//
// devx_obj_create
//
__checkReturn _Ret_maybenull_
struct devx_obj_handle*
devx_obj_create(
    __in devx_device_ctx*      device,
    __in_bcount(inlen) void*   in,
    __in size_t                inlen,
    __out_bcount(outlen) void* out,
    __in size_t                outlen
);

//
// devx_obj_destroy
//
L2W_RETURN_ERRNO
devx_obj_destroy(
    __in struct devx_obj_handle* obj
);

//
// devx_umem_reg
// access - see DEVX_UMEM_ACCESS bits
//
__checkReturn _Ret_maybenull_
struct devx_obj_handle*
devx_umem_reg(
    __in devx_device_ctx*       device,
    __bcount(size)    void*     addr,
    __in              size_t    size,
    __in              int       access,
    __out             uint32_t* id
);

//
// devx_umem_unreg
//
L2W_RETURN_ERRNO
devx_umem_unreg(
    __in struct devx_obj_handle* obj
);

enum MLX5_DEVX_PP_FLAGS
{
    // When used, DevX will allocate a dedicated pp index
    MLX5_DEVX_PP_ALLOC_FLAGS_DEDICATED_INDEX = 1 << 0,
};

//
// devx_pp_handle
// packet pacing index
//
typedef struct devx_pp_handle_s
{
    uint16_t    pp_index;
    uint32_t    reserved[7];
}
devx_pp_handle;

//
// devx_alloc_pp
//
__checkReturn _Ret_maybenull_
devx_pp_handle*
devx_alloc_pp(
    __in    devx_device_ctx*        ctx,
    __in_bcount(inlen) void*        in,
    __in    uint32_t                inlen,
    __in    uint32_t                flags
);

//
// devx_free_pp
//
L2W_RETURN_ERRNO
devx_free_pp(
    __in devx_pp_handle* obj
);

//
// MLX5_DEVX_FS_RULES
// flow steering rules bitmask
// devx_device::supported_fs_rules
enum MLX5_DEVX_FS_RULES
{
    // match destination IPv4 and UDP port
    // always allowed
    // MLX5_DEVX_FS_RULE_DST_IPV4_UDP = (1 << 0),

    // match destination IPv6 and UDP port
    // destination IPv4 and UDP port always allowed
    MLX5_DEVX_FS_RULE_DST_IPV6_UDP = (1 << 1),

    // match destination IPv4 and UDP port and source IPv4
    MLX5_DEVX_FS_RULE_DST_IPV4_UDP_SRC_IP = (1 << 2),

    // match destination Ethernet MAC
    MLX5_DEVX_FS_RULE_DST_ETH_MAC = (1 << 3),

    // match destination IPv4 and UDP port with cvlan
    MLX5_DEVX_FS_RULE_DST_IPV4_UDP_CVLAN = (1 << 4),

    // match promiscuous mode
    MLX5_DEVX_FS_RULE_DST_PROMISC_MODE = (1 << 5),

    // match IPv6 multicast
    MLX5_DEVX_FS_RULE_DST_IPV6_MULTICAST = (1 << 6),

    // match IPv6 and L4 protocol
    MLX5_DEVX_FS_RULE_DST_IPV6_L4_PROTO = (1 << 7),

    // match Ethernet MAC and L4 protocol
    MLX5_DEVX_FS_RULE_DST_ETH_MAC_L4_PROTO = (1 << 8),

    // match IPv6 multicast and any IP version
    MLX5_DEVX_FS_RULE_DST_IPV6_IP_ANY = (1 << 9),

    // match Ethernet MAC and any IP version
    MLX5_DEVX_FS_RULE_DST_ETH_MAC_IP_ANY = (1 << 10),

    // match L4 protocol
    MLX5_DEVX_FS_RULE_DST_L4_PROTO = (1 << 11),

    // match any IP protocol
    MLX5_DEVX_FS_RULE_DST_IP_ANY = (1 << 12),

    // match destination IPv4 and TCP port with cvlan
    MLX5_DEVX_FS_RULE_DST_IPV4_TCP_CVLAN = (1 << 13),
};

//
// devx_fs_rule_add
// in template - mlx5_ifc_devx_fs_rule_add_in_bits
//
__checkReturn _Ret_maybenull_
struct devx_obj_handle*
devx_fs_rule_add(
    __in devx_device_ctx*       device,
    __in_bcount(inlen) void*    in,
    __in               uint32_t inlen
);

//
// devx_fs_rule_del
//
L2W_RETURN_ERRNO
devx_fs_rule_del(
    __in struct devx_obj_handle* obj
);

//
// devx_set_vf_comm_channel
// action_type - see DEVX_COMM_CHANNEL_ACTION
//
L2W_RETURN_ERRNO
devx_vf_comm_channel(
    __in                  devx_device_ctx* ctx,
    __in                  uint16_t         action_type,
    __in                  uint8_t          fWrite,
    __in_bcount(inlen)    void*            in,
    __in                  size_t           inlen,
    __out_bcount(*outlen) void*            out,
    __inout               size_t*          outlen
);

struct rte_pci_addr;

// windows build
#ifdef MP_WINDOWS_BUILD_DEVX
struct rte_pci_addr
{
    uint32_t domain;
    uint8_t  bus;
    uint8_t  devid;
    uint8_t  function;
};
#endif

//
// devx_device_to_pci_addr
//
L2W_RETURN_ERRNO
devx_device_to_pci_addr(
    __in  const struct devx_device* device,
    __out struct rte_pci_addr*      pci_addr
);

//
//
// devx_hca_clock_query
// pObject - object returned from devx_acquire_hca_clock_mapping
// *pp_iseg_internal_timer - RO user mode memory mapping of iseg.internal_timer
//  see PRM "Initialization Segment/internal_timer"
// mapiing is valid while devx_device_ctx is opened
// *p_clock_frequency - accurate clock hertz frequency
// *p_is_stable_clock_frequency - frequency is stable
//
L2W_RETURN_ERRNO
devx_hca_clock_query(
    __in        devx_device_ctx* device,
    __deref_out void**           pp_iseg_internal_timer,
    __out       uint64_t*        p_clock_frequency_hz,
    __out       int*             p_is_stable_clock_frequency
);

//
//
// devx_query_hca_iseg_mapping
// return RO memory mapping to the HCA Initialization Segment
// mlx5_ifc_initial_seg_bits
// *cb_iseg - size in bytes of iseg
// *pp_iseg - pointer to starting iseg address
//
L2W_RETURN_ERRNO
devx_query_hca_iseg_mapping(
    __in        devx_device_ctx*        device,
    __out       uint32_t*               cb_iseg,
    __deref_out_bcount(*cb_iseg) void** pp_iseg
);

//
// devx_ioctl
// generic call for future extensions
// get/setsockopt like function
// not supported - return -EOPNOTSUPP
// see DEVX_IOCTL_OPCODE below
//
L2W_RETURN_ERRNO
devx_ioctl(
    __in              uint32_t    opcode,
    __in              size_t      size,
    __in_bcount(size) const void* data
);

// allow dynamic load of devx_ioctl
// i.e. call via pointer to function
#ifdef altr_devx_ioctl
#define devx_ioctl altr_devx_ioctl
#endif

//
// DEVX_EVENT_TYPE
//
enum DEVX_EVENT_TYPE
{
    // CQ ARM completion - mlx5_ifc_comp_event_bits
    DEVX_EVENT_TYPE_CQE,

    // CQ events - mlx5_ifc_cq_error_bits
    DEVX_EVENT_TYPE_CQ,

    // QP events - mlx5_ifc_qp_events_bits
    DEVX_EVENT_TYPE_QP,

    // general obj events - mlx5_ifc_general_obj_event_bits
    DEVX_EVENT_TYPE_GEN_OBJ,
};

//
// Events notifications API - suggestion
// CQ, QP and SRQ notifications
//
struct devx_event_context
{
    // null terminated list
    struct devx_event_context* next;

    // buffer to hold mlx5_ifc_eqe_bits
    // buffer, when specified, must have capcity
    // of MLX5_ST_SZ_BYTES(eqe)
    // DEVX_EVENT_TYPE_CQE - buffer not used and can be null
    void* buffer;

    // number of HW events (eqes) generated for this context
    // CQE - needed for arm_sn
    uint32_t event_count;

    // internal reference count
    uint32_t ref_count;

    // internal flags
    uint32_t flags;
};

struct devx_event_queue;

//
// devx_event_queue_create
//
__checkReturn _Ret_maybenull_
struct devx_event_queue*
devx_event_queue_create(
    __in uint32_t max_cq_comp_events,
    __in uint32_t max_general_events,
    __in uint32_t flags
);

//
// devx_event_queue_shutdown
// release waiting devx_event_dequeue
//
void
devx_event_queue_shutdown(
    __in struct devx_event_queue* queue
);

//
// devx_event_queue_destroy
// no outstanding devx_event_queue_bind or
// incomming devx_event_dequeue_done
//
void
devx_event_queue_destroy(
    __in struct devx_event_queue* queue
);

//
// devx_event_queue_win_event
// Get event that can be used w/ WaitForMultipleObjects
// once singnaled devx_event_dequeue will not block
//
__checkReturn _Ret_maybenull_
HANDLE
devx_event_queue_win_event(
    __in struct devx_event_queue* queue
);

//
// devx_event_dequeue
// p_head null terminated list
// *max_events == 0 - no limit
// ms_timeout == UINT_MAX - no timeout
// return 0 on timeout/shutdown
//
int
devx_event_dequeue(
    __in            struct devx_event_queue*    queue,
    __inout         size_t*                     max_events,
    __deref_out_opt struct devx_event_context** p_head,
    __in            uint32_t                    ms_timeout
);

//
// devx_event_dequeue_done
// call needed once dequeing caller is done with the context
// this is needed in order to keep the object alive
// when devx_event_queue_unbind is called when context was polled
// return:
//  0 - last reference after devx_event_queue_unbind
//  1 - otherwise
//
__checkReturn int
devx_event_dequeue_done(
    __in struct devx_event_queue*   queue,
    __in struct devx_event_context* ctx
);

//
// devx_event_queue_bind
// associate devx object with event queue
// using completion context
// e.g. CQ ARM notification queueing
// ctx->buffer must be valid for non DEVX_EVENT_TYPE_CQE events
//
L2W_RETURN_ERRNO
devx_event_queue_bind(
    __in struct devx_event_queue*   queue,
    __in struct devx_obj_handle*    obj,
    __in struct devx_event_context* ctx,
    __in enum DEVX_EVENT_TYPE       type
);

//
// devx_event_queue_unbind
// return:
//  0 - pending completion defer cleanup
//      to devx_event_dequeue_done
//  1 - no more completions
//
__checkReturn int
devx_event_queue_unbind(
    __in struct devx_event_queue*   queue,
    __in struct devx_obj_handle*    obj,
    __in struct devx_event_context* ctx,
    __in enum DEVX_EVENT_TYPE       type
);

//
// CQ event notifications for RiverMax
// overlapped IO completion model (ND like)
//

//
// devx_overlapped_file_open
// close handle using CloseHandle
// like ND CreateOverlappedFile
//
L2W_RETURN_ERRNO
devx_overlapped_file_open(
    __in        devx_device_ctx* device,
    __deref_out HANDLE*          p_ovfile
);

//
// devx_overlapped_io_enable
// enable devx_overlapped_io_park for given object
// enable once per object
// for now only DEVX_EVENT_TYPE_CQE is supported on CQ objects
//
L2W_RETURN_ERRNO
devx_overlapped_io_enable(
    __in struct devx_obj_handle* obj,
    __in enum DEVX_EVENT_TYPE    type
);

//
// devx_overlapped_io_park
// park IO overlapped requests
// ovfile - devx_overlapped_file_open
//
// like CQ notify without the ARM
// perform ARM CQ *after* parking to avoid missing interrupts
//
// p_events - number of EQEs arrived for obj
// for CQ it's needed for the arm_sn part of arm_db
//
// type == DEVX_EVENT_TYPE_CQE - on EQE
// all parked overlapped are flushed
//
L2W_RETURN_ERRNO
devx_overlapped_io_park(
    __in  HANDLE                  ovfile,
    __in  struct devx_obj_handle* obj,
    __in  enum DEVX_EVENT_TYPE    type,
    __in  OVERLAPPED*             ovctx,
    __out uint32_t*               p_events
);

//
// devx_overlapped_io_park_ex
// devx_overlapped_io_park like function with output buffer
// to which the mlx5_ifc_eqe_bits can be written
// type == DEVX_EVENT_TYPE_CQE - data must be null
//
L2W_RETURN_ERRNO
devx_overlapped_io_park_ex(
    __in  HANDLE                  ovfile,
    __in  struct devx_obj_handle* obj,
    __in  enum DEVX_EVENT_TYPE    type,
    __in  OVERLAPPED*             ovctx,
    __in  size_t                  size,
    __out_bcount_opt(size) void*  data,
    __out uint32_t*               p_events
);

//
// devx_overlapped_io_flush
// flush parked overlapped IO with ERROR_OPERATION_ABORTED
// like ND CancelOverlappedRequests
//
void
devx_overlapped_io_flush(
    __in struct devx_obj_handle* obj,
    __in enum DEVX_EVENT_TYPE    type
);

//
// devx_shutdown_event
// shutdown status and notification
//
struct devx_shutdown_event
{
    // *p_flag != 0 - adapter is down
    // app should close all resources
    // and retry devx_open_device
    // pointer is alive until devx_open_close
    uint32_t const* p_flag;

    // manual reset event owned by devx
    // the even is signaled once upon shutdown
    // HANDLE has only the SYNCHRONIZE access right
    // Use RegisterWaitForSingleObject to wait
    // without dedicated thread.
    // caller should NOT close it
    // handle is alive until devx_open_close
    HANDLE h_event;

    uint8_t reserved_1[32];
};

//
// devx_query_shutdown_event
//
L2W_RETURN_ERRNO
devx_query_shutdown_event(
    __in  devx_device_ctx*            device,
    __out struct devx_shutdown_event* info
);

#pragma warning(push)
#pragma warning(disable: 6101) // A successful path through the function does not set

//
// dexv_ioctl_out_param
// avoid false C6101 warning
// A successful path through the function does not set
//
__inline void
dexv_ioctl_out_param(__out_opt void* p)
{
    UNREFERENCED_PARAMETER(p);
}

#pragma warning(pop)

//
// DEVX_IOCTL_OPCODE
//
enum DEVX_IOCTL_OPCODE
{
    DEVX_IOCTL_OPCODE_RESOLVE_FROM_IP = 1,
};

//
// devx_ioctl_resolve_from_ip
//
typedef struct
{
    void const* pv_sockaddr;

    // out
    struct devx_device_bdf* bdf;

    // out_opt
    struct devx_device*     info;
}
DEVX_IOCTL_RESOLVE_FROM_IP;

//
// devx_ioctl_resolve_from_ip
// pv_sockaddr - pointer to sockaddr
//
__inline
L2W_RETURN_ERRNO
devx_ioctl_resolve_from_ip(
    __in      void const*             pv_sockaddr,
    __out     struct devx_device_bdf* bdf,
    __out_opt struct devx_device*     info
){
    DEVX_IOCTL_RESOLVE_FROM_IP aData;

    dexv_ioctl_out_param(bdf);
    dexv_ioctl_out_param(info);

    aData.pv_sockaddr = pv_sockaddr;
    aData.bdf  = bdf;
    aData.info = info;

    return devx_ioctl(DEVX_IOCTL_OPCODE_RESOLVE_FROM_IP, sizeof(aData), &aData);
}

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // __DEVX_H__
