#!/bin/bash

if [ $# -lt 1 ]; then
    name=${BASH_SOURCE[@]}
    echo "Usage: source $name <buildtype>"
    echo
    echo "Examples:"
    echo
    echo "source $name debug"
    echo "source $name trace"
    echo "source $name release"
    echo "source $name relwithdebug"
    echo "source $name relwithprof"
    echo
    if [ -n "$AAD_HAS_ENV_SET" ]; then
        echo "Currently-set environment"
        echo "========================="
        echo "Build directory: $AAD_BUILD_DIR"
        echo "CMake args: $AAD_CMAKE_ARGS"
    fi
else
    # WARNING: Need to exit using control-flow rather than 'exit 1' because this script is sourced
    invalid_buildtype=0

    buildtype=`echo "$1" | tr '[:upper:]' '[:lower:]'`

    scriptdir=$(cd $(dirname ${BASH_SOURCE[@]}); pwd -P)
    sourcedir=$(cd $scriptdir/../..; pwd -P)
    #echo "Source dir: $sourcedir"

    . $scriptdir/shlibs/os.sh

    branch=`git branch | grep "\*"`
    branch=${branch/* /}
    builddir_base=~/builds/aad/$branch
    case "$buildtype" in
    trace)
        builddir=$builddir_base/trace
        cmake_args="-DCMAKE_BUILD_TYPE=Trace"
        ;;
    debug)
        builddir=$builddir_base/debug
        cmake_args="-DCMAKE_BUILD_TYPE=Debug"
        ;;
    release)
        builddir=$builddir_base/release
        cmake_args="-DCMAKE_BUILD_TYPE=Release"
        ;;
    relwithdebug)
        builddir=$builddir_base/relwithdebug
        cmake_args="-DCMAKE_BUILD_TYPE=RelWithDebInfo"
        ;;
    relwithprof)
        builddir=$builddir_base/relwithprof
        cmake_args="-DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS=-pg"
        ;;
    *)
        invalid_buildtype=1
        ;;
    esac

    if [ "$OS" == "OSX" ]; then
        echo
        echo "WARNING: OS X detected! Switching off OpenMP..."
        echo
        cmake_args="$cmake_args -DMULTICORE=OFF"
    fi

    #
    # grep-code alias
    #
    alias grep-code='grep --exclude-dir=.git --exclude=".gitignore" --exclude="*.html"'

    if [ $invalid_buildtype -eq 0 ]; then
        echo "Configuring for build type '$1' ..."
        echo

        export PATH="$scriptdir:$PATH"
        export PATH="$builddir/libaad/bin:$PATH"
        export PATH="$builddir/libaad/bin/app:$PATH"
        export PATH="$builddir/libaad/bin/test:$PATH"
        export PATH="$builddir/libaad/bin/bench:$PATH"
        export PATH="$builddir/libaad/bin/examples:$PATH"

        echo "Build directory: $builddir"
        echo "CMake args: $cmake_args"
        echo "PATH envvar: $PATH" 

        export AAD_BUILD_DIR=$builddir
        export AAD_BUILD_DIR_BASE=$builddir_base
        export AAD_CMAKE_ARGS=$cmake_args
        export AAD_HAS_ENV_SET=1
    else
        echo "ERROR: Invalid build type '$buildtype'"
    fi
fi
