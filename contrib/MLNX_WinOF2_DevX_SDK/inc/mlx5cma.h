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

#ifndef __MLX5_CMA_H__
#define __MLX5_CMA_H__

#include "mlx5verbs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIN_RDMA_CMA_VERSION 1

struct sockaddr;

// cma functions retunring -1 and dexv_errno
#define CMA_RETURN_ERRNO   __checkReturn __success(return == 0) int

//
// rdma_event_channel
// NO Linux file-descriptor (FD) in windows
// use rdma_event_channel_set_non_blocking_mode
// to have winevent for WaitForMultipleObjects
//
struct rdma_event_channel
{
    HANDLE winevent;
};

// only blocking mode is supported since there is no
// Linux file-descriptor (FD) in windows
// use rdma_event_channel_set_non_blocking_mode
// to get event for WaitForMultipleObjects
__checkReturn _Ret_maybenull_
struct rdma_event_channel*
rdma_create_event_channel(void);

void
rdma_destroy_event_channel(
    __in struct rdma_event_channel* channel
);

L2W_RETURN_ERRNO
rdma_event_channel_set_non_blocking_mode(
    __in struct rdma_event_channel* channel
);

struct rdma_cm_id
{
    uint32_t                   version;
    enum rdma_port_space       ps;
    struct ibv_context*        verbs;
    struct rdma_event_channel* channel;
    void*                      context;
    struct rdma_cm_event*      event;
};

enum rdma_cm_event_type
{
    RDMA_CM_EVENT_ADDR_RESOLVED,
    RDMA_CM_EVENT_ADDR_ERROR,
    RDMA_CM_EVENT_ROUTE_RESOLVED,
    RDMA_CM_EVENT_ROUTE_ERROR,
    RDMA_CM_EVENT_CONNECT_REQUEST,
    RDMA_CM_EVENT_CONNECT_RESPONSE,
    RDMA_CM_EVENT_CONNECT_ERROR,
    RDMA_CM_EVENT_UNREACHABLE,
    RDMA_CM_EVENT_REJECTED,
    RDMA_CM_EVENT_ESTABLISHED,
    RDMA_CM_EVENT_DISCONNECTED,
    RDMA_CM_EVENT_DEVICE_REMOVAL,
    RDMA_CM_EVENT_MULTICAST_JOIN,
    RDMA_CM_EVENT_MULTICAST_ERROR,
    RDMA_CM_EVENT_ADDR_CHANGE,
    RDMA_CM_EVENT_TIMEWAIT_EXIT
};

enum rdma_port_space
{
    RDMA_PS_IPOIB = 0x0002,
    RDMA_PS_TCP   = 0x0106,
    RDMA_PS_UDP   = 0x0111,
    RDMA_PS_IB    = 0x013F,
};

struct rdma_conn_param
{
    const void* private_data;
    uint8_t private_data_len;
    uint8_t responder_resources;
    uint8_t initiator_depth;
    uint8_t flow_control;
    uint8_t retry_count;
    uint8_t rnr_retry_count;
    uint8_t srq;
    uint32_t qp_num;
};

struct rdma_ud_param
{
    const void* private_data;
    uint8_t private_data_len;
    struct ibv_ah_attr ah_attr;
    uint32_t qp_num;
    uint32_t qkey;
};

struct rdma_cm_event
{
    struct rdma_cm_id*      id;
    struct rdma_cm_id*      listen_id;
    enum rdma_cm_event_type event;
    int                     status;

    union
    {
        struct rdma_conn_param conn;
        struct rdma_ud_param   ud;
    }
    param;
};

CMA_RETURN_ERRNO
rdma_get_cm_event(
    __in        struct rdma_event_channel* channel,
    __deref_out struct rdma_cm_event**     event
);

CMA_RETURN_ERRNO
rdma_ack_cm_event(
    __in struct rdma_cm_event* event
);

CMA_RETURN_ERRNO
rdma_create_id(
    __in_opt struct rdma_event_channel* channel,
    __deref_out struct rdma_cm_id**     id,
    __in_opt    void*                   context,
    __in        enum rdma_port_space    ps
);

int
rdma_destroy_id(
    __in struct rdma_cm_id* id
);

CMA_RETURN_ERRNO
rdma_bind_addr(
    __in struct rdma_cm_id* id,
    __in struct sockaddr*   addr
);

CMA_RETURN_ERRNO
rdma_resolve_addr(
    __in     struct rdma_cm_id* id,
    __in_opt struct sockaddr*   src_addr,
    __in     struct sockaddr*   dst_addr,
    __in     int                timeout_ms
);

CMA_RETURN_ERRNO
rdma_resolve_route(
    __in struct rdma_cm_id* id,
    __in int                timeout_ms
);

CMA_RETURN_ERRNO
rdma_connect(
    __in struct rdma_cm_id*      id,
    __in struct rdma_conn_param* conn_param
);

CMA_RETURN_ERRNO
rdma_establish(
    __in struct rdma_cm_id* id
);

CMA_RETURN_ERRNO
rdma_listen(
    __in struct rdma_cm_id* id,
    __in int                backlog
);

CMA_RETURN_ERRNO
rdma_get_request(
    __in        struct rdma_cm_id*  listen,
    __deref_out struct rdma_cm_id** id
);

CMA_RETURN_ERRNO
rdma_accept(
    __in     struct rdma_cm_id*      id,
    __in_opt struct rdma_conn_param* conn_param
);

CMA_RETURN_ERRNO
rdma_reject(
    __in struct rdma_cm_id*                       id,
    __in_bcount_opt(private_data_len) const void* private_data,
    __in uint8_t                                  private_data_len
);

CMA_RETURN_ERRNO
rdma_disconnect(
    __in struct rdma_cm_id* id
);

__checkReturn _Ret_notnull_
struct sockaddr*
rdma_get_local_addr(__in struct rdma_cm_id *id);

__checkReturn _Ret_notnull_
struct sockaddr*
rdma_get_peer_addr(__in struct rdma_cm_id* id);

__checkReturn uint16_t
rdma_get_src_port(__in struct rdma_cm_id* id);

__checkReturn uint16_t
rdma_get_dst_port(__in struct rdma_cm_id* id);

__checkReturn _Ret_notnull_
LPCSTR
rdma_event_str(enum rdma_cm_event_type event);

CMA_RETURN_ERRNO
rdma_set_option(
    __in_opt struct rdma_cm_id* id,
    __in int                    level,
    __in int                    optname,
    __in_bcount(optlen) void*   optval,
    __in size_t                 optlen
);

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // __MLX5_CMA_H__

