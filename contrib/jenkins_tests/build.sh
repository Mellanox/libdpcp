#!/bin/bash -eExl
#
# Testing script: build
#
# Copyright (c) 2020-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# BSD-3-Clause
#
# See file LICENSE for terms.
#

source $(dirname $0)/globals.sh

do_check_filter "Checking for building with gcc ..." "off"

cd $WORKSPACE

rm -rf ${build_dir}
mkdir -p ${build_dir}
cd ${build_dir}

# Set symbolic links to default build and install
ln -s "${build_dir}/0/install" "${install_dir}"

build_list="\
default: "


build_tap=${WORKSPACE}/${prefix}/build-$DISTRO.tap
echo "1..$(echo $build_list | tr " " "\n" | wc -l)" > $build_tap

test_id=0
for build in $build_list; do
    IFS=':' read build_name build_option <<< "$build"
    mkdir -p ${build_dir}/${test_id}
    cd ${build_dir}/${test_id}
    test_exec='${WORKSPACE}/configure --prefix=${build_dir}/${test_id}/install $build_option $jenkins_test_custom_configure && make $make_opt install'
    do_check_result "$test_exec" "$test_id" "$build_name" "$build_tap" "${build_dir}/build-${test_id}"
    cd ${build_dir}
    test_id=$((test_id+1))
done


echo "[${0##*/}]..................exit code = $rc"
exit $rc
