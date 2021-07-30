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
#ifndef SRC_DCMD_BASE_CTX_H_
#define SRC_DCMD_BASE_CTX_H_

namespace dcmd {

class obj;
class uar;
class umem;
class flow;

class base_ctx {
public:
    base_ctx()
    {
    }
    virtual ~base_ctx()
    {
    }

    virtual int exec_cmd(const void* in, size_t inlen, void* out, size_t outlen) = 0;
    virtual obj* create_obj(struct obj_desc* desc) = 0;
    virtual uar* create_uar(struct uar_desc* desc) = 0;
    virtual umem* create_umem(struct umem_desc* desc) = 0;
    virtual flow* create_flow(struct flow_desc* desc) = 0;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_BASE_CTX_H_ */
