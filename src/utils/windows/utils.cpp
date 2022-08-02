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
#include "utils/os.h"

static const int DPCP_DEFAULT_CACHELINE_SIZE = 64;

size_t get_cacheline_size()
{
    size_t result = DPCP_DEFAULT_CACHELINE_SIZE;
    DWORD buffer_size = 0;
    DWORD i = 0;

    GetLogicalProcessorInformation(0, &buffer_size);
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer =
        (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)malloc(buffer_size);
    if (buffer == nullptr) {
        log_error("failed allocating memory");
        return DPCP_DEFAULT_CACHELINE_SIZE;
    }
    GetLogicalProcessorInformation(&buffer[0], &buffer_size);
    for (i = 0; i != buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
        if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == 1) {
            result = buffer[i].Cache.LineSize;
            break;
        }
    }
    free(buffer);
    return result;
}
