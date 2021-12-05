#include <string>
#include <errno.h>
#include "utils/os.h"
#include "dcmd/dcmd.h"

using namespace dcmd;

obj::obj(ctx_handle handle, struct obj_desc* desc)
{
#if defined(HAVE_DEVX)
    struct mlx5dv_devx_obj* devx_ctx;

    if (!handle || !desc) {
        throw DCMD_EINVAL;
    }

    devx_ctx = mlx5dv_devx_obj_create(handle, desc->in, desc->inlen, desc->out, desc->outlen);
    log_trace("obj(%p) handle: %p in: %p in_sz: %ld out: %p, out_sz: %ld errno=%d\n", devx_ctx,
              handle, desc->in, desc->inlen, desc->out, desc->outlen, errno);
    if (NULL == devx_ctx) {
        throw DCMD_ENOTSUP;
    }
    m_handle = devx_ctx;

#else
    UNUSED(handle);
    UNUSED(desc);
    throw DCMD_ENOTSUP;
#endif /* HAVE_DEVX */
}

int obj::destroy()
{
    int ret = DCMD_EOK;

#if defined(HAVE_DEVX)
    if (m_handle) {
        ret = mlx5dv_devx_obj_destroy(m_handle);
        log_trace("obj::destroy(%p) ret=%d errno=%d\n", m_handle, ret, errno);
        m_handle = nullptr;
    }
#endif /* HAVE_DEVX */

    return ret;
}

obj::~obj()
{
    destroy();
}

int obj::query(struct obj_desc* desc)
{

    if (!desc) {
        return DCMD_EINVAL;
    }

#if defined(HAVE_DEVX)
    int ret = mlx5dv_devx_obj_query(m_handle, desc->in, desc->inlen, desc->out, desc->outlen);
    log_trace("obj::query(%p) in: %p in_sz: %ld out: %p, out_sz: %ld errno=%d\n", m_handle,
              desc->in, desc->inlen, desc->out, desc->outlen, errno);
    return (ret ? DCMD_EIO : DCMD_EOK);
#else
    return DCMD_ENOTSUP;
#endif /* HAVE_DEVX */
}

int obj::modify(struct obj_desc* desc)
{

    if (!desc) {
        return DCMD_EINVAL;
    }

#if defined(HAVE_DEVX)
    int ret = mlx5dv_devx_obj_modify(m_handle, desc->in, desc->inlen, desc->out, desc->outlen);
    log_trace("obj::modify(%p) in: %p in_sz: %ld out: %p, out_sz: %ld errno=%d\n", m_handle,
              desc->in, desc->inlen, desc->out, desc->outlen, errno);
    return (ret ? DCMD_EIO : DCMD_EOK);
#else
    return DCMD_ENOTSUP;
#endif /* HAVE_DEVX */
}
