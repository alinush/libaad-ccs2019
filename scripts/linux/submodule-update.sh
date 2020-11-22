#!/bin/bash
set -e

scriptdir=$(cd $(dirname $0); pwd -P)
sourcedir=$(cd $scriptdir/../..; pwd -P)
. $scriptdir/shlibs/os.sh

(
    cd $sourcedir;
    git submodule init;
    git submodule update;
)
