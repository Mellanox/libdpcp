#include <string>

#include "dcmd/dcmd.h"
#include "utils/os.h"

using namespace dcmd;

uar::uar(ctx_handle handle, struct uar_desc* desc)
{
#if defined(HAVE_DEVX)

    struct mlx5dv_devx_uar* devx_uar;

    if (!handle || !desc) {
        throw DCMD_EINVAL;
    }

    devx_uar = mlx5dv_devx_alloc_uar(handle, desc->flags);
    if (NULL == devx_uar) {
        throw DCMD_ENOTSUP;
    }
    m_handle = devx_uar;

#else
    UNUSED(handle);
    UNUSED(desc);
    throw DCMD_ENOTSUP;
#endif /* HAVE_DEVX */
}

uar::~uar()
{
#if defined(HAVE_DEVX)

    if (m_handle) {
        mlx5dv_devx_free_uar(m_handle);
        log_trace("~uar, handle=%p\n", m_handle);
        m_handle = nullptr;
    }
#endif /* HAVE_DEVX */
}

/* The device page id to be used */
uint32_t uar::get_id()
{
#if defined(HAVE_DEVX)
    return m_handle->page_id;
#else
    return 0;
#endif /* HAVE_DEVX */
}

void* uar::get_page()
{
#if defined(HAVE_DEVX)
    return m_handle->base_addr;
#else
    return 0;
#endif /* HAVE_DEVX */
}

/* Used to do doorbell write (The write address of DB/BF) */
void* uar::get_reg()
{
#if defined(HAVE_DEVX)
    return m_handle->reg_addr;
#else
    return 0;
#endif /* HAVE_DEVX */
}
