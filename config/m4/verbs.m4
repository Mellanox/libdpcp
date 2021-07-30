# verbs.m4 - Parsing verbs capabilities
#
# Copyright (C) Mellanox Technologies Ltd. 2001-2020.  ALL RIGHTS RESERVED.
# See file LICENSE for terms.
#


# Check attributes
# Usage: CHECK_VERBS_ATTRIBUTE([attribute], [header file], [definition])
# Note:
# - [definition] can be omitted if it is equal to attribute
#
AC_DEFUN([CHECK_VERBS_ATTRIBUTE], [
    AC_TRY_LINK(
        [#include <$2>],
        [int attr = (int)$1; attr = attr;],
        [vma_cv_attribute_$1=yes],
        [vma_cv_attribute_$1=no])

    AC_MSG_CHECKING([for attribute $1])
    AC_MSG_RESULT([$vma_cv_attribute_$1])
    AS_IF([test "x$vma_cv_attribute_$1" = "xyes"], [
        AS_IF([test "x$3" = "x"],
            [AC_DEFINE_UNQUOTED([HAVE_$1], [1], [Define to 1 if attribute $1 is supported])],
            [AC_DEFINE_UNQUOTED([HAVE_$3], [1], [Define to 1 if attribute $1 is supported])]
        )
    ])
])

# Check attributes
# Usage: CHECK_VERBS_MEMBER([attribute], [header file], [definition])
#
AC_DEFUN([CHECK_VERBS_MEMBER], [
    AC_CHECK_MEMBER( $1, [AC_DEFINE_UNQUOTED([HAVE_$3], [1], [Define to 1 if attribute $1 is supported])], [], [[#include <$2>]])
])

##########################
# Configure ofed capabilities
#
AC_DEFUN([VERBS_CAPABILITY_SETUP],
[

AC_CHECK_HEADERS([infiniband/verbs.h], ,
    [AC_MSG_ERROR([Unable to find the libibverbs-devel header files])])

AC_CHECK_LIB(ibverbs,
    ibv_get_device_list, [VERBS_LIBS="$VERBS_LIBS -libverbs"],
    AC_MSG_ERROR([ibv_get_device_list() not found.]))

AC_SUBST([VERBS_LIBS])

# Save LIBS
verbs_saved_libs=$LIBS
LIBS="$LIBS $VERBS_LIBS"

# Check if VERBS version
#
vma_cv_verbs=0
vma_cv_verbs_str="None"
AC_TRY_LINK(
#include <infiniband/verbs_exp.h>
,
[
    int access = (int)IBV_EXP_ACCESS_ALLOCATE_MR;
    access = access;
],
[
    vma_cv_verbs=2
    vma_cv_verbs_str="Experimental"
],
[
    AC_CHECK_HEADER([infiniband/verbs.h],
        [AC_CHECK_MEMBERS([struct ibv_query_device_ex_input.comp_mask],
            [vma_cv_verbs=3 vma_cv_verbs_str="Upstream"],
            [vma_cv_verbs=1 vma_cv_verbs_str="Legacy"],
            [[#include <infiniband/verbs.h>]] )],
            [],
            [AC_MSG_ERROR([Can not detect VERBS version])]
    )
])
AC_MSG_CHECKING([for OFED Verbs version])
AC_MSG_RESULT([$vma_cv_verbs_str])
AC_DEFINE_UNQUOTED([DEFINED_VERBS_VERSION], [$vma_cv_verbs], [Define found Verbs version])


# Check if direct hardware operations can be used instead of VERBS API
#
vma_cv_directverbs=0
case "$vma_cv_verbs" in
    1)
        ;;
    2)
        AC_CHECK_HEADER([infiniband/mlx5_hw.h],
            [AC_CHECK_DECL([MLX5_ETH_INLINE_HEADER_SIZE],
                [vma_cv_directverbs=$vma_cv_verbs], [], [[#include <infiniband/mlx5_hw.h>]])])
        ;;
    3)
        AC_CHECK_HEADER([infiniband/mlx5dv.h],
            [AC_CHECK_LIB(mlx5,
                mlx5dv_init_obj, [VERBS_LIBS="$VERBS_LIBS -lmlx5" vma_cv_directverbs=$vma_cv_verbs])])
        ;;
    *)
        AC_MSG_ERROR([Unrecognized parameter 'vma_cv_verbs' as $vma_cv_verbs])
        ;;
esac
AC_MSG_CHECKING([for direct verbs support])
if test "$vma_cv_directverbs" -ne 0; then
    AC_DEFINE_UNQUOTED([DEFINED_DIRECT_VERBS], [$vma_cv_directverbs], [Direct VERBS support])
    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no])
fi

# Check Upstream
#
if test "x$vma_cv_verbs" == x3; then
    CHECK_VERBS_ATTRIBUTE([MLX5DV_CONTEXT_FLAGS_DEVX], [infiniband/mlx5dv.h], [DEVX])
fi
#
if [[ -z $vma_cv_attribute_MLX5DV_CONTEXT_FLAGS_DEVX ]] || [[ "$vma_cv_attribute_MLX5DV_CONTEXT_FLAGS_DEVX" == "no" ]] ; then
     AC_MSG_ERROR([Component DEVX of OFED is required for DPCP. Please install fresh OFED!])
fi

# Restore LIBS
LIBS=$verbs_saved_libs
])
