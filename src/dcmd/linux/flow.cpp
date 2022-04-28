#include <string>

#include <utils/os.h>
#include "dcmd/dcmd.h"

using namespace dcmd;

flow::flow(ctx_handle handle, struct flow_desc* desc)
{
#if defined(HAVE_DEVX)

    struct ibv_flow* ib_flow;
    struct mlx5dv_flow_matcher* matcher = NULL;
    struct mlx5dv_flow_matcher_attr matcher_attr;

    memset(&matcher_attr, 0, sizeof(matcher_attr));
    matcher_attr.type = IBV_FLOW_ATTR_NORMAL;
    matcher_attr.flags = 0;
    matcher_attr.priority = desc->priority;
    // Only outer header for now!!!
    matcher_attr.match_criteria_enable = 1
        << MLX5_CREATE_FLOW_GROUP_IN_MATCH_CRITERIA_ENABLE_OUTER_HEADERS;
    matcher_attr.match_mask = (struct mlx5dv_flow_match_parameters*)desc->match_criteria;
    matcher_attr.comp_mask = MLX5DV_FLOW_MATCHER_MASK_FT_TYPE;
    matcher_attr.ft_type = MLX5_IB_UAPI_FLOW_TABLE_TYPE_NIC_RX;

    matcher = mlx5dv_create_flow_matcher(handle, &matcher_attr);
    if (NULL == matcher) {
        throw DCMD_ENOTSUP;
    }

    size_t num_actions = (desc->flow_id ? (desc->num_dst_tir + 1) : desc->num_dst_tir);
    num_actions += desc->modify_actions ? 1 : 0;
    struct mlx5dv_flow_action_attr actions_attr[num_actions];
    int i = 0;
    int j = 0;

    if (desc->flow_id) {
        actions_attr[i].type = MLX5DV_FLOW_ACTION_TAG;
        actions_attr[i].tag_value = desc->flow_id;
        i++;
    }
    if (desc->modify_actions) {
        actions_attr[i].type = MLX5DV_FLOW_ACTION_IBV_FLOW_ACTION;
        actions_attr[i].action = mlx5dv_create_flow_action_modify_header(
            handle,
            sizeof(modify_action) * desc->num_of_actions,
            (uint64_t *)desc->modify_actions,
            MLX5_IB_UAPI_FLOW_TABLE_TYPE_NIC_RX);
        if (!actions_attr[i].action) {
            throw DCMD_ENOTSUP;
        }
        i++;
    }
    for (j = 0; j < (int)desc->num_dst_tir; j++, i++) {
        actions_attr[i].type = MLX5DV_FLOW_ACTION_DEST_DEVX;
        actions_attr[i].obj = desc->dst_tir_obj[j];
    }

    ib_flow = mlx5dv_create_flow(matcher, (struct mlx5dv_flow_match_parameters*)desc->match_value,
                                 num_actions, actions_attr);
    if (NULL == ib_flow) {
        mlx5dv_destroy_flow_matcher(matcher);
        throw DCMD_ENOTSUP;
    }
    m_matcher = matcher;
    m_handle = ib_flow;

#else
    UNUSED(handle);
    UNUSED(desc);
    throw DCMD_ENOTSUP;
#endif /* HAVE_DEVX */
}

flow::~flow()
{
#if defined(HAVE_DEVX)

    if (m_handle) {
        ibv_destroy_flow(m_handle);
        m_handle = nullptr;
        mlx5dv_destroy_flow_matcher(m_matcher);
        m_matcher = nullptr;
    }
#endif /* HAVE_DEVX */
}
