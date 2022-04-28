/*
Copyright (C) Mellanox Technologies, Ltd. 2020-2021. ALL RIGHTS RESERVED.

This software product is a proprietary product of Mellanox Technologies, Ltd.
(the "Company") and all right, title, and interest in and to the software
product, including all associated intellectual property rights, are and shall
remain exclusively with the Company. All rights in or to the software product
are licensed, not sold. All rights not licensed are reserved.

This software product is governed by the End User License Agreement provided
with the software product.
*/

#ifndef SRC_DCMD_WINDOWS_COMPCHANNEL_H_
#define SRC_DCMD_WINDOWS_COMPCHANNEL_H_

namespace dcmd {

struct compchannel_ctx {
    LPOVERLAPPED overlapped;
    uint32_t eqe_nums;
};

class compchannel {
private:
    OVERLAPPED m_overlapped_ctx;
    ctx_handle m_ctx;
    obj_handle m_cq_obj;
    event_channel m_handle;
    bool m_binded;
    bool m_adapter_shutdown;

public:
    compchannel()
    {
        m_ctx = nullptr;
        m_binded = false;
    }
    compchannel(ctx_handle handle);
    virtual ~compchannel();

    int bind(obj_handle src_obj, bool unused);
    int unbind();
    int get_comp_channel(event_channel*& ch);
    int request(compchannel_ctx& cc_ctx);
    void flush(uint32_t unused);
};

} /* namespace dcmd */

#endif /* SRC_DCMD_WINDOWS_COMPCHANNEL_H_ */
