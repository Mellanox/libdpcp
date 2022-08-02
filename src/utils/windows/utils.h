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

#ifndef SRC_UTILS_WINDOWS_UTILS_H_
#define SRC_UTILS_WINDOWS_UTILS_H_

#include <cstdint>
#include <string>
#include <vector>
#include <stdlib.h>
#include <sysinfoapi.h>

#if defined(_WIN32) && BYTE_ORDER == LITTLE_ENDIAN
#include <winsock2.h>

#define htobe16(x) htons(x)
#define htole16(x) (x)
#define be16toh(x) ntohs(x)
#define le16toh(x) (x)
#define htobe32(x) htonl(x)
#define htole32(x) (x)
#define be32toh(x) ntohl(x)
#define le32toh(x) (x)
#define htobe64(x) htonll(x)
#define htole64(x) (x)
#define be64toh(x) ntohll(x)
#define le64toh(x) (x)

#endif

inline std::string dcmd_getenv(const char* name)
{
    size_t size;
    getenv_s(&size, nullptr, 0, name);
    if (size > 0) {
        std::vector<char> tmpvar(size);
        errno_t result = getenv_s(&size, tmpvar.data(), size, name);
        std::string var = (result == 0 ? std::string(tmpvar.data()) : "");
        return var;
    } else {
        return "";
    }
}

inline size_t get_page_size()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwPageSize;
}

size_t get_cacheline_size();

#endif /* SRC_UTILS_WINDOWS_UTILS_H_ */
