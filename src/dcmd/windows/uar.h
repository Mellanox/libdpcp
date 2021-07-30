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

#ifndef SRC_DCMD_WINDOWS_UAR_H_
#define SRC_DCMD_WINDOWS_UAR_H_

#include "dcmd/base/base_uar.h"

namespace dcmd {

class uar : public base_uar {
public:
    uar()
    {
        m_handle = nullptr;
    }
    uar(ctx_handle handle, struct uar_desc* desc);
    virtual ~uar();

    uint32_t get_id();
    void* get_page();
    void* get_reg();

private:
    uar_handle m_handle;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_WINDOWS_UAR_H_ */
