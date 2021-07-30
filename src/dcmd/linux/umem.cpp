#include <string>

#include "dcmd/dcmd.h"
#include "utils/os.h"

using namespace dcmd;

umem::umem(ctx_handle handle, struct umem_desc* desc)
{
#if defined(HAVE_DEVX)

    struct mlx5dv_devx_umem* devx_umem;

    if (!handle || !desc) {
        throw DCMD_EINVAL;
    }

    devx_umem = mlx5dv_devx_umem_reg(handle, desc->addr, desc->size, desc->access);
    if (NULL == devx_umem) {
        throw DCMD_ENOTSUP;
    }
    m_handle = devx_umem;

#else
    UNUSED(handle);
    UNUSED(desc);
    throw DCMD_ENOTSUP;
#endif /* HAVE_DEVX */
}

umem::~umem()
{
#if defined(HAVE_DEVX)

    if (m_handle) {
        int ret = mlx5dv_devx_umem_dereg(m_handle);
        if (ret) {
            log_trace("~umem: dereg ret: %d errno: %d\n", ret, errno);
        }
        m_handle = nullptr;
    }
#endif /* HAVE_DEVX */
}

uint32_t umem::get_id()
{
#if defined(HAVE_DEVX)
    return m_handle->umem_id;
#else
    return 0;
#endif /* HAVE_DEVX */
}
