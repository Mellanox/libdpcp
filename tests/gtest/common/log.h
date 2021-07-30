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

#ifndef TESTS_GTEST_COMMON_LOG_H_
#define TESTS_GTEST_COMMON_LOG_H_

extern struct gtest_configure_t gtest_conf;

#define log_fatal(fmt, ...)                                                                        \
    do {                                                                                           \
        if (gtest_conf.log_level > 0)                                                              \
            fprintf(stderr, "[    FATAL ] " fmt, ##__VA_ARGS__);                                   \
        exit(1);                                                                                   \
    } while (0)

#define log_error(fmt, ...)                                                                        \
    do {                                                                                           \
        if (gtest_conf.log_level > 1)                                                              \
            fprintf(stderr, "[    ERROR ] " fmt, ##__VA_ARGS__);                                   \
    } while (0)

#define log_warn(fmt, ...)                                                                         \
    do {                                                                                           \
        if (gtest_conf.log_level > 2)                                                              \
            fprintf(stderr, "[     WARN ] " fmt, ##__VA_ARGS__);                                   \
    } while (0)

#define log_info(fmt, ...)                                                                         \
    do {                                                                                           \
        if (gtest_conf.log_level > 3)                                                              \
            printf("\033[0;3%sm"                                                                   \
                   "[     INFO ] " fmt "\033[m",                                                   \
                   "4", ##__VA_ARGS__);                                                            \
    } while (0)

#define log_trace(fmt, ...)                                                                        \
    do {                                                                                           \
        if (gtest_conf.log_level > 4)                                                              \
            printf("\033[0;3%sm"                                                                   \
                   "[    TRACE ] " fmt "\033[m",                                                   \
                   "7", ##__VA_ARGS__);                                                            \
    } while (0)

#endif /* TESTS_GTEST_COMMON_LOG_H_ */
