#!/bin/bash -eExl
#
# Testing script: package
#
# Copyright (c) 2020-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# BSD-3-Clause
#
# See file LICENSE for terms.
#

source $(dirname $0)/globals.sh

do_check_filter "Checking for rpm ..." "off"

cd $WORKSPACE

rm -rf $rpm_dir
mkdir -p $rpm_dir
cd $rpm_dir

rpm_tap=${WORKSPACE}/${prefix}/rpm.tap

opt_tarball=1
opt_srcrpm=1
opt_binrpm=1
opt_checkpkg=1
opt_rpm=0

cd ${build_dir}/0

if [ -x /usr/bin/dpkg-buildpackage ]; then
    echo "Build on debian"
    opt_rpm=0
else
    echo "Build rpms"
    opt_rpm=1
    rpmspec=${build_dir}/0/contrib/scripts/dpcp.spec
    rpmmacros="--define='_rpmdir ${rpm_dir}/rpm-dist' \
               --define='_srcrpmdir ${rpm_dir}/rpm-dist' \
               --define='_sourcedir ${rpm_dir}' \
               --define='_specdir ${rpm_dir}' \
               --define='_builddir ${rpm_dir}' \
               --define='_tmppath ${rpm_dir}/_tmp'"
    rpmopts="--nodeps --buildroot='${rpm_dir}/_rpm'"
fi

echo "1..$(($opt_tarball + $opt_srcrpm + $opt_binrpm + $opt_checkpkg))" > $rpm_tap

# SuSE can not create this folder
mkdir -p ${rpm_dir}/rpm-dist
mkdir -p ${rpm_dir}/deb-dist

test_id=0
if [ $opt_tarball -eq 1 ]; then
    # Automake 1.10.1 has a bug https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=456632
    if [ -n "$(automake --version | grep 'automake (GNU automake) 1.10.1')" ]; then
        test_exec='make dist'
    else
        test_exec='make dist && make distcheck'
    fi

    do_check_result "$test_exec" "$test_id" "tarball" "$rpm_tap" "${rpm_dir}/rpm-${test_id}"
    eval $timeout_exe cp dpcp*.tar.gz ${rpm_dir}
    test_id=$((test_id+1))
fi

if [ $opt_rpm -eq 0 ]; then
    cd ${rpm_dir}/deb-dist
    tar xzvf ${rpm_dir}/dpcp*.tar.gz
    cd $(find . -maxdepth 1 -type d -name "dpcp*")
fi

if [ $opt_srcrpm -eq 1 ]; then
    if [ $opt_rpm -eq 1 ]; then
        test_exec="rpmbuild -bs $rpmmacros $rpmopts $rpmspec"
    else
        test_exec="dpkg-buildpackage -us -uc -S"
    fi
    do_check_result "$test_exec" "$test_id" "srcrpm" "$rpm_tap" "${rpm_dir}/rpm-${test_id}"
    test_id=$((test_id+1))
fi

if [ $opt_binrpm -eq 1 ]; then
    if [ $opt_rpm -eq 1 ]; then
        test_exec="rpmbuild -bb $rpmmacros $rpmopts $rpmspec"
    else
        test_exec="dpkg-buildpackage -us -uc -b"
    fi
    do_check_result "$test_exec" "$test_id" "binrpm" "$rpm_tap" "${rpm_dir}/rpm-${test_id}"
    test_id=$((test_id+1))
fi

if [ $opt_checkpkg -eq 1 ]; then
    test_exec="${WORKSPACE}/contrib/build_pkg.sh -b -s -i ${WORKSPACE} -o ${rpm_dir}/deb-dist-pkg"
    do_check_result "$test_exec" "$test_id" "checkpkg" "$rpm_tap" "${rpm_dir}/rpm-${test_id}"
    test_id=$((test_id+1))
fi

# check if we have email of indevidual users in the packages rpm/deb metadata Maintainer field
if [ $opt_rpm -eq 1 ]; then
    pacakges_location="$rpm_dir"/rpm-dist
    email_log_file="$rpm_dir"/rpm-dist/email_scan.log
    search_filter="*.rpm"
    test_info_exec="rpm -qpi --changelog"
else
    pacakges_location="$rpm_dir"/deb-dist-pkg/packages
    email_log_file="$rpm_dir"/deb-dist-pkg/packages/email_scan.log
    search_filter="*.deb"
    test_info_exec="apt info"
fi

# iterate on all packages and extarct the metadata to outout file
touch "$email_log_file"
find "$pacakges_location" -type f -name "$search_filter" -exec $test_info_exec {} \; | tee -a "$email_log_file"
do_archive "$email_log_file"
set +e
# grep email strings exclude allowed email networking-support@nvidiacom
test_output=$(grep -E -o "\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,6}\b" "$email_log_file" | grep -v "networking-support")
test_rc=$?
set -e
# check rc - grep will return 0 if it found such mail and 1 if not
if [[ $test_rc -eq 0 ]]; then
    # if we found such mail we will get return code 0
    echo "ERROR: found bad email address $test_output"
    rc=$((rc + 1))
elif [[ -n "$test_output" ]]; then
    # if we got rc not 0 and we have output it means something else failed
    echo "ERROR: could not find bad email but something else failed: $test_output"
    rc=$((rc + 1))
fi

echo "[${0##*/}]..................exit code = $rc"
exit $rc
