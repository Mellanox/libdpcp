# SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

include(DpcpUtilities)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set(DPCP_STATIC ON CACHE INTERNAL "" FORCE)
elseif(NOT DEFINED DPCP_STATIC)
    option(DPCP_STATIC "Enables linking with local version of DPCP." OFF)
endif()

if (DPCP_STATIC)
    set(DPCP_LIBRARY_ATTRIBUTES STATIC)
else()
    set(DPCP_LIBRARY_ATTRIBUTES SHARED)
endif()

add_library(dpcp_config INTERFACE)
if (MSVC)
    set(DPCP_C_CXX_FLAGS
        /GS /GL /W4 /Gy 
        /Zc:wchar_t- 
        /Qspectre 
        /Zi /sdl 
        /Zc:inline 
        /fp:fast 
        /D_UNICODE /DUNICODE 
        /D_WIN64 /D_AMD64_ /DAMD64 /DWIN32_LEAN_AND_MEAN=1 
        /D_ALLOW_RUNTIME_LIBRARY_MISMATCH 
        /D_CRT_SECURE_NO_WARNINGS
        /D_SECURE_SCL=0 
        /D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS 
        /D_NO_CRT_STDIO_INLINE /D_CRTIMP_= 
        /errorReport:prompt 
        /WX 
        /Zc:forScope 
        /GR /Gz /Oi /MD 
        /nologo 
        /D_HAS_ITERATOR_DEBUGGING=0 
        /Gm- 
        /EHsc
    )
    set(DPCP_CXX_FLAGS
    )
else()
    set(DPCP_C_CXX_FLAGS
        -Wall
        -Wextra
        -Werror
        -Wundef
        -Wsequence-point
        -Wformat=2
        -Wformat-security
        -ffunction-sections
        -fdata-sections
        -pipe
        -Winit-self
        -Wmissing-include-dirs
        -D_GNU_SOURCE
    )
    if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
        DpcpAppendCompileFlags(DPCP_C_CXX_FLAGS -fstack-protector-strong -fstack-clash-protection)
        set(CMAKE_SHARED_LINKER_FLAGS "-z relro -z now -z noexecstack")
    endif()
    set(DPCP_CXX_FLAGS
        -Wshadow
        -Wno-overloaded-virtual
    )
    target_compile_definitions(dpcp_config INTERFACE _FORTIFY_SOURCE=2)
endif()
target_compile_options(dpcp_config INTERFACE $<$<COMPILE_LANGUAGE:CXX,C>:${DPCP_C_CXX_FLAGS}>) 
target_compile_options(dpcp_config INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${DPCP_CXX_FLAGS}>)
