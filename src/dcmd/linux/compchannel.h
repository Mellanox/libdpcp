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

#ifndef SRC_DCMD_LINUX_COMPCHANNEL_H_
#define SRC_DCMD_LINUX_COMPCHANNEL_H_

namespace dcmd {

struct compchannel_ctx {
    LPOVERLAPPED overlapped;
    uint32_t eqe_nums;
};

class compchannel {
private:
    ctx_handle m_ctx;
    cq_handle m_cq_obj;
    comp_channel m_event_channel;
    bool m_binded;
    bool m_solicited;

public:
    compchannel()
    {
        m_ctx = nullptr;
        m_binded = false;
    }
    compchannel(ctx_handle handle);
    virtual ~compchannel();

    int bind(cq_handle cq_obj, bool solicited_only);
    int unbind();
    int get_comp_channel(event_channel*& ch);
    int request(compchannel_ctx& cc_ctx);
    int query(void*& cq_ctx);
    void flush(uint32_t n_events);
};

} /* namespace dcmd */

#endif /* SRC_DCMD_LINUX_COMPCHANNEL_H_ */
