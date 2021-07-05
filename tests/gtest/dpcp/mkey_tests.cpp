/*
Copyright (C) Mellanox Technologies, Ltd. 2019-2020. ALL RIGHTS RESERVED.

This software product is a proprietary product of Mellanox Technologies, Ltd.
(the "Company") and all right, title, and interest in and to the software
product, including all associated intellectual property rights, are and shall
remain exclusively with the Company. All rights in or to the software product
are licensed, not sold. All rights not licensed are reserved.

This software product is governed by the End User License Agreement provided
with the software product.
*/

#include "common/def.h"
#include "common/log.h"
#include "common/sys.h"
#include "common/base.h"

#include "dpcp_base.h"

using namespace dpcp;

class dpcp_mkey : /*public obj,*/ public dpcp_base {
protected:
    //	SetUp() {}
};

/**
 * @test dpcp_mkey.ti_dm01_Constructor
 * @brief
 *    Check direct-mkey constructor
 * @details
 */
TEST_F(dpcp_mkey, ti_dm01_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    uint32_t length = 4096;
    uint8_t* buf = new (std::nothrow) uint8_t[length];
    ASSERT_NE(nullptr, buf);

    direct_mkey mk(ad, buf, length, (mkey_flags)0);

    void* addr;
    status ret = mk.get_address(addr);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(buf, addr);

    uint32_t len;
    ret = mk.get_length(len);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(len, length);

    uint32_t id = 0;
    ret = mk.get_id(id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(0, id);

    int32_t num;
    ret = mk.get_mkey_num(num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(0, num);

    delete[] buf;
    delete ad;
}
/**
 * @test dpcp_mkey.ti_dm02_get_address
 * @brief
 *    Check direct_mkey::get_address method
 * @details
 *
 */
TEST_F(dpcp_mkey, ti_dm02_get_address)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    void* buf = nullptr;
    direct_mkey mk(ad, buf, 0, (mkey_flags)0);

    void* addr;
    status ret = mk.get_address(addr);
    ASSERT_EQ(DPCP_ERR_NO_MEMORY, ret);
    ASSERT_EQ(nullptr, addr);
    ASSERT_EQ(buf, addr);

    delete ad;
}
/**
 * @test dpcp_mkey.ti_dm03_get_length
 * @brief
 *    Check direct_mkey::get_length method
 * @details
 *
 */
TEST_F(dpcp_mkey, ti_dm03_get_length)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    void* buf = nullptr;
    uint32_t length = 0;
    direct_mkey mk(ad, buf, length, (mkey_flags)0);

    uint32_t len;
    status ret = mk.get_length(len);
    ASSERT_EQ(DPCP_ERR_OUT_OF_RANGE, ret);
    ASSERT_EQ(0, len);
    ASSERT_EQ(len, length);

    delete ad;
}

/**
 * @test dpcp_mkey.ti_dm04_reg_umem
 * @brief
 *    Check direct_mkey::reg_umem method
 * @details
 *
 */
TEST_F(dpcp_mkey, ti_dm04_reg_mem)
{
    status ret;
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t length = 4096;
    uint8_t* buf = new (std::nothrow) uint8_t[length];
    ASSERT_NE(nullptr, buf);

    direct_mkey mk(ad, buf, length, (mkey_flags)0);

    void* pd;
    ret = ad->get_ibv_pd(pd);
    ASSERT_EQ(DPCP_OK, ret);

    ret = mk.reg_mem(pd);
    ASSERT_EQ(DPCP_OK, ret);

    delete[] buf;
    delete ad;
}
/**
 * @test dpcp_mkey.ti_dm05_create
 * @brief
 *    Check direct_mkey::reg_umem method
 * @details
 *
 */
