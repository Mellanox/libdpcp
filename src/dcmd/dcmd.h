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

struct modify_action {
    /**
     * @brief modify_action_dst - represent the destination field, the field to modify.
     */
    union modify_action_dst {
        uint32_t data; /**< Used to convert to network byte order */
        struct modify_action_dst_config {
            uint32_t length : 5; /**< Number of bits to be written starting from offset. 0 means
                                      length of 32 bits */
            uint32_t rsvd0 : 3;
            uint32_t offset : 5; /**< The start offset in the field to be modified */
            uint32_t rsvd1 : 3;
            uint32_t field : 12; /**< The field of packet to be modified. OUT_UDP_DPORT,
                                    METADATA_REG_C_0, ... */
            uint32_t action_type : 4; /**< Action Type: MLX5_ACTION_TYPE_SET, MLX5_ACTION_TYPE_ADD,
                                         ... */
        } config;
    } dst;
    /**
     * @brief modify_action_src - represent the src of the which we change the dst field.
     *        Set - it will represent the value to set the dest field.
     *        Add - it will represent the value to add the dest field.
     *        copy - it will represent the field to copy from.
     */
    union modify_action_src {
        uint32_t data; /**< the data to apply on dst field, valid for set, add.
                            When use copy, it will be used to convers to network byte order*/
        /**
         * @brief modify_action_src_config - represent the src field, it will define the location of
         *        the data to copy. Valid only on modify_action from type copy
         */
        struct modify_action_src_config {
            uint32_t rsvd2 : 8;
            uint32_t offset : 5; /**< offset inside the field */
            uint32_t rsvd3 : 3;
            uint32_t field : 12; /**< field to copy from */
            uint32_t rsvd4 : 4;
        } config;
    } src;
};

struct flow_desc {
    struct flow_match_parameters* match_criteria;
    struct flow_match_parameters* match_value;
    obj_handle* dst_obj;
    mlx5_ifc_dest_format_struct_bits* dst_formats;
    uint32_t flow_id;
    size_t num_dst_obj;
    uint16_t priority;
    dcmd::obj_desc modify_acttions_obj_desc;
    modify_action* modify_actions;
    size_t num_of_actions;

    flow_desc()
        : match_criteria()
        , match_value()
        , dst_obj()
        , dst_formats()
        , flow_id()
        , num_dst_obj()
        , priority()
        , modify_acttions_obj_desc()
        , modify_actions()
        , num_of_actions()
    {
    }
};

struct fwd_dst_desc {
    int type;
    uint32_t id;
    uintptr_t handle;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_H_ */
