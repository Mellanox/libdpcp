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

#ifndef TESTS_GTEST_COMMON_BASE_H_
#define TESTS_GTEST_COMMON_BASE_H_

/**
 * Base class for tests
 */
class test_base {
public:
protected:
    test_base();
    virtual ~test_base();

protected:
    virtual void cleanup();
    virtual void init();
    bool barrier();

private:
    static void* thread_func(void* arg);
#if defined(__linux__)
    pthread_barrier_t m_barrier;
#endif
};

#endif /* TESTS_GTEST_COMMON_BASE_H_ */
