/*
Copyright (C) Mellanox Technologies, Ltd. 2020. ALL RIGHTS RESERVED.

This software product is a proprietary product of Mellanox Technologies, Ltd.
(the "Company") and all right, title, and interest in and to the software
product, including all associated intellectual property rights, are and shall
remain exclusively with the Company. All rights in or to the software product
are licensed, not sold. All rights not licensed are reserved.

This software product is governed by the End User License Agreement provided
with the software product.
*/
#ifndef SRC_DCMD_LINUX_FLOW_H_
#define SRC_DCMD_LINUX_FLOW_H_

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

#endif /* SRC_DCMD_LINUX_FLOW_H_ */
