#include <string>

#include "dcmd/dcmd.h"

using namespace dcmd;

device::device(dev_handle handle)
{
    m_handle = handle;
    m_id = std::string(handle->name);
    m_name = std::string(handle->name);
    memset(&m_device_attr, 0, sizeof(m_device_attr));
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
