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

#ifndef SRC_DCMD_WINDOWS_FLOW_H_
#define SRC_DCMD_WINDOWS_FLOW_H_

#include "dcmd/dcmd.h"
#include "dcmd/base/base_flow.h"

namespace dcmd {

class flow : public base_flow {
public:
    flow()
    {
        m_handle = nullptr;
    }
    flow(ctx_handle handle, struct flow_desc* desc);
    virtual ~flow();

private:
    flow_handle m_handle;
    struct mlx5dv_flow_matcher* m_matcher;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_WINDOWS_FLOW_H_ */
