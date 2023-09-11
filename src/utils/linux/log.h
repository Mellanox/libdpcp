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
