/*
* Copyright (c) 2021, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* This software product is a proprietary product of Nvidia Corporation and its affiliates
* (the "Company") and all right, title, and interest in and to the software
* product, including all associated intellectual property rights, are and
* shall remain exclusively with the Company.
*
* This software product is governed by the End User License Agreement
* provided with the software product.
*/
#ifndef __MLX5DOCA_H__
#define __MLX5DOCA_H__

#include "mlx5devx.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Reference:
// https://nvidia.sharepoint.com/:u:/r/sites/NBU-Architecture/SWArch/General/Host%20SW%20Arch/Architecture%20Specs/DOCA/DOCA%20Headers%20drafts/doca_cc.h?csf=1&web=1&e=NaXtgK
//

#define doca_dev devx_device_ctx_s

// DOCA Comm Channel handle for local End Point
struct doca_comm_channel_ep_t;

// DOCA Comm Channel handle for peer address
// address object life span is part of the assosiated doca_comm_channel_ep_t
struct doca_comm_channel_addr_t;

#define SERVICE_NAME_MAX 128         // AF_UNIX max name is 108 bytes

struct doca_comm_channel_init_attr {
    struct       doca_dev *device;
    uint32_t     flags;              // Flags: 0 or O_NONBLOCK
    uint32_t     maxmsgs;            // Max. # of messages on queue
    uint32_t     msgsize;            // Max. message size (bytes)
};

//
// NO Linux file-descriptor (FD) in windows
// use doca_event_channel_set_non_blocking_mode
// to have winevent for WaitForMultipleObjects
//
typedef HANDLE doca_event_channel_t;

// Create local End Point handle
__checkReturn _Ret_maybenull_
struct doca_comm_channel_ep_t *doca_comm_channel_ep_create(struct doca_comm_channel_init_attr *attr);

// Get event channel handle associated with the End Point
doca_event_channel_t doca_comm_channel_ep_event_channel_get(struct doca_comm_channel_ep_t *local_ep);

// Service side listen on all interafces
// 'name' identifies the service, as a SERVICE_NAME_MAX bytes size null-terminated string.
int doca_comm_channel_ep_listen(struct doca_comm_channel_ep_t *local_ep, const char *name);

// Client side Connect
// INPUT:
//   'name' identifies the service to connect to, as a SERVICE_NAME_MAX bytes size null-terminated string.
// OUTPUT:
//   'peer_addr'
// Errors: EFAULT, ECONNREFUSED,
int doca_comm_channel_ep_connect(struct doca_comm_channel_ep_t *local_ep, const char *name, struct doca_comm_channel_addr_t **peer_addr);

// Data Path send
// @msg, len: pointer to the message to be sent, of length bytes
// @flags: MSG_DONTWAIT, MSG_MORE
// @peer_addr: destination address handle of the send operation
// Errors: EAGAIN, ENOTCONN, EFAULT
int doca_comm_channel_ep_sendto(struct doca_comm_channel_ep_t *local_ep, const void *msg, size_t len, int flags, const struct doca_comm_channel_addr_t *peer_addr);

// Data Path recv
// @msg, len: pointer to the buffer where the message should be stored, of maximum len of bytes
// @flags: MSG_DONTWAIT, MSG_PEEK
// @peer_addr: recieved message source address handle
// Errors: EAGAIN, ENOTCONN, EFAULT
int doca_comm_channel_ep_recvfrom(struct doca_comm_channel_ep_t *local_ep, void* msg, size_t len, int flags, struct doca_comm_channel_addr_t **peer_addr);

// Disconnect the End Point from the remote peer
// block until all resources related to peer address are freed
// new connection could be created on the End Point
int doca_comm_channel_ep_disconnect(struct doca_comm_channel_ep_t *local_ep, struct doca_comm_channel_addr_t *peer_addr);

// Release End Point handles (remote and local)
// block until all queued messages are sent
// close fd, and release all internal resources
int doca_comm_channel_ep_destroy(struct doca_comm_channel_ep_t *ep);

// Extract 'user_context' from peer_addr handle
uint64_t doca_comm_channel_peer_addr_user_data_get(struct doca_comm_channel_addr_t *peer_addr);

// Save 'user_context' in peer_addr handle
void doca_comm_channel_peer_addr_user_data_set(struct doca_comm_channel_addr_t *peer_addr, uint64_t user_context);

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // __MLX5DOCA_H__
