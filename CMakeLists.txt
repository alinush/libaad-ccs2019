cmake_minimum_required (VERSION 3.2)
# project(<name> VERSION <ver> LANGUAGES CXX)
project(libaad VERSION 0.1.0.0 LANGUAGES CXX)

#
# Configuration options
#
set(
  CURVE
  "BN128"
  CACHE
  STRING
  "Default curve: one of ALT_BN128, BN128, EDWARDS, MNT4, MNT6"
)

option(
  NO_PT_COMPRESSION
  "libff EC point compression. Do NOT change unless you also compiled libff with this option."
  ON
)

option(
  MULTICORE
  "Enable parallelized execution for multi-exponentiations in libff, using OpenMP"
  ON
)

option(
  BINARY_OUTPUT
  "In serialization of elliptic curve points, output raw binary data (instead of decimal), which is smaller and faster. Do NOT change unless you also compiled libff with this option."
  OFF
)

add_definitions(
  -DCURVE_${CURVE}
)

if("${MULTICORE}")
  add_definitions(-DMULTICORE=1)
endif()

if(${CURVE} STREQUAL "BN128")
  add_definitions(
    -DBN_SUPPORT_SNARK=1
  )
endif()

# OS X Catalina fix
include_directories(SYSTEM "/usr/local/include")
link_directories("/usr/local/lib")

# some fix I needed to hash boost::dynamic_bitset's
add_definitions(-DBOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libaad/include/aad/Configuration.h.in
	${CMAKE_CURRENT_BINARY_DIR}/libaad/include/aad/Configuration.h @ONLY)

#
# Configure CCache if available
#
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

set(aad_packages
    xassert
    xutils
)

#
# Dependencies
#
# TODO: Find ate-pairing, libff, libfqfft too or issue error with pointer to install script
foreach(package ${aad_packages})
    find_package(${package} QUIET)

    if(${package}_FOUND)
        message("${package} library is installed!")
    else()
        message("${package} library not installed locally, please download from https//github.com/alinush/lib${package}")
    endif()
endforeach()

#find_package(Threads REQUIRED)
#find_package(Boost 1.65 COMPONENTS program_options REQUIRED)
find_package(Boost 1.58 REQUIRED)

#include_directories(${Boost_INCLUDE_DIR})

#
# C++ options
#   TODO: change to set_target_properties?
#   https://crascit.com/2015/03/28/enabling-cxx11-in-cmake/
#
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#
# Compiler flags
#

# When you do 'a > b in 'C/C++, if a is unsigned and b is signed and equal to -1, C/C++
# actually casts b to unsigned (probably because casting unsigned to signed would require a bigger data type)
# Thus, 1 > -1 will evaluate to false because during the cast -1 will be set to to 2^32 - 1
#
# WARNING: For the love of god, do not remove this flag or you will regret it. Instead,
# just use signed types everywhere and cast your unsigned to signed when mixing unsigned
# variables with signed ones. See: http://soundsoftware.ac.uk/c-pitfall-unsigned
set(CXX_FLAGS_INTEGER_CORRECTNESS 
    "-Wconversion -Wsign-conversion")
set(CXX_FLAGS_FORMAT 
    "-Wformat-y2k -Wno-format-extra-args -Wno-format-zero-length -Wformat-nonliteral -Wformat-security -Wformat=2")
set(CXX_FLAGS_OPTIMIZATIONS "-O3")

string(APPEND CXX_FLAGS " ${CXX_FLAGS_OPTIMIZATIONS}")
string(APPEND CXX_FLAGS " ${CXX_FLAGS_FORMAT}")
string(APPEND CXX_FLAGS " ${CXX_FLAGS_INTEGER_CORRECTNESS}")
# TODO: use target_compile_features instead: 
#   https://cmake.org/cmake/help/v3.1/command/target_compile_features.html#command:target_compile_features
#   https://cmake.org/cmake/help/v3.1/prop_gbl/CMAKE_CXX_KNOWN_FEATURES.html#prop_gbl:CMAKE_CXX_KNOWN_FEATURES
string(APPEND CXX_FLAGS " -Wall")
string(APPEND CXX_FLAGS " -Werror")
string(APPEND CXX_FLAGS " -Wextra")


# TODO: Figure out right way to deal with -fstrict-overflow / -Wstrict-overflow related errors 
string(APPEND CXX_FLAGS 
    " -fno-strict-overflow")
string(APPEND CXX_FLAGS_DEBUG 
    " -D_FORTIFY_SOURCE=2")
# TODO: add -D_GLIBCXX_DEBUG if you want bounds checking for std::vector::operator[] but it seems to cause a segfault at the end of execution

if("${MULTICORE}")
    string(APPEND CMAKE_CXX_FLAGS " -fopenmp")
endif()

# GNU and Clang-specific flags
string(APPEND CMAKE_CXX_FLAGS 
    " ${CXX_FLAGS}")
string(APPEND CMAKE_CXX_FLAGS_DEBUG 
    " ${CXX_FLAGS_DEBUG}")
# When building with 'cmake -DCMAKE_BUILD_TYPE=Trace'
string(APPEND CMAKE_CXX_FLAGS_TRACE 
    " ${CXX_FLAGS_DEBUG} -DTRACE")

# using Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang-specific options
    string(APPEND CMAKE_CXX_FLAGS 
        " -ferror-limit=3")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG
        " -fstack-protector-all")

# using GCC
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # GCC-specific options
    string(APPEND CMAKE_CXX_FLAGS 
        " -fmax-errors=3")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG 
        " -fstack-protector-all")

# TODO: using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")

# TODO: using Visual Studio C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

endif()

#
# Testing flags
#
enable_testing()

# Subdirectories
add_subdirectory(libaad)
