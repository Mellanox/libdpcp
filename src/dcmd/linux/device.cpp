#include <string>

#include "dcmd/dcmd.h"

using namespace dcmd;

device::device(dev_handle handle)
{
#if defined(HAVE_DEVX)

    m_handle = handle;
    m_id = std::string(handle->name);
    m_name = std::string(handle->name);

#else
    UNUSED(handle);
    throw DCMD_ENOTSUP;
#endif /* HAVE_DEVX */
}

std::string device::get_name()
{
    return m_name;
}

ctx* device::create_ctx()
{

    ctx* obj_ptr = nullptr;

    try {
        obj_ptr = new ctx(m_handle);
    } catch (...) {
        return nullptr;
    }

    return obj_ptr;
}
