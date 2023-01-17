#!/bin/bash -xeEl
#
# Testing script: gtest
#
# Copyright (c) 2019-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# BSD-3-Clause
#
# See file LICENSE for terms.
#

source $(dirname $0)/globals.sh

do_check_filter "Checking for gtest ..." "on"

cd $WORKSPACE

rm -rf $gtest_dir
mkdir -p $gtest_dir
cd $gtest_dir

gtest_app="$PWD/tests/gtest/gtest"
gtest_opt=

set +eE

${WORKSPACE}/configure --prefix=$install_dir
make
make -C tests/gtest

eval "sudo $timeout_exe env GTEST_TAP=2 $gtest_app $gtest_opt --gtest_output=xml:${install_dir}/gtest-all.xml"
rc=$(($rc+$?))

set -eE

for f in $(find $gtest_dir -name '*.tap')
do
    cp $f ${WORKSPACE}/${prefix}/gtest-$(basename $f .tap).tap
done

echo "[${0##*/}]..................exit code = $rc"
exit $rc
