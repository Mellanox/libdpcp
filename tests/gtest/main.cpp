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

#include <getopt.h>

#include "common/gtest.h"
#include "common/tap.h"
#include "common/def.h"
#include "common/log.h"
#include "common/sys.h"

static int _set_config(int argc, char** argv);
static int _def_config(void);
static void _usage(void);

struct gtest_configure_t gtest_conf;

int main(int argc, char** argv)
{
    // coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
    ::testing::InitGoogleTest((int*)&argc, argv);

    char* str = getenv("GTEST_TAP");
    /* Append TAP Listener */
    if (str) {
        if (0 < strtol(str, NULL, 0)) {
            testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
            if (1 == strtol(str, NULL, 0)) {
                delete listeners.Release(listeners.default_result_printer());
            }
            listeners.Append(new tap::TapListener());
        }
    }

    _def_config();
    _set_config(argc, (char**)argv);

    return RUN_ALL_TESTS();
}

static int _def_config(void)
{
    int rc = 0;

    memset(&gtest_conf, 0, sizeof(gtest_conf));
    gtest_conf.log_level = 4;
    gtest_conf.random_seed = time(NULL) % 32768;
    return rc;
}

static int _set_config(int argc, char** argv)
{
    int rc = 0;
    static struct option long_options[] = {{"debug", required_argument, 0, 'd'},
                                           {"help", no_argument, 0, 'h'},
                                           {NULL, no_argument, 0, 0}};
    int op;
    int option_index;

    while ((op = getopt_long(argc, argv, "d:h", long_options, &option_index)) != -1) {
        switch (op) {
        case 'd':
            errno = 0;
            gtest_conf.log_level = strtol(optarg, NULL, 0);
            if (0 != errno) {
                rc = -EINVAL;
                log_error("Invalid option value <%s>\n", optarg);
            }
            break;
        case 'h':
            _usage();
            break;
        default:
            rc = -EINVAL;
            log_error("Unknown option <%c>\n", op);
            break;
        }
    }

    if (0 != rc) {
        _usage();
    } else {
        srand(gtest_conf.random_seed);
        log_info("CONFIGURATION:\n");
        log_info("log level: %d\n", gtest_conf.log_level);
        log_info("seed: %d\n", gtest_conf.random_seed);
    }

    return rc;
}

static void _usage(void)
{
    printf("Usage: gtest [options]\n"
           "\t--random,-s <count>     Seed (default %d).\n"
           "\t--debug,-d <level>      Output verbose level (default: %d).\n"
           "\t--help,-h               Print help and exit\n",

           gtest_conf.random_seed, gtest_conf.log_level);
    exit(0);
}
