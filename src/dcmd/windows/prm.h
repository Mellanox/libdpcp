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

#pragma once
#if !defined(SRC_DCMD_WINDOWS_PRM_H_)
#define SRC_DCMD_WINDOWS_PRM_H_

#if defined __cplusplus

typedef uint8_t u8;

#pragma warning(push)
#pragma warning(disable : 4200)

extern "C" {
#include <mlx5_ifc.h>

#if !defined(MLX5_FLOW_DESTINATION_TYPE_TIR)
#define MLX5_FLOW_DESTINATION_TYPE_TIR MLX5_FLOW_CONTEXT_DEST_TYPE_TIR
#endif

struct mlx5_wqe_umr_repeat_ent_seg {
    __be16 stride;
    __be16 byte_count;
    __be32 memkey;
    __be64 va;
};

struct mlx5_wqe_umr_repeat_block_seg {
    __be32 byte_count;
    __be32 op;
    __be32 repeat_count;
    __be16 reserved;
    __be16 num_ent;
    struct mlx5_wqe_umr_repeat_ent_seg entries[0];
};

} // extern "C"
#pragma warning(pop)

#endif // defined __cplusplus
#endif
