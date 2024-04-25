/*
 * SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
 * Copyright (c) 2019-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
    gtest_conf.adapter[0] = '\0';
    return rc;
}

static int _set_config(int argc, char** argv)
{
    int rc = 0;
    static struct option long_options[] = {{"debug", required_argument, 0, 'd'},
                                           {"adapter", required_argument, 0, 'a'},
                                           {"help", no_argument, 0, 'h'},
                                           {NULL, no_argument, 0, 0}};
    int op;
    int option_index;

    while ((op = getopt_long(argc, argv, "d:a:h", long_options, &option_index)) != -1) {
        switch (op) {
        case 'd':
            errno = 0;
            gtest_conf.log_level = strtol(optarg, NULL, 0);
            if (0 != errno) {
                rc = -EINVAL;
                log_error("Invalid option value <%s>\n", optarg);
            }
            break;
        case 'a':
            if (optarg) {
                strncpy(gtest_conf.adapter, optarg, sizeof(gtest_conf.adapter) - 1);
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
        log_info("adapter: %s\n", gtest_conf.adapter);
    }

    return rc;
}

static void _usage(void)
{
    printf("Usage: gtest [options]\n"
           "\t--random,-s <count>     Seed (default %d).\n"
           "\t--debug,-d <level>      Output verbose level (default: %d).\n"
           "\t--adapter,-a <name>     Adapter name.\n"
           "\t--help,-h               Print help and exit\n",

           gtest_conf.random_seed, gtest_conf.log_level);
    exit(0);
}
