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

#ifndef SRC_DCMD_BASE_OBJ_H_
#define SRC_DCMD_BASE_OBJ_H_

namespace dcmd {

class base_obj {
public:
    base_obj()
    {
    }
    virtual ~base_obj()
    {
    }

    virtual int query(struct obj_desc* desc) = 0;
    virtual int modify(struct obj_desc* desc) = 0;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_BASE_OBJ_H_ */
