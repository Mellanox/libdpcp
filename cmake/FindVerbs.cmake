# SPDX-FileCopyrightText: Copyright (c) 2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

include(FindPackageHandleStandardArgs)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set(DPCP_SDK_PATH_SUFFIX Mellanox/MLNX_WinOF2_DevX_SDK)

    find_path(Verbs_INCLUDE_DIR NAMES mlx5devx.h PATH_SUFFIXES ${DPCP_SDK_PATH_SUFFIX}/inc)
    find_library(Verbs_LIBRARY NAMES mlx5devx PATH_SUFFIXES ${DPCP_SDK_PATH_SUFFIX}/lib)
    if (Verbs_LIBRARY)
        find_file(Verbs_SDK_BUILD_FILE NAMES build_id.txt PATH_SUFFIXES ${DPCP_SDK_PATH_SUFFIX})
        file(STRINGS "${Verbs_SDK_BUILD_FILE}" Verbs_VERSION REGEX "^Version:[ \t]*[0-9\.]+" )
        string(REGEX REPLACE "^Version:[ \t]*([0-9\.]+)" "\\1" Verbs_VERSION "${Verbs_VERSION}")
    endif()
else()
    set(Verbs_FIND_COMPONENTS mlx5 rdmacm)

    foreach(comp_name ${Verbs_FIND_COMPONENTS})
        find_library(${comp_name}_LIBRARY NAMES ${comp_name})
        if (${comp_name}_LIBRARY)
            set(Verbs_${comp_name}_FOUND TRUE)
        else()
            unset(Verbs_${comp_name}_FOUND)
        endif()
    endforeach()

    find_path(Verbs_INCLUDE_DIR NAMES infiniband/verbs.h)
    find_library(Verbs_LIBRARY NAMES ibverbs)
    if (Verbs_LIBRARY)
        file(GLOB Verbs_VERSION "${Verbs_LIBRARY}*.*.*" )
        if (Verbs_VERSION)
            string(REGEX REPLACE "^.*so\.([0-9]+\.[0-9]+\.[0-9]+).*" "\\1" Verbs_VERSION "${Verbs_VERSION}")
        endif()
        if (NOT Verbs_VERSION)
            message(WARNING "Failed to determine the version of the VERBS library.")
            set(Verbs_VERSION "0.0.0")
        endif()
    endif()
endif()

find_package_handle_standard_args(Verbs
  REQUIRED_VARS Verbs_LIBRARY Verbs_INCLUDE_DIR
  VERSION_VAR Verbs_VERSION
  HANDLE_COMPONENTS
)

if (Verbs_FOUND)
    if (NOT TARGET Verbs::Verbs)
        add_library(Verbs::Verbs UNKNOWN IMPORTED)
        set_target_properties(Verbs::Verbs PROPERTIES 
            IMPORTED_LOCATION ${Verbs_LIBRARY}
        )
        target_include_directories(Verbs::Verbs INTERFACE ${Verbs_INCLUDE_DIR})
        target_link_libraries(Verbs::Verbs INTERFACE ${Verbs_FIND_COMPONENTS})
    endif()
endif()
