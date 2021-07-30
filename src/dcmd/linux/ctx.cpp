#include <string>

#include <utils/os.h>
#include "dcmd/dcmd.h"

using namespace dcmd;

ctx::ctx(dev_handle handle)
{
#if defined(HAVE_DEVX)

    struct mlx5dv_context_attr dv_attr;
    struct ibv_context* ibv_ctx;

    memset(&dv_attr, 0, sizeof(dv_attr));
    dv_attr.flags |= MLX5DV_CONTEXT_FLAGS_DEVX;
    ibv_ctx = mlx5dv_open_device(handle, &dv_attr);
    if (NULL == ibv_ctx) {
        throw DCMD_ENOTSUP;
    }
    m_handle = ibv_ctx;

#else
    UNUSED(handle);
    throw DCMD_ENOTSUP;
#endif /* HAVE_DEVX */
}

ctx::~ctx()
{
#if defined(HAVE_DEVX)

    if (m_handle) {
        ibv_close_device(m_handle);
        m_handle = nullptr;
    }
#endif /* HAVE_DEVX */
}

void* ctx::get_context()
{
    return m_handle;
}

int ctx::exec_cmd(const void* in, size_t inlen, void* out, size_t outlen)
{
#if defined(HAVE_DEVX)

    int ret = mlx5dv_devx_general_cmd(m_handle, in, inlen, out, outlen);
    return (ret ? DCMD_EIO : DCMD_EOK);

#else
    UNUSED(in);
    UNUSED(inlen);
    UNUSED(out);
    UNUSED(outlen);
    return DCMD_ENOTSUP;
#endif /* HAVE_DEVX */
}

obj* ctx::create_obj(struct obj_desc* desc)
{

    obj* obj_ptr = nullptr;

    try {
        obj_ptr = new obj(m_handle, desc);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}

uar* ctx::create_uar(struct uar_desc* desc)
{

    uar* obj_ptr = nullptr;

    try {
        obj_ptr = new uar(m_handle, desc);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}

umem* ctx::create_umem(struct umem_desc* desc)
{

    umem* obj_ptr = nullptr;

    try {
        obj_ptr = new umem(m_handle, desc);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}

flow* ctx::create_flow(struct flow_desc* desc)
{

    flow* obj_ptr = nullptr;

    try {
        obj_ptr = new flow(m_handle, desc);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}

int ctx::query_eqn(uint32_t cpu_num, uint32_t& eqn)
{
    int ret = mlx5dv_devx_query_eqn(m_handle, cpu_num, &eqn);
    log_trace("query_eqn: cpuNum: %x eqn: %x ret: %d\n", cpu_num, eqn, ret);
    return (ret ? DCMD_EIO : DCMD_EOK);
}
