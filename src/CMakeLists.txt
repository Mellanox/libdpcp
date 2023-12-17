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

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if (DPCP_STATIC AND ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux"))
    set(system_include_mock ${PROJECT_BINARY_DIR}/mock_include)
    file(MAKE_DIRECTORY ${system_include_mock})
    CreateSymlink(${CMAKE_CURRENT_LIST_DIR}/api ${system_include_mock}/mellanox)
    
    target_include_directories(${PROJECT_NAME} PUBLIC ${system_include_mock})
    add_subdirectory(${system_include_mock}/mellanox ${CMAKE_CURRENT_LIST_DIR}/api)
else()
    set(dpcp_include_api_directory TRUE)
    add_subdirectory("api")
endif()

foreach(dir_name "dcmd" "dpcp" "utils")
    add_subdirectory(${dir_name})
endforeach()
