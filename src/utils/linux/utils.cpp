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

#include <cstdint>
#include <fstream>
#include "utils.h"

static const int DPCP_DEFAULT_CACHELINE_SIZE = 64;
static const char* DPCP_LINUX_CACHE_SIZE_PATH =
    "/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size";

size_t get_cacheline_size()
{
    size_t result = DPCP_DEFAULT_CACHELINE_SIZE;
    std::ifstream is(DPCP_LINUX_CACHE_SIZE_PATH);
    if (is.bad()) {
        return result;
    }
    is >> result;
    return result;
}
