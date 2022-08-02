/*
 * Copyright © 2020-2022 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Nvidia Corporation and its affiliates
 * (the "Company") and all right, title, and interest in and to the software
 * product, including all associated intellectual property rights, are and
 * shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 */

#ifndef SRC_DCMD_LINUX_ACTION_H_
#define SRC_DCMD_LINUX_ACTION_H_

#include <memory>

#include "dcmd/base/base_action.h"

namespace dcmd {

class action_fwd : public base_action_fwd {
public:
    action_fwd(const std::vector<fwd_dst_desc>& dests);
    virtual int apply(struct flow_desc& desc) const override;
    virtual ~action_fwd() = default;

private:
    std::unique_ptr<uintptr_t[]> m_dst_obj_handls;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_LINUX_ACTION_H_ */
