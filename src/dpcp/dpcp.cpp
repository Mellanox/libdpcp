/*
 * Copyright (c) 2019-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <atomic>
#include "utils/os.h"
#include "dpcp/internal.h"

namespace dpcp {

provider::provider()
    : m_devices(nullptr)
    , m_num_devices(0)
    , m_dcmd_provider(nullptr)
    , m_version(dpcp_version)
{
}

status provider::get_instance(provider*& provider, const char* version)
{
    int self_version[3] = {0};
    int user_version[3] = {0};

    if (!version) {
        return DPCP_ERR_INVALID_PARAM;
    }

    sscanf(dpcp_version, "%d.%d.%d", &self_version[0], &self_version[1], &self_version[2]);
    sscanf(version, "%d.%d.%d", &user_version[0], &user_version[1], &user_version[2]);
    if ((user_version[0] != self_version[0]) || (user_version[1] > self_version[1])) {
        log_warn("DPCP library version (%d.%d.%d) is incompatible with requested (%d.%d.%d)\n",
                 self_version[0], self_version[1], self_version[2], user_version[0],
                 user_version[1], user_version[2]);
        return DPCP_ERR_NO_SUPPORT;
    }

    log_trace("DPCP library version: %d.%d.%d\n", self_version[0], self_version[1],
              self_version[2]);

    static dpcp::provider self;

    auto dcmd_pr = dcmd::provider::get_instance();
    self.m_dcmd_provider = dcmd_pr ? dcmd_pr : nullptr;

    if (self.m_dcmd_provider) {
        self.m_devices = self.m_dcmd_provider->get_device_list(self.m_num_devices);
        if (nullptr == self.m_devices)
            return DPCP_ERR_NO_DEVICES;
    } else {
        return DPCP_ERR_NO_PROVIDER;
    }
    provider = &self;

    direct_mkey::init_mkeys();

    return DPCP_OK;
}

status provider::get_adapter_info_lst(adapter_info* info_lst, size_t& adapter_num)
{

    if ((0 == adapter_num) || (nullptr == info_lst) || (adapter_num < m_num_devices)) {
        adapter_num = m_num_devices;
        return DPCP_ERR_OUT_OF_RANGE;
    }

    for (int i = 0; i < (int)m_num_devices; i++) {
        adapter_info* p_ai = info_lst + i;

        p_ai->id = m_devices[i]->get_id();
        p_ai->name = m_devices[i]->get_name();
        p_ai->vendor_id = m_devices[i]->get_vendor_id();
        p_ai->vendor_part_id = m_devices[i]->get_vendor_part_id();
        log_trace("%s %x %x\n", p_ai->name.c_str(), p_ai->vendor_id, p_ai->vendor_part_id);
    }
    return DPCP_OK;
}

status provider::open_adapter(std::string adapter_id, adapter*& ad)
{
    if (adapter_id.empty())
        return DPCP_ERR_INVALID_ID;

    for (unsigned i = 0; i < m_num_devices; i++) {
        dcmd::device* dev = m_devices[i];
        if (dev->get_id() == adapter_id) {

            dcmd::ctx* ctx = dev->create_ctx();

            if (nullptr == ctx)
                return DPCP_ERR_NO_DEVICES;

            ad = new (std::nothrow) adapter(dev, ctx);
            if (nullptr != ad) {
                return DPCP_OK;
            }
        }
    }
    return DPCP_ERR_NO_DEVICES;
}

} // namespace dpcp
