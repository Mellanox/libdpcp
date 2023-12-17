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
        ${CMAKE_CURRENT_LIST_DIR}/adapter_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/dek_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/dpcp_base.cpp
        ${CMAKE_CURRENT_LIST_DIR}/flow_group_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/flow_rule_ex_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/flow_table_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fr_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/mkey_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/obj_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/parser_graph_node_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pd_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pp_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/provider_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rq_ibq_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rq_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/sq_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/td_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tir_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/tis_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/uar_tests.cpp
        ${CMAKE_CURRENT_LIST_DIR}/dpcp_base.h
)
