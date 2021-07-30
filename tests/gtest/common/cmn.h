/*
Copyright (C) Mellanox Technologies, Ltd. 2020. ALL RIGHTS RESERVED.

This software product is a proprietary product of Mellanox Technologies, Ltd.
(the "Company") and all right, title, and interest in and to the software
product, including all associated intellectual property rights, are and shall
remain exclusively with the Company. All rights in or to the software product
are licensed, not sold. All rights not licensed are reserved.

This software product is governed by the End User License Agreement provided
with the software product.
*/

#ifndef TESTS_GTEST_COMMON_CMN_H_
#define TESTS_GTEST_COMMON_CMN_H_

#include <stdexcept>
#include <sstream>
#include <string>

namespace cmn {

class test_skip_exception : public std::exception {
public:
    test_skip_exception(const std::string& reason = "")
        : m_reason(reason)
    {
    }
    virtual ~test_skip_exception() throw()
    {
    }

    virtual const char* what() const throw()
    {
        return (std::string("[  SKIPPED ] ") + m_reason).c_str();
    }

private:
    const std::string m_reason;
};

#define SKIP_TRUE(_expr, _reason)                                                                  \
    if (!(_expr)) {                                                                                \
        throw cmn::test_skip_exception(_reason);                                                   \
    }

} // namespace cmn

#endif /* TESTS_GTEST_COMMON_CMN_H_ */
