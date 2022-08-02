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

#ifndef SRC_DCMD_BASE_ACTION_H_
#define SRC_DCMD_BASE_ACTION_H_

#include <vector>

namespace dcmd {

// Forward declarations
struct flow_desc;
struct fwd_dst_desc;

/**
 * @brief base_action - Action interface class.
 *
 * @note: All Action types should inherit this interface.
 */
class base_action {
public:
    base_action() = default;
    virtual ~base_action() = default;
    /**
     * @brief apply - Apply the Action to the Flow description structure.
     *
     * @param [out] desc: Flow description.
     *
     * @retval Returns DCMD_EOK for success.
     */
    virtual int apply(struct flow_desc& desc) const = 0;
};

/**
 * @brief base_action_fwd - Action forward abstract class.
 *
 * @note: All Action forward ,OS concrete, should inherit from this class.
 */
class base_action_fwd : public base_action {
public:
    base_action_fwd(const std::vector<fwd_dst_desc>& dests)
        : m_dests(dests)
    {
    }
    virtual ~base_action_fwd() = default;
    virtual int apply(struct flow_desc& desc) const = 0;

protected:
    std::vector<fwd_dst_desc> m_dests;
};

} /* namespace dcmd */

#endif /* SRC_DCMD_BASE_ACTION_H_ */
