#!/bin/bash -El
#
# Testing script, to run from Jenkins CI
#
# Copyright (c) 2019-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# BSD-3-Clause
#
# See file LICENSE for terms.
#
#
# Environment variables set by Jenkins CI:
#  - WORKSPACE         : path to working directory
#  - BUILD_NUMBER      : jenkins build number
#  - JOB_URL           : jenkins job url
#  - JENKINS_RUN_TESTS : whether to run unit tests
#  - TARGET            : target configuration
#

echo "======================================================"
echo
echo "# starting on host --------->  $(hostname) "
echo "# arguments called with ---->  ${@}        "
echo "# path to me --------------->  ${0}        "
echo "# parent path -------------->  ${0%/*}     "
echo "# name --------------------->  ${0##*/}    "
echo

PATH=${PATH}:/hpc/local/bin:/hpc/local/oss/vma/
MODULEPATH=${MODULEPATH}:/hpc/local/etc/modulefiles
env
for f in autoconf automake libtool ; do $f --version | head -1 ; done
echo "======================================================"

source $(dirname $0)/jenkins_tests/globals.sh

set -xe

rel_path=$(dirname $0)
abs_path=$(readlink -f $rel_path)

# Values: none, fail, always
#
jenkins_opt_artifacts=${jenkins_opt_artifacts:="always"}

# Values: 0..N test (max 100)
#
jenkins_opt_exit=${jenkins_opt_exit:="10"}

# Test scenario list
#
jenkins_test_build=${jenkins_test_build:="yes"}

jenkins_test_compiler=${jenkins_test_compiler:="yes"}
jenkins_test_rpm=${jenkins_test_rpm:="yes"}
jenkins_test_copyrights=${jenkins_test_copyrights:="no"}
jenkins_test_cov=${jenkins_test_cov:="yes"}
jenkins_test_cppcheck=${jenkins_test_cppcheck:="yes"}
jenkins_test_csbuild=${jenkins_test_csbuild:="yes"}
jenkins_test_gtest=${jenkins_test_gtest:="yes"}
jenkins_test_style=${jenkins_test_style:="yes"}


echo Starting on host: $(hostname)
if [ "${name}" == "" ]; then
    name=$(hostname -s)
fi

cd $WORKSPACE

rm -rf ${WORKSPACE}/${prefix}

