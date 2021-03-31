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
#ifndef SRC_DCMD_LINUX_CTX_H_
#define SRC_DCMD_LINUX_CTX_H_

#include "dcmd/base/base_ctx.h"

namespace dcmd {

class ctx : public base_ctx {
public:
    ctx()
    {
        m_handle = nullptr;
    }
    ctx(dev_handle handle);
    virtual ~ctx();

    void* get_context();
    int exec_cmd(const void* in, size_t inlen, void* out, size_t outlen);
    obj* create_obj(struct obj_desc* desc);
    uar* create_uar(struct uar_desc* desc);
    umem* create_umem(struct umem_desc* desc);
    flow* create_flow(struct flow_desc* desc);
    int query_eqn(uint32_t cpu_num, uint32_t& eqn);
    int hca_iseg_mapping(void*& pv_iseg);

private:
    ctx_handle m_handle;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_LINUX_CTX_H_ */
