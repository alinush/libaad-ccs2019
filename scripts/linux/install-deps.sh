#!/bin/bash
set -e

scriptdir=$(cd $(dirname $0); pwd -P)
sourcedir=$(cd $scriptdir/../..; pwd -P)

sudo apt-get install libntl-dev libboost-dev g++ cmake clang-4.0 libgmp3-dev
