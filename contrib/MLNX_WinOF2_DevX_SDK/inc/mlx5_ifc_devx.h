/*
 Copyright (C) Mellanox Technologies, Ltd. 2001-2021. ALL RIGHTS RESERVED.

 This software product is a proprietary product of Mellanox Technologies, Ltd.
 (the "Company") and all right, title, and interest in and to the software
 product, including all associated intellectual property rights, are and shall
 remain exclusively with the Company.  All rights in or to the software product
 are licensed, not sold.  All rights not licensed are reserved.

 This software product is governed by the End User License Agreement provided
 with the software product.
*/
#pragma once

#ifndef __DEVX_IFC_H__
#define __DEVX_IFC_H__

#ifdef __cplusplus
extern "C" {
#endif

// user mode windows build
#if defined(MP_WINDOWS_BUILD_DEVX) && !defined(MP_KERNEL_MODE_CODE)
//
// types needed for mlx5_ifc.h
//
typedef UINT8  u8;
typedef UINT16 u16;
typedef UINT32 u32;
typedef UINT64 u64;

typedef u16 __be16;
typedef u32 __be32;
typedef u64 __be64;

#define be16_to_cpu MpByteSwapU16
#define cpu_to_be16 be16_to_cpu

#define be32_to_cpu MpByteSwapU32
#define cpu_to_be32 be32_to_cpu

#define be64_to_cpu MpByteSwapU64
#define cpu_to_be64 be64_to_cpu

#define IS_ERR(ptr)  devx_l2w_is_error(ptr)
#define PTR_ERR(ptr) devx_l2w_ptr_error(ptr)

#define DECLARE_BITMAP(name, bits) \
    ULONG name[(bits)/32]

#define __packed

#define __LITTLE_ENDIAN
#define BUILD_BUG_ON(condition) C_ASSERT(!(condition))
struct ib_grh { u64 aData[5]; };

#include <../drivex/mlx5/inc/mlx5_ifc.h>
#include <../drivex/mlx5/inc/device.h>

#endif // user mode windows build

#define DEVX_MAX_ERRNO 999
#define DEVX_ENOMEM    12

__inline __checkReturn BOOL
devx_l2w_is_error(void const* p)
{
    LONG_PTR x = (LONG_PTR)p;
    return x >= -DEVX_MAX_ERRNO && x <= 0;
}

__inline __checkReturn int
devx_l2w_ptr_error(void const* p)
{
    if (p == NULL) {
        return -DEVX_ENOMEM;
    }

    return (int)(LONG_PTR)p;
}

#define DEVX_IS_ERR(ptr)  devx_l2w_is_error(ptr)
#define DEVX_PTR_ERR(ptr) devx_l2w_ptr_error(ptr)

#pragma pack(push, 1)

//
// mlx5_ifc_devx_fs_rule_add_in_bits
// extra_dests_count - allow more dest
// mlx5_ifc_dest_format_struct_bits after the end of the struct
//
struct mlx5_ifc_devx_fs_rule_add_in_bits
{
    struct mlx5_ifc_fte_match_param_bits    match_criteria;
    struct mlx5_ifc_fte_match_param_bits    match_value;
    struct mlx5_ifc_dest_format_struct_bits dest;

    u8  prio[0x8];
    u8  is_flow_counter[0x1];
    u8  has_modify_header_id[0x1];
    u8  reserved_1[0x6];
    u8  extra_dests_count[0x8];
    u8  match_criteria_enable[0x8];
    u8  reserved_2[0x8];
    u8  flow_tag[0x18];
    u8  modify_header_id[0x20];
    u8  reserved_x[0xE0];
};

#pragma pack(pop)

enum DEVX_UMEM_ACCESS
{
    // MLX5_IB_MTT_READ - always granted
    DEVX_UMEM_ACCESS_READ      = 0,

    // MLX5_IB_MTT_WRITE
    DEVX_UMEM_ACCESS_WRITE     = (1 << 0),

    // UMEM used only for MPKEYs
    DEVX_UMEM_ACCESS_MKEY_ONLY = (1 << 9),

    // UMEM used only for HW queues
    DEVX_UMEM_ACCESS_HW_QUEUES_ONLY = (1 << 10),
};

enum win_ibv_access_flags
{
    WIN_IBV_ACCESS_LOCAL_WRITE   = (1 << 0),
    WIN_IBV_ACCESS_REMOTE_WRITE  = (1 << 1),
    WIN_IBV_ACCESS_REMOTE_READ   = (1 << 2),
    WIN_IBV_ACCESS_REMOTE_ATOMIC = (1 << 3),
    WIN_IBV_ACCESS_ZERO_BASED    = (1 << 5),
};

// extra PRM like commands
enum {
    DEVX_CMD_OP_CREATE_MKEY  = 0xa200,
    DEVX_CMD_OP_DESTROY_MKEY = 0xa202,
};

#pragma pack(push, 1)

//
// mlx5_ifc_devx_create_mr_in_bits
//
struct mlx5_ifc_devx_create_mr_in_bits
{
    u8  opcode[0x10];
    u8  uid[0x10];
    u8  reserved_1[0x10];
    u8  op_mod[0x10];

    u8  addr[0x40];
    u8  len[0x40];
    u8  iova[0x40];
    u8  access[0x20];
    u8  pdn[0x18];
    u8  reserved_2[0x8];
    u8  reserved_x[0x100];
};

//
// mlx5_ifc_devx_create_mr_out_bits
//
struct mlx5_ifc_devx_create_mr_out_bits
{
    u8  status[0x8];
    u8  reserved_0[0x18];
    u8  syndrome[0x20];

    u8  reserved_1[0x8];
    u8  mkey_index[0x18];
    u8  mkey[0x20];

    u8  reserved_2[0x40];
    u8  reserved_3[0x40];
};

#pragma pack(pop)

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // __DEVX_IFC_H__

