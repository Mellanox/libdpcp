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

#ifndef SRC_DCMD_WINDOWS_DEF_H_
#define SRC_DCMD_WINDOWS_DEF_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <windows.h>
#include <winnt.h>
#include "intrin.h"
#include <mlx5devx.h>

#define ibv_context devx_device_ctx_s

#define mlx5dv_devx_obj devx_obj_handle
#define mlx5dv_devx_obj_create devx_obj_create
#define mlx5dv_devx_obj_destroy devx_obj_destroy
#define mlx5dv_devx_obj_modify devx_cmd
#define mlx5dv_devx_obj_query devx_cmd
#define mlx5dv_devx_general_cmd devx_cmd

#define mlx5dv_devx_umem devx_obj_handle
#define mlx5dv_devx_umem_reg devx_umem_reg
#define mlx5dv_devx_umem_dereg devx_umem_unreg

#define mlx5dv_devx_uar devx_uar_handle_s
#define mlx5dv_devx_alloc_uar devx_alloc_uar
#define mlx5dv_devx_free_uar devx_free_uar
#define mlx5dv_devx_query_eqn devx_query_eqn

#define DEVX_FLD_SZ_BYTES MLX5_FLD_SZ_BYTES
#define DEVX_ST_SZ_BYTES MLX5_ST_SZ_BYTES
#define DEVX_ST_SZ_DW MLX5_ST_SZ_DW
#define DEVX_ST_SZ_QW MLX5_ST_SZ_QW
#define DEVX_UN_SZ_BYTES MLX5_UN_SZ_BYTES
#define DEVX_UN_SZ_DW MLX5_UN_SZ_DW
#define DEVX_BYTE_OFF MLX5_BYTE_OFF
#define DEVX_ADDR_OF MLX5_ADDR_OF

#define DEVX_SET MLX5_SET
#define DEVX_GET MLX5_GET
#define DEVX_SET64 MLX5_SET64
#define DEVX_GET64 MLX5_GET64
/* helper macros */
typedef uint16_t __be16;
typedef uint32_t __be32;
typedef uint64_t __be64;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

inline bool Mlx5QuietFalseImpl()
{
    return FALSE;
}

#define MLX5_QUIET_FALSE Mlx5QuietFalseImpl()

#define be16_to_cpu MpByteSwapU16
#define cpu_to_be16 be16_to_cpu

#define be32_to_cpu MpByteSwapU32
#define cpu_to_be32 be32_to_cpu

#define be64_to_cpu MpByteSwapU64
#define cpu_to_be64 be64_to_cpu

#define BUILD_BUG_ON(condition) C_ASSERT(!(condition))
#define __mlx5_nullp(typ) ((struct mlx5_ifc_##typ##_bits*)0)
#define __mlx5_bit_sz(typ, fld) sizeof(__mlx5_nullp(typ)->fld)
#define __mlx5_bit_off(typ, fld) ((unsigned)(uintptr_t)(&(__mlx5_nullp(typ)->fld)))
#define __mlx5_dw_off(typ, fld) (__mlx5_bit_off(typ, fld) / 32)
#define __mlx5_64_off(typ, fld) (__mlx5_bit_off(typ, fld) / 64)
#define __mlx5_dw_bit_off(typ, fld)                                                                \
    (32 - __mlx5_bit_sz(typ, fld) - (__mlx5_bit_off(typ, fld) & 0x1f))
#define __mlx5_mask(typ, fld) ((u32)((1ull << __mlx5_bit_sz(typ, fld)) - 1))
#define __mlx5_dw_mask(typ, fld) (__mlx5_mask(typ, fld) << __mlx5_dw_bit_off(typ, fld))
#define __mlx5_st_sz_bits(typ) sizeof(struct mlx5_ifc_##typ##_bits)

#define MLX5_FLD_SZ_BYTES(typ, fld) (__mlx5_bit_sz(typ, fld) / 8)
#define MLX5_ST_SZ_BYTES(typ) (sizeof(struct mlx5_ifc_##typ##_bits) / 8)
#define MLX5_ST_SZ_DW(typ) (sizeof(struct mlx5_ifc_##typ##_bits) / 32)
#define MLX5_ST_SZ_QW(typ) (sizeof(struct mlx5_ifc_##typ##_bits) / 64)
#define MLX5_UN_SZ_BYTES(typ) (sizeof(union mlx5_ifc_##typ##_bits) / 8)
#define MLX5_UN_SZ_DW(typ) (sizeof(union mlx5_ifc_##typ##_bits) / 32)
#define MLX5_BYTE_OFF(typ, fld) (__mlx5_bit_off(typ, fld) / 8)
#define MLX5_DW_OFF(typ, fld) __mlx5_dw_off(typ, fld)
#define MLX5_DW_BIT_OFF(typ, fld) __mlx5_dw_bit_off(typ, fld)
#define MLX5_MASK(typ, fld) __mlx5_mask(typ, fld)
#define MLX5_ADDR_OF(typ, p, fld) ((char*)(p) + MLX5_BYTE_OFF(typ, fld))

