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
#ifndef SRC_UTILS_LINUX_UTILS_H_
#define SRC_UTILS_LINUX_UTILS_H_

#include <unistd.h>
#include <cstdint>

inline size_t get_page_size()
{
    long page_size = sysconf(_SC_PAGESIZE);
    return (page_size > 0 ? (size_t)page_size : 4096);
}

size_t get_cacheline_size();

#endif /* SRC_UTILS_LINUX_UTILS_H_ */
