#
# Targets
#

add_library(aad 
    BitString.cpp
    Library.cpp
    NtlLib.cpp
    PolyCommit.cpp
    PublicParameters.cpp
    Utils.cpp
)

message("binary include dir: ${CMAKE_CURRENT_BINARY_DIR}")

target_include_directories(aad PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>" # for the Configuration.h file
    #"$<INSTALL_INTERFACE:${INSTALL_CMAKE_DIR}/include>"
)

target_link_libraries(aad PUBLIC ntl ff zm gmp gmpxx xutils xassert)

# TODO: only do this when MULTICORE is enabled
include(FindOpenMP)
if(OPENMP_FOUND)
    target_link_libraries(aad PUBLIC gomp)
endif()

#target_link_libraries(aad PUBLIC Threads::Threads)

#
# Installation
# FIXME: Add <Package>Config[Version].cmake files so this package can be easily imported?
# (See https://cmake.org/cmake/help/git-master/manual/cmake-packages.7.html#creating-packages)
#

# This creates the <Package>Config.cmake file and installs it
install(TARGETS aad EXPORT aadConfig 
   ARCHIVE DESTINATION lib)
install(EXPORT aadConfig DESTINATION lib/cmake/aad)

# This installs the static or (/and?) dynamic library
install(TARGETS aad 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# This installs the headers
# WARNING: Don't add / at the end. No slash means aad/ directory is created in the destination path
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/aad DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../include/aad DESTINATION include) # installs the Configuration.h file
