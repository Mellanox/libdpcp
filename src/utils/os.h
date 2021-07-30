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
#ifndef SRC_UTILS_OS_H_
#define SRC_UTILS_OS_H_

#include "log.h"
#include "utils.h"

inline uint32_t align(uint32_t val, uint32_t alignment)
{
    return (val + alignment - 1) & ~(alignment - 1);
}

#endif /* SRC_UTILS_OS_H_ */
