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

target_sources(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/adapter.cpp
        ${CMAKE_CURRENT_LIST_DIR}/cq.cpp
        ${CMAKE_CURRENT_LIST_DIR}/dek.cpp
        ${CMAKE_CURRENT_LIST_DIR}/dpcp.cpp
        ${CMAKE_CURRENT_LIST_DIR}/dpcp_obj.cpp
        ${CMAKE_CURRENT_LIST_DIR}/eq.cpp
        ${CMAKE_CURRENT_LIST_DIR}/flow_action.cpp
        ${CMAKE_CURRENT_LIST_DIR}/flow_group.cpp
        ${CMAKE_CURRENT_LIST_DIR}/flow_matcher.cpp
        ${CMAKE_CURRENT_LIST_DIR}/flow_rule_ex.cpp
        ${CMAKE_CURRENT_LIST_DIR}/flow_table.cpp
        ${CMAKE_CURRENT_LIST_DIR}/forwardable_obj.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fr.cpp
        ${CMAKE_CURRENT_LIST_DIR}/mkey.cpp
        ${CMAKE_CURRENT_LIST_DIR}/parser_graph_node.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rq.cpp
        ${CMAKE_CURRENT_LIST_DIR}/sq.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tag_buffer_table_obj.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tir.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tis.cpp
        ${CMAKE_CURRENT_LIST_DIR}/internal.h
)
