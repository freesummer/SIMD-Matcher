# Copyright (c) 2015 Flowgrammable.org
# All rights reserved

cmake_minimum_required(VERSION 3.0)
project(match CXX)
enable_testing()

set(env{CTEST_OUTPUT_ON_FAILURE} TRUE)

# Configure the compiler.
#
# We effectively require a functioning C++11 implementation.
# For Clang, use the libc++ as the standard library.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8")
    message(FATAL_ERROR "Requires GCC version 4.8 or greater")
  endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -gdwarf")  
endif()

# Compile in C++11.
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -fno-omit-frame-pointer") // backup
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -march=haswell -g -fno-omit-frame-pointer")

# Require Boost C++ Libraries.
find_package(Boost 1.54.0 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})


add_executable(prefix_kmeans.test prefix_kmeans.cpp)
target_link_libraries(prefix_kmeans.test "-static-libgcc -static-libstdc++")

add_executable(linear_arbitrary.test linear_arbitrary.cpp)
target_link_libraries(linear_arbitrary.test "-static-libgcc -static-libstdc++")

add_executable(hot_tree.test hot_tree.cpp)
target_link_libraries(hot_tree.test ${TBB_IMPORTED_TARGETS})


add_test(test_prefix_kmeans prefix_kmeans.test)
add_test(test_hot_tree hot_tree.test)
add_test(test_linear_arbitrary linear_arbitrary.test)




