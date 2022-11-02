# func.m4 - Collection of functions
# 
# Copyright (c) 2020-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# BSD-3-Clause
#
# See file LICENSE for terms.
#

##########################
# Configure functions
#
# Some helper script functions
#
AC_DEFUN([FUNC_CONFIGURE_INIT],
[
show_section_title()
{
    cat <<EOF

============================================================================
== ${1}
============================================================================
EOF
}

show_summary_title()
{
    cat <<EOF

Mellanox DPCP library
============================================================================
Version: ${PRJ_MAJOR}.${PRJ_MINOR}.${PRJ_REVISION}.${PRJ_RELEASE}
Build: ${BUILD_VER}

EOF
}

])
