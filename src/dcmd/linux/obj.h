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
#ifndef SRC_DCMD_LINUX_OBJ_H_
#define SRC_DCMD_LINUX_OBJ_H_

#include "dcmd/base/base_obj.h"

namespace dcmd {

class obj : public base_obj {
public:
    obj()
    {
        m_handle = nullptr;
    }
    obj(ctx_handle handle, struct obj_desc* desc);
    virtual ~obj();

    int query(struct obj_desc* desc);
    int modify(struct obj_desc* desc);

    uintptr_t get_handle()
    {
        return (uintptr_t)m_handle;
    }

    int destroy();

private:
    obj_handle m_handle;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_LINUX_OBJ_H_ */