#define MLX5_SET(typ, p, fld, v)                                                                   \
    do {                                                                                           \
        BUILD_BUG_ON(__mlx5_st_sz_bits(typ) % 32);                                                 \
        BUILD_BUG_ON(__mlx5_bit_sz(typ, fld) > 32);                                                \
        *((__be32*)(p) + __mlx5_dw_off(typ, fld)) =                                                \
            cpu_to_be32((be32_to_cpu(*((__be32*)(p) + __mlx5_dw_off(typ, fld))) &                  \
                         (~__mlx5_dw_mask(typ, fld))) |                                            \
                        (((v)&__mlx5_mask(typ, fld)) << __mlx5_dw_bit_off(typ, fld)));             \
    } while (MLX5_QUIET_FALSE)

#define MLX5_SET_TO_ONES(typ, p, fld)                                                              \
    do {                                                                                           \
        BUILD_BUG_ON(__mlx5_st_sz_bits(typ) % 32);                                                 \
        BUILD_BUG_ON(__mlx5_bit_sz(typ, fld) > 32);                                                \
        *((__be32*)(p) + __mlx5_dw_off(typ, fld)) =                                                \
            cpu_to_be32((be32_to_cpu(*((__be32*)(p) + __mlx5_dw_off(typ, fld))) &                  \
                         (~__mlx5_dw_mask(typ, fld))) |                                            \
                        ((__mlx5_mask(typ, fld)) << __mlx5_dw_bit_off(typ, fld)));                 \
    } while (MLX5_QUIET_FALSE)

#define MLX5_GET(typ, p, fld)                                                                      \
    ((be32_to_cpu(*((__be32*)(p) + __mlx5_dw_off(typ, fld))) >> __mlx5_dw_bit_off(typ, fld)) &     \
     __mlx5_mask(typ, fld))

#define MLX5_GET_PR(typ, p, fld) MLX5_GET(typ, p, fld)
#define MLX5_GET64_PR(typ, p, fld) MLX5_GET64(typ, p, fld)

#define MLX5_SET64(typ, p, fld, v)                                                                 \
    do {                                                                                           \
        BUILD_BUG_ON(__mlx5_bit_sz(typ, fld) != 64);                                               \
        BUILD_BUG_ON(__mlx5_bit_off(typ, fld) % 64);                                               \
        *((__be64*)(p) + __mlx5_64_off(typ, fld)) = cpu_to_be64(v);                                \
    } while (MLX5_QUIET_FALSE)

#define MLX5_SET_STRING(typ, p, fld, v, s)                                                         \
    do {                                                                                           \
        BUILD_BUG_ON(__mlx5_bit_off(typ, fld) % 64);                                               \
        memcpy(((__be64*)(p) + __mlx5_64_off(typ, fld)), v, s);                                    \
    } while (MLX5_QUIET_FALSE)

#define MLX5_GET64(typ, p, fld)                                                                    \
    (((__mlx5_bit_off(typ, fld) % 64) == 0)                                                        \
         ? be64_to_cpu(*((__be64*)(p) + __mlx5_64_off(typ, fld)))                                  \
         : be64_to_cpu(*((__be64 UNALIGNED*)MLX5_ADDR_OF(typ, p, fld))))

#define MLX5_ISEG_OFF(fld) MLX5_BYTE_OFF(initial_seg, fld)

#if !defined(IS_ERR) && !defined(PTR_ERR)

#define UM_MAX_ERRNO 999
#define UM_ENOMEM 12

__inline int mlx5dv_l2w_is_error(void const* p)
{
    LONG_PTR x = (LONG_PTR)p;
    return x >= -UM_MAX_ERRNO && x <= 0;
}

__inline int mlx5dv_l2w_ptr_error(void const* p)
{
    if (p == NULL) {
        return -UM_ENOMEM;
    }

    return (int)(LONG_PTR)p;
}

#define IS_ERR(ptr) mlx5dv_l2w_is_error(ptr)
#define PTR_ERR(ptr) mlx5dv_l2w_ptr_error(ptr)

#endif // IS_ERR

typedef struct devx_device_bdf* dev_handle;
typedef devx_device_ctx* ctx_handle;
typedef struct mlx5dv_devx_obj* obj_handle;
typedef struct mlx5dv_devx_obj* cq_handle;
typedef struct mlx5dv_devx_umem* umem_handle;
typedef struct mlx5dv_devx_uar* uar_handle;
typedef struct devx_obj_handle* flow_handle;
typedef devx_pp_handle pp_handle;

inline uint32_t get_pp_index(pp_handle* pp)
{
    if (pp) {
        return pp->pp_index;
    }
    return 0;
}

#if !defined(EVENT_CHANNEL)
typedef HANDLE event_channel;
#define EVENT_CHANNEL
#endif

#define MpByteSwapU16(x) _byteswap_ushort(x)
#define MpByteSwapU32(x) _byteswap_ulong(x)
#define MpByteSwapU64(x) _byteswap_uint64(x)

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif /* UNUSED */

#pragma warning(push)
#pragma warning(disable : 4200)
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
#pragma warning(pop)

inline void* aligned_alloc(size_t alignment, size_t size)
{
    return ::_aligned_malloc(size, alignment);
}

inline void aligned_free(void* mem_ptr)
{
    ::_aligned_free(mem_ptr);
    return;
}

#endif /* SRC_DCMD_WINDOWS_DEF_H_ */
