# SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include_guard(GLOBAL)

function(DpcpDetermineVersion output_prefix)
    set(api_header_path ${DPCP_ROOT_SOURCE_DIR}/src/api/dpcp.h)
    if (EXISTS ${api_header_path})
        file(STRINGS ${api_header_path} version_lines REGEX "^.*dpcp_version[ \t]*\=[ \t]*\"[0-9]+\.[0-9]+\.[0-9]+\"")
        if (version_lines)
            string(REGEX REPLACE ".*\"([0-9]+\.[0-9]+\.[0-9]+)[\"\.\-].*" "\\1" ${output_prefix}_VERSION "${version_lines}")
        endif()
    endif()
    if (NOT ${output_prefix}_VERSION)
        message(WARNING "Failed to determine the version of DPCP.")
        set(${output_prefix}_VERSION "0.0.0.0")
    endif()
    set(${output_prefix}_VERSION ${${output_prefix}_VERSION} PARENT_SCOPE)
endfunction()

function(CreateSymlink target_path link_path)
    if((IS_DIRECTORY ${link_path}) OR (IS_SYMLINK ${link_path}))
        return()
    else()
        cmake_policy(PUSH)
        cmake_policy(VERSION 3.17)
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${target_path} ${link_path})
        cmake_policy(POP)
    endif()
endfunction()

include(CheckCXXCompilerFlag)

function(DpcpAppendCompileFlags target_var)
    foreach(compile_flag ${ARGN})
        STRING(REGEX REPLACE "[^A-Za-z0-9]" "_" VarName ${compile_flag})
        check_cxx_compiler_flag(${compile_flag} ${VarName})
        if (${VarName})
            list(APPEND ${target_var} ${compile_flag})
        endif()
    endforeach()
    set(${target_var} ${${target_var}} PARENT_SCOPE)
endfunction()
