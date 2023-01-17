/*
 * Copyright (c) 2020-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "stdafx.h"
#include "provider.h"

using namespace dcmd;

provider* provider::pinstance = 0;

device** provider::get_device_list(size_t& size)
{
    if (nullptr == m_dev_array) {
        dev_handle device_list = nullptr;
        size_t num_devices = 0;
        m_dev_array_size = 0;
        // get device list
        int err = devx_get_device_list(&num_devices, &device_list);
        if (err) {
            goto exit;
        }
        log_trace("\n");
        m_dev_array = new (std::nothrow) device*[num_devices];
        if (m_dev_array) {
            /* search for the given device in the device list */
            for (int i = 0; i < (int)num_devices; ++i) {
                device* dv = create_device(device_list + i);
                if (dv) {
                    m_dev_array[m_dev_array_size++] = dv;
                }
            }
        }

        devx_free_device_list(device_list);
    }

exit:
    size = m_dev_array_size;

    return m_dev_array;
}

device* provider::create_device(dev_handle handle)
{
    device* obj_ptr = nullptr;
    bool can_be_open = false;

    try {
        obj_ptr = new device(handle);
        // test is device can be opened
        ctx* ctx_obj = obj_ptr->create_ctx();
        if (ctx_obj) {
            can_be_open = true;
            delete ctx_obj;
        }
    } catch (...) {
        if (obj_ptr) {
            delete obj_ptr;
        }
        return nullptr;
    }

    if (can_be_open) {
        return obj_ptr;
    }
    return nullptr;
}