for target_v in "${target_list[@]}"; do
    ret=0
    IFS=':' read target_name target_option <<< "$target_v"

    export jenkins_test_artifacts="${WORKSPACE}/${prefix}/dpcp-${BUILD_NUMBER}-${name}-${target_name}"
    export jenkins_test_custom_configure="${target_option}"
    export jenkins_target="${target_name}"
    set +x
    echo "======================================================"
    echo "Jenkins is checking for [${target_name}] target ..."
    echo "======================================================"
    set -x

    set +e
    # check building and ignore others in case failure
    #
    if [ "$jenkins_test_build" = "yes" ]; then
        do_check_env
        rm -f autom4te.cache
        ./autogen.sh -s
        $WORKSPACE/contrib/jenkins_tests/build.sh
        ret=$?
        if [ $ret -gt 0 ]; then
           do_err "case: [build: ret=$ret]"
           jenkins_opt_exit=1
        fi
        rc=$((rc + $ret))
    fi

    # check other units w/o forcing exiting
    #
    if [ 1 -lt "$jenkins_opt_exit" -o "$rc" -eq 0 ]; then
	    if [ "$jenkins_test_compiler" = "yes" ]; then
	        do_check_env
	        $WORKSPACE/contrib/jenkins_tests/compiler.sh
	        ret=$?
	        if [ $ret -gt 0 ]; then
	           do_err "case: [compiler: ret=$ret]"
	        fi
	        rc=$((rc + $ret))
	    fi
    fi
    if [ 2 -lt "$jenkins_opt_exit" -o "$rc" -eq 0 ]; then
	    if [ "$jenkins_test_rpm" = "yes" ]; then
	        do_check_env
	        $WORKSPACE/contrib/jenkins_tests/rpm.sh
	        ret=$?
	        if [ $ret -gt 0 ]; then
	           do_err "case: [rpm: ret=$ret]"
	        fi
	        rc=$((rc + $ret))
	    fi
	fi
    if [ 3 -lt "$jenkins_opt_exit" -o "$rc" -eq 0 ]; then
	    if [ "$jenkins_test_cov" = "yes" ]; then
	        do_check_env
	        $WORKSPACE/contrib/jenkins_tests/cov.sh
	        ret=$?
	        if [ $ret -gt 0 ]; then
	           do_err "case: [cov: ret=$ret]"
	        fi
	        rc=$((rc + $ret))
	    fi
    fi
    if [ 4 -lt "$jenkins_opt_exit" -o "$rc" -eq 0 ]; then
	    if [ "$jenkins_test_cppcheck" = "yes" ]; then
	        do_check_env
	        $WORKSPACE/contrib/jenkins_tests/cppcheck.sh
	        ret=$?
	        if [ $ret -gt 0 ]; then
	           do_err "case: [cppcheck: ret=$ret]"
	        fi
	        rc=$((rc + $ret))
	    fi
    fi
    if [ 5 -lt "$jenkins_opt_exit" -o "$rc" -eq 0 ]; then
	    if [ "$jenkins_test_csbuild" = "yes" ]; then
	        do_check_env
	        $WORKSPACE/contrib/jenkins_tests/csbuild.sh
	        ret=$?
	        if [ $ret -gt 0 ]; then
	           do_err "case: [csbuild: ret=$ret]"
	        fi
	        rc=$((rc + $ret))
	    fi
    fi
    if [ 6 -lt "$jenkins_opt_exit" -o "$rc" -eq 0 ]; then
	    if [ "$jenkins_test_gtest" = "yes" ]; then
	        do_check_env
	        $WORKSPACE/contrib/jenkins_tests/gtest.sh
	        ret=$?
	        if [ $ret -gt 0 ]; then
	           do_err "case: [gtest: ret=$ret]"
	        fi
	        rc=$((rc + $ret))
	    fi
    fi
    if [ 7 -lt "$jenkins_opt_exit" -o "$rc" -eq 0 ]; then
	    if [ "$jenkins_test_style" = "yes" ]; then
	        $WORKSPACE/contrib/jenkins_tests/style.sh
	        ret=$?
	        if [ $ret -gt 0 ]; then
	           do_err "case: [style: ret=$ret]"
	        fi
	        rc=$((rc + $ret))
	    fi
    fi
    if [ 8 -lt "$jenkins_opt_exit" -o "$rc" -eq 0 ]; then
	    if [ "$jenkins_test_copyrights" = "yes" ]; then
	        $WORKSPACE/contrib/jenkins_tests/copyrights.sh
	        ret=$?
	        if [ $ret -gt 0 ]; then
	           do_err "case: [copyrights: ret=$ret]"
	        fi
	        rc=$((rc + $ret))
	    fi
    fi
    set -e

    # Archive all logs in single file
    do_archive "${WORKSPACE}/${prefix}/${target_name}/*.tap"

    if [ "$jenkins_opt_artifacts" == "always" ] || [ "$jenkins_opt_artifacts" == "fail" -a "$rc" -gt 0 ]; then
	    set +x
	    gzip "${jenkins_test_artifacts}.tar"
	    echo "======================================================"
	    echo "Jenkins result for [${target_name}] target: return $rc"
	    echo "Artifacts: ${jenkins_test_artifacts}.tar.gz"
	    echo "======================================================"
	    set -x
    fi

done

rm -rf $WORKSPACE/config.cache

echo "[${0##*/}]..................exit code = $rc"
exit $rc
