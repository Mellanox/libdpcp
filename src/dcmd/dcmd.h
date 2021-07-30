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
#ifndef SRC_DCMD_H_
#define SRC_DCMD_H_

#if defined(__linux__)
#include "prm.h"

#include "def.h"
#include "provider.h"
#include "compchannel.h"
#include "device.h"
#include "ctx.h"
#include "obj.h"
#include "uar.h"
#include "umem.h"
#include "flow.h"
#else

#include "src/dcmd/windows/def.h"
#include "src/dcmd/windows/prm.h"
#include "src/dcmd/windows/provider.h"
#include "src/dcmd/windows/compchannel.h"
#include "src/dcmd/windows/device.h"
#include "src/dcmd/windows/ctx.h"
#include "src/dcmd/windows/obj.h"
#include "src/dcmd/windows/uar.h"
#include "src/dcmd/windows/umem.h"
#include "src/dcmd/windows/flow.h"
#endif

enum {
    DCMD_EOK = 0, /* */
    DCMD_EIO = 5, /* errno */
    DCMD_EINVAL = 22, /* errno */
    DCMD_ENOTSUP = 134 /* errno */
};

#define DCMD_OBJ_SET(_desc, _in, _out)                                                             \
    do {                                                                                           \
        (_desc)->in = (_in);                                                                       \
        (_desc)->inlen = sizeof(_in);                                                              \
        (_desc)->out = (_out);                                                                     \
        (_desc)->outlen = sizeof(_out);                                                            \
    } while (0)

namespace dcmd {

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4200)
#endif
struct flow_match_parameters {
    size_t match_sz;
    uint64_t match_buf[0]; /* Device spec format */
};
#if defined(_WIN32)
#pragma warning(pop)
#endif

struct obj_desc {
    const void* in;
    size_t inlen;
    void* out;
    size_t outlen;
};

struct uar_desc {
    uint32_t flags;
};

struct umem_desc {
    void* addr;
    size_t size;
    uint32_t access;
};

struct flow_desc {
    struct flow_match_parameters* match_criteria;
    struct flow_match_parameters* match_value;
    obj_handle* dst_tir_obj;
    mlx5_ifc_dest_format_struct_bits* dst_formats;
    uint32_t flow_id;
    size_t num_dst_tir;
    uint16_t priority;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_H_ */
