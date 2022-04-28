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

#if defined __cplusplus

#pragma warning(push)
#pragma warning(disable : 4200)
extern "C" {
#include "prm.h"
} // extern "C"
#pragma warning(pop)

#endif // defined __cplusplus

#include "src/dcmd/windows/def.h"
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
#ifdef _WIN32
// TODO: GalN to rethink with another fix and remove the warning disablement.
#pragma warning(disable : 4201)
#endif // _WIN32

// Same struct as defined in DPDK - mlx5_modification_cmd
// TODO: Need to think if we want to put it in other header file.
struct modify_action {
    union {
        uint32_t data0;
        struct {
            uint32_t length:5;  /* Number of bits to be written starting from offset. 0 means length of 32 bits */
            uint32_t rsvd0:3;
            uint32_t offset:5;  /* The start offset in the field to be modified */
            uint32_t rsvd1:3;
            uint32_t field:12;  /* The field of packet to be modified. ..., OUT_UDP_DPORT [0xC], METADATA_REG_C_0 [0x51], ... */
            uint32_t action_type:4; /* Action Type: SET [0x1], ADD [0x2], COPY[0x3] */
        };
    };
    union {                 /* The data to be written on the specific field */
        uint32_t data1;
        uint8_t data[4];
        struct {
            uint32_t rsvd2:8;
            uint32_t dst_offset:5;
            uint32_t rsvd3:3;
            uint32_t dst_field:12;
            uint32_t rsvd4:4;
        };
    };
};
#ifdef _WIN32
// TODO: GalN to rethink with another fix and remove the warning disablement.
#pragma warning(default : 4201)
#endif // _WIN32
struct flow_desc {
    struct flow_match_parameters* match_criteria;
    struct flow_match_parameters* match_value;
    obj_handle* dst_tir_obj;
    mlx5_ifc_dest_format_struct_bits* dst_formats;
    uint32_t flow_id;
    size_t num_dst_tir;
    uint16_t priority;
    modify_action* modify_actions;
    size_t num_of_actions;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_H_ */
