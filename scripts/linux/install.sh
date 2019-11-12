#!/bin/bash
set -e

scriptdir=$(cd $(dirname $0); pwd -P)
sourcedir=$(cd $scriptdir/../..; pwd -P)
. $scriptdir/shlibs/check-env.sh

builddir=$AAD_BUILD_DIR

echo "Installing from '$builddir' ..."
echo
[ ! -d "$builddir" ] && $scriptdir/cmake.sh
cd "$builddir"
sudo make install
