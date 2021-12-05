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

#ifndef TESTS_GTEST_COMMON_DEF_H_
#define TESTS_GTEST_COMMON_DEF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h> /* printf PRItn */
#if defined(__linux__)
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <poll.h>
#include <dirent.h>

#else // windows
#include <ws2tcpip.h>
#include <winsock2.h>
#include <Shlobj.h>
#endif

#include <fcntl.h>
#include <ctype.h>
#include <malloc.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <signal.h>
#include <chrono>
#include <thread>

#include "common/gtest.h" /* Google framework header */

#define INLINE __inline

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) ((void)P)
#endif

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#ifdef __GNUC__
#define likely(condition) __builtin_expect(static_cast<bool>(condition), 1)
#define unlikely(condition) __builtin_expect(static_cast<bool>(condition), 0)
#else
#define likely(condition) (condition)
#define unlikely(condition) (condition)
#define CHAR_BIT 8
#endif

#define NOT_IN_USE(a) ((void)(a))

/* Platform specific 16-byte alignment macro switch.
   On Visual C++ it would substitute __declspec(align(16)).
   On GCC it substitutes __attribute__((aligned (16))).
*/

#if defined(_MSC_VER)
#define ALIGN(x) __declspec(align(x))
#else
#define ALIGN(x) __attribute__((aligned(x)))
#endif

#if !defined(EOK)
#define EOK 0 /* no error */
#endif

#ifndef container_of
/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) (type*)((char*)(ptr)-offsetof(type, member))
#endif

#define UNDEFINED_VALUE (-1)

struct gtest_configure_t {
    int log_level;
    int random_seed;
    char adapter[16];
};

#endif /* TESTS_GTEST_COMMON_DEF_H_ */
