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

target_sources(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/action.cpp
        ${CMAKE_CURRENT_LIST_DIR}/compchannel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ctx.cpp
        ${CMAKE_CURRENT_LIST_DIR}/device.cpp
        ${CMAKE_CURRENT_LIST_DIR}/flow.cpp
        ${CMAKE_CURRENT_LIST_DIR}/obj.cpp
        ${CMAKE_CURRENT_LIST_DIR}/provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/uar.cpp
        ${CMAKE_CURRENT_LIST_DIR}/umem.cpp
        ${CMAKE_CURRENT_LIST_DIR}/action.h
        ${CMAKE_CURRENT_LIST_DIR}/compchannel.h
        ${CMAKE_CURRENT_LIST_DIR}/ctx.h
        ${CMAKE_CURRENT_LIST_DIR}/def.h
        ${CMAKE_CURRENT_LIST_DIR}/device.h
        ${CMAKE_CURRENT_LIST_DIR}/flow.h
        ${CMAKE_CURRENT_LIST_DIR}/obj.h
        ${CMAKE_CURRENT_LIST_DIR}/provider.h
        ${CMAKE_CURRENT_LIST_DIR}/uar.h
        ${CMAKE_CURRENT_LIST_DIR}/umem.h
)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR})
