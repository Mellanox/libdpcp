#include <string>

#include "dcmd/dcmd.h"

using namespace dcmd;

provider* provider::pinstance = 0;

device** provider::get_device_list(size_t& size)
{
#if defined(HAVE_DEVX)
    if (nullptr == m_dev_array) {
        struct ibv_device** device_list;
        int i, num_devices;
        m_dev_array_size = 0;

        /* get device list using verbs */
        device_list = ibv_get_device_list(&num_devices);
        if (!device_list) {
            goto exit;
        }

        m_dev_array = new (std::nothrow) device*[num_devices];
        if (m_dev_array) {
            /* search for the given device in the device list */
            for (i = 0; i < num_devices; ++i) {
                device* dv = create_device(device_list[i]);
                if (dv) {
                    m_dev_array[m_dev_array_size++] = dv;
                }
            }
        }

        ibv_free_device_list(device_list);
    }

exit:
#endif /* HAVE_DEVX */

    size = m_dev_array_size;

    return m_dev_array;
}

device* provider::create_device(dev_handle handle)
{
    device* obj_ptr = nullptr;
    bool can_be_open = false;
    try {
        obj_ptr = new device(handle);
        // test if device can be opened
        ctx* ctx_obj = obj_ptr->create_ctx();
        if (ctx_obj) {
            can_be_open = true;
            delete ctx_obj;
        }
    } catch (...) {
        if (obj_ptr) {
            delete obj_ptr;
        }
        return nullptr;
    }

    if (can_be_open) {
        return obj_ptr;
    }
    return nullptr;
}
