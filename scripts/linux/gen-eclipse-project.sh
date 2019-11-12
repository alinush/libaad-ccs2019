#!/bin/bash
set -e

scriptdir=$(cd $(dirname $0); pwd -P)
sourcedir=$(cd $(dirname $0)/../..; pwd -P)
. $scriptdir/shlibs/check-env.sh

projdir="$AAD_BUILD_DIR_BASE/eclipse"

mkdir -p "$projdir"
cd "$projdir"
echo "Storing project in $projdir ..."
cmake $AAD_CMAKE_ARGS -G "Eclipse CDT4 - Unix Makefiles" "$sourcedir"
