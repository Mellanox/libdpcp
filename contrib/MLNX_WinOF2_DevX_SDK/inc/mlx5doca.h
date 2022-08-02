/*
 * Copyright (c) 2021 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of NVIDIA CORPORATION &
 * AFFILIATES (the "Company") and all right, title, and interest in and to the
 * software product, including all associated intellectual property rights, are
 * and shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 *
 */
#ifndef __MLX5DOCA_H__
#define __MLX5DOCA_H__

#ifdef __cplusplus
extern "C" {
#endif

// doca_error.h
typedef enum doca_error {
    DOCA_SUCCESS,
    DOCA_ERROR_UNKNOWN,
    DOCA_ERROR_NOT_PERMITTED,           /**< Operation not permitted */
    DOCA_ERROR_IN_USE,                  /**< Resource already in use */
    DOCA_ERROR_NOT_SUPPORTED,           /**< Operation not supported */
    DOCA_ERROR_AGAIN,                   /**< Resource temporarily unavailable, try again */
    DOCA_ERROR_INVALID_VALUE,           /**< Invalid input */
    DOCA_ERROR_NO_MEMORY,               /**< Memory allocation failure */
    DOCA_ERROR_INITIALIZATION,          /**< Resource initialization failure */
    DOCA_ERROR_TIME_OUT,                /**< Timer expired waiting for resource */
    DOCA_ERROR_SHUTDOWN,                /**< Shut down in process or completed */
    DOCA_ERROR_CONNECTION_RESET,        /**< Connection reset by peer */
    DOCA_ERROR_CONNECTION_ABORTED,      /**< Connection aborted */
    DOCA_ERROR_CONNECTION_INPROGRESS,   /**< Connection in progress */
    DOCA_ERROR_NOT_CONNECTED,           /**< Not Connected */
} doca_error_t;

/*
 * Arch Reference:
 * https://nvidia.sharepoint.com/:u:/r/sites/NBU-Architecture/SWArch/General/Host%20SW%20Arch/Architecture%20Specs/DOCA/DOCA%20Headers%20drafts/doca_cc.h?csf=1&web=1&e=NaXtgK
 */

/*
 * Linux Reference:
 * http://l-gerrit.mtl.labs.mlnx:8080/gitweb?p=doca.git;a=blob_plain;f=libs/doca_comm_channel/doca_comm_channel.h
 */

#define SERVICE_NAME_MAX 128
#define DOCA_O_NONBLOCK  0x0004      // O_NONBLOCK in Linux

enum doca_comm_channel_msg_flags {
    DOCA_CC_MSG_FLAG_DONTWAIT               = 0x1,    /**< Enables nonblocking operations per send and/or recv calls. if the operation would block, EAGAIN is returned */
    DOCA_CC_MSG_FLAG_MORE                   = 0x2,    /**< Not supported */
};

enum doca_comm_channel_init_flags {
    DOCA_COMM_CHANNEL_INIT_FLAG_NONBLOCK    = 0x1,    /**< EP API will be non-blocking (default is blocking API calls) */
};

/**
 * @brief Handle for local End Point
 */
struct doca_comm_channel_ep_t;

/**
 * @brief Handle for peer address
 */
struct doca_comm_channel_addr_t;

/**
 * @brief Configuration attributes for End Point initialization.
 */
struct doca_comm_channel_init_attr {
    uint32_t    cookie;     /**< Cookie returned when polling the event_channel. */
    uint32_t    flags;      /**< Flags: 0 or DOCA_CC_INIT_FLAG_NONBLOCK. */
    uint32_t    maxmsgs;    /**< Max. # of messages on queue. */
    uint32_t    msgsize;    /**< Max. message size (bytes). */
};

/**
 * @brief Create local End Point handle
 * @param attr
 * Attributes to use when initializing the End Point resources and QPs.
 * @param  ep
 * Output, handle to the newely created ep object.
 * @return
 * If the creation is successfull, ep will point to the newly created endpoint and DOCA_SUCCESS will be returned.
 * Errors:
 * DOCA_ERROR_NOT_PERMITTED if the given msgsize is bigger than supported max size.
 * DOCA_ERROR_NO_MEMORY if memory allocation failed during ep creation.
 * DOCA_ERROR_INITIALIZATION if initialization of ep failed.
 * DOCA_ERROR_UNKNOWN if an unknown error occured.
 */
doca_error_t
doca_comm_channel_ep_create(struct doca_comm_channel_init_attr *attr, struct doca_comm_channel_ep_t **ep);


/**
 * @brief Service side listen on all interafces.
 *
 * The function opens new QP for each vhca_id (gvmi) it exposes to and wait for new connections.
 * After calling this function the user should call doca_comm_channel_ep_recvfrom() in order to get
 * new peers to communicate with.
 *
 * This function available only for service side use.
 *
 * @param name
 * identifies the service, as a SERVICE_NAME_MAX bytes size null-terminated string.
 * @param local_ep
 * handle for the end_point created beforehand with doca_comm_channel_ep_create().
 * @return
 * DOCA_SUCCESS on success
 * Errors:
 * DOCA_ERROR_NOT_PERMITTED if the function was called on the client side.
 * DOCA_ERROR_NO_MEMORY if memory allocation failed.
 * DOCA_ERROR_INITIALIZATION if initialization of service failed.
 * DOCA_ERROR_CONNECTION_ABORTED if registration of service failed.
 * DOCA_ERROR_UNKNOWN if an unknown error occured.
 */
doca_error_t
doca_comm_channel_ep_listen(struct doca_comm_channel_ep_t *local_ep, const char *name);

/**
 * @brief Client side Connect
 *
 * This function available only for client side use.
 * As part of the connection process, the client send a "hello" message to the service to inform
 * him about new connection.
 *
 * If the connect function is being called before the service perform listen with the same name
 * the connection will fail.
 *
 * @param name
 * identifies the service to connect to, as a SERVICE_NAME_MAX bytes size null-terminated string.
 * @param local_ep
 * handle for the end_point created beforehand with doca_comm_channel_ep_create().
 * @param peer_addr
 * Output, handle to use for sending packets and recognize source of messages.
 * @return
 * DOCA_SUCCESS on success.
 * Errors:
 * DOCA_ERROR_NOT_PERMITTED if the function was called on the service or the endpoint is already connected.
 * DOCA_ERROR_NO_MEMORY if memory allocation failed.
 * DOCA_ERROR_INITIALIZATION if initialization of ep connection failed.
 * DOCA_ERROR_CONNECTION_ABORTED if connection failed for any reason (connections rejected or failed).
 * DOCA_ERROR_UNKNOWN if an unknown error occured.
 */
doca_error_t
doca_comm_channel_ep_connect(struct doca_comm_channel_ep_t *local_ep, const char *name, struct doca_comm_channel_addr_t **peer_addr);

/**
 * @brief Send message to peer address.
 *
 * @param msg
 * pointer to the message to be sent.
 * @param len
 * length in bytes of msg.
 * @param flags
 * DOCA_CC_MSG_FLAG_DONTWAIT to return on any case or 0 to block when waiting for credits to arrive.
 * @param peer_addr
 * destination address handle of the send operation.
 * @return
 * DOCA_SUCCESS on success. If the send was successfull, the value pointed by len will be updated with
 * the number of bytes sent.
 * Errors:
 * DOCA_ERROR_NOT_CONNECTED if no peer_address was supplied or no connection was found.
 * DOCA_ERROR_INVALID_VALUE if the supplied was larger than the msgsize given at ep creation.
 * DOCA_ERROR_AGAIN if the command or the endpoint is set to non-blocking mode and no credits are available.
 * DOCA_ERROR_UNKNOWN if an unknown error occured.
 */
doca_error_t
doca_comm_channel_ep_sendto(struct doca_comm_channel_ep_t *local_ep, const void *msg,
                            size_t len, int flags,
                            struct doca_comm_channel_addr_t *peer_addr);

/**
 * @brief Receive message from connected client/service.
 *
 * On service side, doca_comm_channel_ep_recvfrom() also used for accepting new connection from clients.
 *
 * @param msg
 * pointer to the buffer where the message should be stored.
 * @param len
 * input - maximum len of bytes in the msg buffer, output - len of actual received message.
 * @param flags
 * DOCA_CC_MSG_FLAG_DONTWAIT to return on any case or 0 to block when waiting on empty queue.
 * @param peer_addr
 * output, recieved message source address handle
 * @return
 * DOCA_SUCCESS on successful receive. If a message was received, the value pointed by len will be updated with
 * the number of bytes received.
 * Errors:
 * DOCA_ERROR_AGAIN if the command or the endpoint is set to non-blocking mode and no message was received.
 * DOCA_ERROR_UNKNOWN if an unknown error occured.
 */
doca_error_t
doca_comm_channel_ep_recvfrom(struct doca_comm_channel_ep_t *local_ep, void *msg, size_t *len,
                              int flags, struct doca_comm_channel_addr_t **peer_addr);

/**
 * @brief Disconnect the End Point from the remote peer.
 * block until all resources related to peer address are freed
 * new connection could be created on the End Point
 * @return
 * DOCA_SUCCESS on success.
 * Errors:
 * DOCA_ERROR_NOT_CONNECTED if there is no connection.
 */
doca_error_t
doca_comm_channel_ep_disconnect(struct doca_comm_channel_ep_t *local_ep, struct doca_comm_channel_addr_t *peer_addr);

/**
 * @brief Release End Point handle.
 *
 * Blocking until all queued messages are sent
 * The function close the fd and release all internal resources.
 * If the doca_comm_channel_ep_disconnect() is included as part of the destroy process.
 *
 * @return
 * DOCA_SUCCESS on success.
 * Errors:
 * DOCA_ERROR_NOT_CONNECTED if ep does not exist or was already destroyed.
 */
doca_error_t
doca_comm_channel_ep_destroy(struct doca_comm_channel_ep_t *ep);

/**
 * @brief Extract 'user_context' from peer_addr handle.
 *
 * @param peer_addr
 * Pointer to peer_addr to extract user_context from.
 * @param user_data
 * Output param, will contain the extracted data.
 *
 * @return
 * DOCA_SUCCESS on success.
 * DOCA_ERROR_INVALID_VALUE if peer_address or user_data is NULL.
 */
doca_error_t
doca_comm_channel_peer_addr_user_data_get(struct doca_comm_channel_addr_t *peer_addr, uint64_t *user_data);

/**
 * @brief Save 'user_context' in peer_addr handle
 *
 * Can be use by the user to identify the peer address got from doca_comm_channel_ep_recvfrom().
 * The user_context for new peers is initialized to 0.
 *
 * @param peer_addr
 * Pointer to peer_addr to set user_context to.
 * @param user_context
 * Data to set for peer_addr.
 *
 * @return
 * DOCA_SUCCESS on success.
 * DOCA_ERROR_INVALID_VALUE if peer_address is NULL.
 */
doca_error_t
doca_comm_channel_peer_addr_user_data_set(struct doca_comm_channel_addr_t *peer_addr, uint64_t user_context);

/**
 * @brief End Point notification file descriptor for blocking with epoll for recv ready event
 * Use for doca_comm_channels_poll wait on multiple channels
 */
#ifdef __linux__

typedef int doca_event_channel_t;
#define doca_pollfd pollfd

#else  /**< Windows */
typedef HANDLE doca_event_channel_t;

// Linux pollfd like struct
struct doca_pollfd
{
    // fd returned by doca_comm_channel_ep_event_channel_get
    doca_event_channel_t fd;

    uint16_t events;
    uint16_t revents; // DOCA_POLLIN/_POLLOUT
    UINT8    reserved[4];
    void*    pv_reserved;
};

#endif

// POLLIN and POLLOUT
#define DOCA_POLLIN      0x300 // There is data to recv
#define DOCA_POLLOUT     0x010 // Send will not block

//
// Linux poll() like function
// Linux implemenation can directly call poll()
// or even #define doca_comm_channels_poll poll
// on windows the caller must call this function as there is
// no OS implementation
// return number of ready FDs
// return zero on timeout
// return -1 on error w/ errno (devx_errno on windows)
//
int
doca_comm_channels_poll(struct doca_pollfd fds[], uint32_t nfds, int timeout);

// Get event channel handle associated with the End Point
// set in pollfd.fd when calling doca_comm_channels_poll
doca_event_channel_t doca_comm_channel_ep_event_channel_get(struct doca_comm_channel_ep_t *local_ep);

struct doca_comm_channel_peer_addr_info {
    uint16_t     vport;     // peer's switch vport address

    // connection parameters, lib updated these once connection is established
    uint32_t     msgsize;
    uint32_t     maxmsgs;
    uint32_t     curmsgs;   // num of in flight msgs in send queue

    // status: rx/tx msg count/bytes
    uint64_t     rx_msgs;
    uint64_t     tx_msgs;
    uint64_t     rx_bytes;
    uint64_t     tx_bytes;
};

/**
 * @brief Fills peer_address_info with information about the peer_address supplied.
 * @return
 * DOCA_SUCCESS on success.
 * Errors:
 * DOCA_ERROR_NOT_CONNECTED if not connected / no such peer adderss.
 */
doca_error_t
doca_comm_channel_peer_addr_info_get(struct doca_comm_channel_addr_t *peer_addr, struct doca_comm_channel_peer_addr_info *info);


#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* __MLX5DOCA_H__ */
