# SPDX-FileCopyrightText: Copyright (c) 2022-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

# OFED places restriction for minimal version of CMake.
# The following are the oldest distros that are supported by OFED:
#   Ubuntu 18.04 supports cmake 3.10.2
#       see https://launchpad.net/ubuntu/bionic/+source/cmake#:~:text=cmake%203.10.2%2D1ubuntu2
#   RHEL 7.2 supports cmake 3.17.5 (installed via EPEL 7 by our build containers, 
#       see https://dl.fedoraproject.org/pub/epel/7/x86_64/Packages/c/#:~:text=cmake3%2D3.17.5%2D1.el7.x86_64.rpm
# Based on these restriction we define the following minimal version of CMake:
cmake_minimum_required(VERSION 3.10.2)

set(DPCP_ROOT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "" FORCE)
list(APPEND CMAKE_MODULE_PATH ${DPCP_ROOT_SOURCE_DIR}/cmake)

include(DpcpUtilities)

DpcpDetermineVersion(DPCP)

project(dpcp 
    VERSION ${DPCP_VERSION}
    DESCRIPTION "Direct Packet Control Plane (DPCP) library" 
    HOMEPAGE_URL https://network.nvidia.com/products/infiniband-drivers/linux/mlnx_ofed/
    LANGUAGES CXX C
)

include(DpcpConfig)

add_library(dpcp_common_deps INTERFACE)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    foreach(lib_name "wsock32" "ws2_32" "legacy_stdio_definitions" "ucrt")
        find_library(${lib_name}_path NAMES ${lib_name} REQUIRED)
        target_link_libraries(dpcp_common_deps INTERFACE ${${lib_name}_path})
    endforeach()
endif()
find_package(Verbs REQUIRED)
target_link_libraries(dpcp_common_deps INTERFACE Verbs::Verbs)

add_library(${PROJECT_NAME} ${DPCP_LIBRARY_ATTRIBUTES})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${PROJECT_VERSION}")
target_link_libraries(${PROJECT_NAME} PRIVATE dpcp_config dpcp_common_deps)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    install(FILES src/api/dpcp.h DESTINATION include/mellanox)
endif()

add_subdirectory(src)
if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # tests/gtest doesn't compile properly for Clang since we have an old version
        # of gtest
        add_subdirectory(tests)
    endif()
endif()
