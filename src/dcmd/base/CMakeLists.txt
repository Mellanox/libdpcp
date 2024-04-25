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
        ${CMAKE_CURRENT_LIST_DIR}/base_action.h
        ${CMAKE_CURRENT_LIST_DIR}/base_ctx.h
        ${CMAKE_CURRENT_LIST_DIR}/base_device.h
        ${CMAKE_CURRENT_LIST_DIR}/base_flow.h
        ${CMAKE_CURRENT_LIST_DIR}/base_obj.h
        ${CMAKE_CURRENT_LIST_DIR}/base_provider.h
        ${CMAKE_CURRENT_LIST_DIR}/base_uar.h
        ${CMAKE_CURRENT_LIST_DIR}/base_umem.h
)
