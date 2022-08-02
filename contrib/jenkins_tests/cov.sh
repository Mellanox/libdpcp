#!/bin/bash -xeEl

source $(dirname $0)/globals.sh

do_check_filter "Checking for coverity ..." "on"

COVERITY_VERSION="2021.12"
do_module "tools/cov-${COVERITY_VERSION}"

cd $WORKSPACE
./autogen.sh -s

rm -rf $cov_dir
mkdir -p $cov_dir
cd $cov_dir

cov_exclude_file_list="tests"

cov_build_id="cov_build_${BUILD_NUMBER}"
cov_build="$cov_dir/$cov_build_id"

set +eE

${WORKSPACE}/configure --prefix=${cov_dir}/install $jenkins_test_custom_configure > "${cov_dir}/cov.log" 2>&1
make clean >> "${cov_dir}/cov.log" 2>&1
eval "cov-configure --config $cov_dir/coverity_config.xml --gcc"
eval "cov-build --config $cov_dir/coverity_config.xml --dir $cov_build make $make_opt >> "${cov_dir}/cov.log" 2>&1"
rc=$(($rc+$?))

for excl in $cov_exclude_file_list; do
    cov-manage-emit --config $cov_dir/coverity_config.xml --dir $cov_build --tu-pattern "file('$excl')" delete
    sleep 1
done

eval "cov-analyze --config $cov_dir/coverity_config.xml \
	--all --aggressiveness-level low \
	--enable-fnptr --fnptr-models --paths 20000 \
	--disable-parse-warnings \
	--dir $cov_build \
	--strip-path ${WORKSPACE}"

if [ "${do_coverity_snapshot}" == "true" ]; then
    eval "cov-commit-defects --ssl --on-new-cert trust \
        --url https://coverity.mellanox.com:8443 \
        --user ${DPCP_COV_USER} --password ${DPCP_COV_PASSWORD} \
        --dir $cov_build \
        --stream dpcp_master_linux \
        --strip-path ${WORKSPACE}"
fi

if [ "${do_coverity_diff}" == "true" ]; then
    eval "cov-run-desktop --ssl --on-new-cert trust \
        --url https://coverity.mellanox.com:8443 \
        --user ${DPCP_COV_USER} --password ${DPCP_COV_PASSWORD} \
        --config $cov_dir/coverity_config.xml \
        --dir $cov_build \
        --stream dpcp_master_linux \
        --exit1-if-defects true \
        --present-in-reference false \
        --triage-attribute-not-regex classification Pending \
        --analyze-captured-source"
    rc=$(($rc+$?))
fi

set -eE

cov_web_path="$(echo $cov_build | sed -e s,$WORKSPACE,,g)"
nerrors=$(cov-format-errors --exclude-files '/usr/include/.*\.h$' --dir $cov_build --html-output $cov_build/output/errors | awk '/Processing [0-9]+ errors?/ { print $2 }')
if [ "${do_coverity_diff}" != "true" ]; then
    rc=$(($rc+$nerrors))
fi

index_html=$(cd $cov_build && find . -name index.html | cut -c 3-)
cov_url="$WS_URL/$cov_web_path/${index_html}"
cov_file="$cov_build/${index_html}"

rm -f jenkins_sidelinks.txt

coverity_tap=${WORKSPACE}/${prefix}/coverity.tap

echo 1..1 > $coverity_tap
if [ $rc -gt 0 ]; then
    echo "not ok 1 Coverity detected $nerrors in-project errors # $cov_url" >> $coverity_tap
    do_err "coverity" "${cov_build}/output/summary.txt"
    info="Coverity found $nerrors in-project errors"
    status="error"
else
    echo ok 1 Coverity found no new issues >> $coverity_tap
    info="Coverity found no new issues"
    status="success"
fi

if [ -n "$ghprbGhRepository" ]; then
    context="MellanoxLab/coverity"
    do_github_status "repo='$ghprbGhRepository' sha1='$ghprbActualCommit' target_url='$cov_url' state='$status' info='$info' context='$context'"
fi

echo Coverity report: $cov_url
printf "%s\t%s\n" Coverity $cov_url >> jenkins_sidelinks.txt

module unload tools/cov-${COVERITY_VERSION}

do_archive "$( find ${cov_build}/output -type f -name "*.txt" -or -name "*.html" -or -name "*.xml" )"

echo "[${0##*/}]..................exit code = $rc"
exit $rc
