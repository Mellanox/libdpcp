# linking_optimization.m4 - Parsing linking options
#
# Copyright (c) 2019-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# BSD-3-Clause
#

AC_DEFUN([LINKING_OPTIMIZATION], [
AC_ARG_ENABLE(lto, AS_HELP_STRING([--enable-lto], [Enable Link Time Optimization]),
     [
        enable_lto=$enableval
     ], [enable_lto=no])

AS_IF([test "x$enable_lto" = "xyes"],
      [
        case $CC in
            gcc*|g++*)
                AC_SUBST([XLIO_LTO], ["-flto=auto"])
                ;;
            clang*|clang++*)
                AC_SUBST([XLIO_LTO], ["-flto=thin"])
                ;;
            *)
                AC_MSG_ERROR([Compiler doesn't support link time optimization])
                ;;
        esac
      ],
      [AC_SUBST([XLIO_LTO], [""])])

AC_ARG_WITH([profile-generate],
    [AS_HELP_STRING([--with-profile-generate=DIR], [Path to store profiles for Profile Guided Optimization])],
    [
        COMMON_FLAGS=""
        case $CC in
            gcc*|g++*)
                COMMON_FLAGS+="-fprofile-generate -fprofile-correction -Wno-error=missing-profile"
                COMMON_FLAGS+=" -fprofile-partial-training -fprofile-dir=$withval"
                ;;
            clang*|clang++*)
                COMMON_FLAGS+="-fprofile-generate=$withval"
                ;;
            *)
                AC_MSG_ERROR([Compiler doesn't support profile guided optimization])
                ;;
        esac
        profile_generate=yes
        AC_CHECK_LIB([gcov], [__gcov_init], [], [AC_MSG_ERROR([libgcov not found])])
        CFLAGS="$CFLAGS $COMMON_FLAGS"
        CXXFLAGS="$CXXFLAGS $COMMON_FLAGS"
        LDFLAGS="$LDFLAGS $COMMON_FLAGS"
        LIBS="$LIBS -lgcov"
    ],
    [profile_generate=no]
)

AC_ARG_WITH([profile-use],
    [AS_HELP_STRING([--with-profile-use=DIR], [Path to read profiles for Profile Guided Optimization])],
    [
        COMMON_FLAGS=""
        case $CC in
            gcc*|g++*)
                COMMON_FLAGS="-fprofile-use -fprofile-correction -Wno-error=missing-profile"
                COMMON_FLAGS+=" -fprofile-partial-training -fprofile-dir=$withval"
                ;;
            clang*|clang++*)
                COMMON_FLAGS+="-fprofile-use=$withval"
                ;;
            *)
                AC_MSG_ERROR([Compiler doesn't support profile guided optimization])
                ;;
        esac
        profile_use=yes
        CFLAGS="$CFLAGS $COMMON_FLAGS"
        CXXFLAGS="$CXXFLAGS $COMMON_FLAGS"
        LDFLAGS="$LDFLAGS $COMMON_FLAGS"
    ],
    [profile_use=no]
)

AS_IF([test "x$profile_use" = "xyes" && test "x$profile_generate" = "xyes"], [
    AC_MSG_ERROR([** Cannot use both --with-profile-generate and --with-profile-use])
])
])
