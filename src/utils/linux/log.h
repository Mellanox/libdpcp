/*
 * Copyright © 2020-2022 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Nvidia Corporation and its affiliates
 * (the "Company") and all right, title, and interest in and to the software
 * product, including all associated intellectual property rights, are and
 * shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 */

#ifndef SRC_UTILS_LINUX_LOG_H_
#define SRC_UTILS_LINUX_LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

extern int dpcp_log_level;

static inline int check_log_level(int level)
{

    if (dpcp_log_level < 0) {
        char* str = getenv("DPCP_TRACELEVEL");
        if (str) {
            dpcp_log_level = (int)strtol(str, NULL, 0);
        }
    }
    return (dpcp_log_level > level);
}

#define log_fatal(fmt, ...)                                                                        \
    do {                                                                                           \
        if (check_log_level(0))                                                                    \
            fprintf(stderr, "[    FATAL ] " fmt, ##__VA_ARGS__);                                   \
        exit(1);                                                                                   \
    } while (0)

#define log_error(fmt, ...)                                                                        \
    do {                                                                                           \
        if (check_log_level(1))                                                                    \
            fprintf(stderr, "[    ERROR ] " fmt, ##__VA_ARGS__);                                   \
    } while (0)

#define log_warn(fmt, ...)                                                                         \
    do {                                                                                           \
        if (check_log_level(2))                                                                    \
            fprintf(stderr, "[     WARN ] " fmt, ##__VA_ARGS__);                                   \
    } while (0)

#define log_info(fmt, ...)                                                                         \
    do {                                                                                           \
        if (check_log_level(3))                                                                    \
            fprintf(stderr, "[     INFO ] " fmt, ##__VA_ARGS__);                                   \
    } while (0)

#define log_trace(fmt, ...)                                                                        \
    do {                                                                                           \
        if (check_log_level(4))                                                                    \
            fprintf(stderr, "[    TRACE ] " fmt, ##__VA_ARGS__);                                   \
    } while (0)

#define log_hexdump(_ptr, _size)                                                                   \
    do {                                                                                           \
        if (check_log_level(5))                                                                    \
            sys_hexdump((_ptr), (_size));                                                          \
    } while (0)

inline void sys_hexdump(void* ptr, int buflen);

#endif /* SRC_UTILS_LINUX_LOG_H_ */