TEST_F(dpcp_mkey, ti_dm05_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    uint32_t length = 4096;
    uint8_t* buf = new (std::nothrow) uint8_t[length];
    ASSERT_NE(nullptr, buf);

    direct_mkey mk(ad, buf, length, (mkey_flags)0);

    void* pd;
    ret = ad->get_ibv_pd(pd);
    ASSERT_EQ(DPCP_OK, ret);

    ret = mk.reg_mem(pd);
    ASSERT_EQ(DPCP_OK, ret);

    // check mkey numbers
    int32_t old_num;
    ret = mk.get_mkey_num(old_num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(0, old_num);

    // check old mkey id
    uint32_t old_id;
    ret = mk.get_id(old_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(0, old_num);

    // create
    ret = mk.create();
    ASSERT_EQ(DPCP_OK, ret);

    // check new mkey id
    uint32_t new_id;
    ret = mk.get_id(new_id);
    ASSERT_EQ(DPCP_OK, ret);
//    ASSERT_NE(new_id, old_id);

    // check mkey numbers
    int32_t new_num;
    ret = mk.get_mkey_num(new_num);
    ASSERT_EQ(DPCP_OK, ret);
//    ASSERT_NE(old_num, new_num);
//    ASSERT_EQ(1, new_num);

    log_trace("mkey_nums old: %d new: %d ids old: 0x%x new: 0x%x\n", old_num, new_num, old_id,
              new_id);

    delete[] buf;
    delete ad;
}

#ifdef GPU_DIRECT

#include <cuda.h>
#include <cuda_runtime_api.h>

void* allocate_buffer_gpu(const size_t size)
{
    uint64_t addr;
    void* ptr;

    CUdeviceptr buffer;

    /* These settings are good for 1 GPU */
    int whichDevice, count = 0, value = 0xFF;
    CUdevice dev;
    CUcontext dev_ctx;

    cuInit(0);
    int dev_id = 1; // get_gpu_direct();
    cuDeviceGet(&dev, dev_id);
    cuDevicePrimaryCtxRetain(&dev_ctx, dev);
    cuCtxSetCurrent(dev_ctx);

    CUdeviceptr d_A;
    cuMemAlloc(&d_A, size);

    ptr = reinterpret_cast<void*>(d_A);

    return ptr;
}

TEST_F(dpcp_mkey, ti_dm06_reg_GPU_mem)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    const size_t size = 4096 * 56320;
    uint8_t* buf = (uint8_t*)allocate_buffer_gpu(size);
    ASSERT_NE(nullptr, buf);

    ibv_context* ibv_ctx = (ibv_context*)ad->get_ibv_context();
    ASSERT_NE(nullptr, ibv_ctx);

    struct ibv_pd* pd = ibv_alloc_pd(ibv_ctx);
    ASSERT_NE(nullptr, pd);

    direct_mkey* mk = new direct_mkey(ad, buf, size, (mkey_flags)0);

    uint32_t old_id;
    status ret = mk->get_id(old_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(0, old_id);

    ret = mk->reg_mem(pd);
    ASSERT_EQ(DPCP_OK, ret);

    ret = mk->get_id(old_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE(0, old_id);

    cuMemFree((CUdeviceptr)buf);
    delete mk;
    delete ad;
}
#endif // GPU_DIRECT

void prepare_bbs1(adapter* ad, pattern_mkey_bb mem_bb[1], uint8_t*& dat_buf,
                  const int32_t strides_num)
{
    const int32_t dat_stride_sz = 512;
    const int32_t dat_len = 512;
    int32_t dat_buf_sz = strides_num * dat_stride_sz;
    dat_buf = new (std::nothrow) uint8_t[dat_buf_sz];
    ASSERT_NE(nullptr, dat_buf);

    direct_mkey* dat_mk = nullptr;
    status ret = ad->create_direct_mkey(dat_buf, dat_buf_sz, (mkey_flags)0, dat_mk);
    ASSERT_EQ(DPCP_OK, ret);
    // prepare memory descriptor
    mem_bb[0].m_key = dat_mk;
    mem_bb[0].m_stride_sz = dat_stride_sz;
    mem_bb[0].m_length = dat_len;
}

void release_bbs1(pattern_mkey_bb mem_bb[1], uint8_t* dat_buf)
{
    delete mem_bb[0].m_key;
    delete[] dat_buf;
}

/**
 * @test dpcp_mkey.ti_pm01_create_indirect
 * @brief
 *    Check pattern_mkey::create method with indirect mkey
 * @details
 *
 */
TEST_F(dpcp_mkey, ti_pm01_create_indirect)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    errno = 0;
    const int32_t strides_num = 256;
    pattern_mkey_bb mem_bb[1];
    uint8_t* dat_buf = nullptr;
    prepare_bbs1(ad, mem_bb, dat_buf, strides_num);

    pattern_mkey mk(ad, dat_buf, MKEY_NONE, strides_num, 1, mem_bb);

    // create
    ret = mk.create();
    ASSERT_EQ(DPCP_OK, ret);

    // check new mkey id
    uint32_t new_id;
    ret = mk.get_id(new_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE((uint32_t)0, new_id);

    log_trace("pattern mkey id: 0x%x\n", new_id);

    release_bbs1(mem_bb, dat_buf);

    delete ad;
}

void prepare_bbs2(adapter* ad, pattern_mkey_bb mem_bb[2], uint8_t*& dat_buf, uint8_t*& hdr_buf,
                  const int32_t strides_num)
{
    const int32_t hdr_stride_sz = 16;
    const int32_t hdr_len = 14;
    int32_t hdr_buf_sz = strides_num * hdr_stride_sz;
    hdr_buf = new (std::nothrow) uint8_t[hdr_buf_sz];
    ASSERT_NE(nullptr, hdr_buf);

    direct_mkey* hdr_mk = nullptr;
    status ret = ad->create_direct_mkey(hdr_buf, hdr_buf_sz, (mkey_flags)0, hdr_mk);
    ASSERT_EQ(DPCP_OK, ret);

    const int32_t dat_stride_sz = 512;
    const int32_t dat_len = 512;
    int32_t dat_buf_sz = strides_num * dat_stride_sz;
    dat_buf = new (std::nothrow) uint8_t[dat_buf_sz];
    ASSERT_NE(nullptr, dat_buf);

    direct_mkey* dat_mk = nullptr;
    ret = ad->create_direct_mkey(dat_buf, dat_buf_sz, (mkey_flags)0, dat_mk);
    ASSERT_EQ(DPCP_OK, ret);
    // prepare memory descriptor
    mem_bb[0].m_key = hdr_mk;
    mem_bb[0].m_stride_sz = hdr_stride_sz;
    mem_bb[0].m_length = hdr_len;

    mem_bb[1].m_key = dat_mk;
    mem_bb[1].m_stride_sz = dat_stride_sz;
    mem_bb[1].m_length = dat_len;
}

void release_bbs2(pattern_mkey_bb mem_bb[2], uint8_t* dat_buf, uint8_t* hdr_buf)
{
    delete mem_bb[0].m_key;
    delete mem_bb[1].m_key;
    delete[] dat_buf;
    delete[] hdr_buf;
}

/**
 * @test dpcp_mkey.ti_pm02_Constructor
 * @brief
 *    Check pattern-mkey constructor
 * @details
 */
TEST_F(dpcp_mkey, ti_pm02_Constructor)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    const int32_t strides_num = 256;
    pattern_mkey_bb mem_bb[2];
    uint8_t* hdr_buf = nullptr;
    uint8_t* dat_buf = nullptr;
    prepare_bbs2(ad, mem_bb, dat_buf, hdr_buf, strides_num);

    pattern_mkey mk(ad, hdr_buf, MKEY_NONE, strides_num, 2, mem_bb);

    size_t m_num;
    ret = mk.get_mkeys_num(m_num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(2, m_num);

    ret = mk.get_stride_num(m_num);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(m_num, strides_num);

    size_t stride_sz = 0;
    ret = mk.get_stride_sz(stride_sz);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(stride_sz, mem_bb[0].m_length + mem_bb[1].m_length);

    uint32_t len;
    ret = mk.get_length(len);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_EQ(len, strides_num * stride_sz);

    release_bbs2(mem_bb, dat_buf, hdr_buf);
    delete ad;
}
/**
 * @test dpcp_mkey.ti_pm03_create
 * @brief
 *    Check pattern_mkey::create method
 * @details
 *
 */
TEST_F(dpcp_mkey, ti_pm03_create)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    errno = 0;
    const int32_t strides_num = 256;
    pattern_mkey_bb mem_bb[2];
    uint8_t* hdr_buf = nullptr;
    uint8_t* dat_buf = nullptr;
    prepare_bbs2(ad, mem_bb, dat_buf, hdr_buf, strides_num);

    pattern_mkey mk(ad, hdr_buf, MKEY_NONE, strides_num, 2, mem_bb);

    // create
    ret = mk.create();
    ASSERT_EQ(DPCP_OK, ret);

    // check new mkey id
    uint32_t new_id;
    ret = mk.get_id(new_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE((uint32_t)0, new_id);

    log_trace("pattern mkey id: 0x%x\n", new_id);

    release_bbs2(mem_bb, dat_buf, hdr_buf);
    delete ad;
}

void prepare_bbs3(adapter* ad, pattern_mkey_bb mem_bb[3], uint8_t*& dat_buf, uint8_t*& hdr_buf,
                  uint8_t*& pad_buf, const int32_t strides_num)
{
    const int32_t hdr_stride_sz = 54;
    const int32_t hdr_len = 54;
    int32_t hdr_buf_sz = strides_num * hdr_stride_sz;
    hdr_buf = new (std::nothrow) uint8_t[hdr_buf_sz];
    ASSERT_NE(nullptr, hdr_buf);

    direct_mkey* hdr_mk = nullptr;
    status ret = ad->create_direct_mkey(hdr_buf, hdr_buf_sz, (mkey_flags)0, hdr_mk);
    ASSERT_EQ(DPCP_OK, ret);

    const int32_t dat_stride_sz = 1220;
    const int32_t dat_len = 1220;
    int32_t dat_buf_sz = strides_num * dat_stride_sz;
    dat_buf = new (std::nothrow) uint8_t[dat_buf_sz];
    ASSERT_NE(nullptr, dat_buf);

    direct_mkey* dat_mk = nullptr;
    ret = ad->create_direct_mkey(dat_buf, dat_buf_sz, (mkey_flags)0, dat_mk);
    ASSERT_EQ(DPCP_OK, ret);

    const int32_t pad_stride_sz = 774;
    const int32_t pad_len = 774;
    int32_t pad_buf_sz = strides_num * pad_stride_sz;
    pad_buf = new (std::nothrow) uint8_t[pad_buf_sz];
    ASSERT_NE(nullptr, pad_buf);

    reserved_mkey* pad_mk = nullptr;
    ret = ad->create_reserved_mkey(MKEY_RESERVED_DUMP_AND_FILL, pad_buf, pad_buf_sz, (mkey_flags)0,
                                   pad_mk);
    ASSERT_EQ(DPCP_OK, ret);
    // prepare memory descriptor
    mem_bb[0].m_key = hdr_mk;
    mem_bb[0].m_stride_sz = hdr_stride_sz;
    mem_bb[0].m_length = hdr_len;

    mem_bb[1].m_key = dat_mk;
    mem_bb[1].m_stride_sz = dat_stride_sz;
    mem_bb[1].m_length = dat_len;

    mem_bb[2].m_key = pad_mk;
    mem_bb[2].m_stride_sz = pad_stride_sz;
    mem_bb[2].m_length = pad_len;
}

void release_bbs3(pattern_mkey_bb mem_bb[3], uint8_t* dat_buf, uint8_t* hdr_buf, uint8_t* pad_buf)
{
    delete mem_bb[0].m_key;
    delete mem_bb[1].m_key;
    delete mem_bb[2].m_key;
    delete[] pad_buf;
    delete[] dat_buf;
    delete[] hdr_buf;
}
/**
 * @test dpcp_mkey.ti_pm04_create_pad
 * @brief
 *    Check pattern_mkey::create method with padding template
 * @details
 *
 */
TEST_F(dpcp_mkey, ti_pm04_create_pad)
{
    adapter* ad = OpenAdapter();
    ASSERT_NE(nullptr, ad);

    status ret = ad->open();
    ASSERT_EQ(DPCP_OK, ret);

    errno = 0;
    const int32_t strides_num = 256;
    pattern_mkey_bb mem_bb[3];
    uint8_t* hdr_buf = nullptr;
    uint8_t* dat_buf = nullptr;
    uint8_t* pad_buf = nullptr;
    prepare_bbs3(ad, mem_bb, dat_buf, hdr_buf, pad_buf, strides_num);

    pattern_mkey mk(ad, hdr_buf, MKEY_NONE, strides_num, 3, mem_bb);

    // create
    ret = mk.create();
    ASSERT_EQ(DPCP_OK, ret);

    // check new mkey id
    uint32_t new_id;
    ret = mk.get_id(new_id);
    ASSERT_EQ(DPCP_OK, ret);
    ASSERT_NE((uint32_t)0, new_id);

    log_trace("pattern mkey id: 0x%x\n", new_id);

    release_bbs3(mem_bb, dat_buf, hdr_buf, pad_buf);
    delete ad;
}
