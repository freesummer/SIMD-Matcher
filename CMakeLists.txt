# Copyright (c) 2015 Flowgrammable.org
# All rights reserved

cmake_minimum_required(VERSION 2.8)
project(match CXX)

option(USEAVX2 "USEAVX2" ON)
option(USETRAVIS "USETRAVIS" OFF)
option(USEAVX512 "USEAVX512" OFF)
option(USE_COUNTERS "USE_COUNTERS" OFF)
option(COVERAGE "COVERAGE" OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

#SET(CMAKE_BUILD_TYPE Debug)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/third-party/bilke-cmake-modules/")

if(USETRAVIS)
	SET(CMAKE_CXX_FLAGS "-std=c++14 -march=haswell -Wno-deprecated-declarations -Wall -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -faligned-new") #-fstack-check -fsanitize=undefined ${CMAKE_CXX_FLAGS}
elseif(USEAVX2)
	SET(CMAKE_CXX_FLAGS "-std=c++14 -march=native -mavx -mavx2 -mbmi2 -mlzcnt -Wno-deprecated-declarations -Wall -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -faligned-new") #-fstack-check -fsanitize=undefined ${CMAKE_CXX_FLAGS}
	#SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -debug")
	SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -DNDEBUG")
else()
	SET(CMAKE_CXX_FLAGS "-std=c++14 -march=native -Wno-deprecated-declarations -Wall ${CMAKE_CXX_FLAGS} -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
endif()

if(USEAVX512)
	SET(CMAKE_CXX_FLAGS "-mavx512f -mavx512vl -mavx512bw -mavx512dq -mavx512cd ${CMAKE_CXX_FLAGS}")
	add_definitions(-DUSE_AVX512)
endif()

if(USE_COUNTERS)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_COUNTERS=1")
endif(USE_COUNTERS)

set(TC_MALLOC_NAMES tcmalloc tcmalloc_minimal libtcmalloc_minimal.so.4.3.0 )

find_library(TC_MALLOC_LIBRARY
	NAMES ${TC_MALLOC_NAMES}
	PATHS /usr/lib/x86_64-linux-gnu /usr/lib64 /lib /usr/lib /usr/local/lib /opt/local/lib  /home/summer/libmatch/release/tbb_cmake_build
	NO_DEFAULT_PATH
)

SET(TBB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/tbb")
include(${TBB_ROOT}/cmake/TBBBuild.cmake)
tbb_build(TBB_ROOT ${TBB_ROOT} CONFIG_DIR TBB_DIR MAKE_ARGS)
find_package(TBB REQUIRED)

#set(BOOST_ROOT ${/home/grads/f/freesummer/opt/boost_1_67_0})
#set(Boost_INCLUDE_DIR ${/home/grads/f/freesummer/opt/boost_1_67_0})
#set(Boost_LIB_DIR ${/home/grads/f/freesummer/opt/boost_1_67_0/stage/lib})


SET(Hana_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/boost-hana")

# Configure the compiler.
#temporarily override to gcc 4.8 on ubuntu
#set(CMAKE_CXX_COMPILER "gcc-4.8")

# We effectively require a functioning C++11 implementation.
# For Clang, use the libc++ as the standard library.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8")
    message(FATAL_ERROR "Requires GCC version 4.8 or greater")
  endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")  
endif()

# Compile in C++11.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")


# Require Boost C++ Libraries.
find_package(Boost 1.54.0 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(.)

find_library(
	PATHS /usr/lib /usr/include /lib /usr/local/lib
)





#message(STATUS "CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE})

add_subdirectory(libmatch)
add_subdirectory(test)
ADD_SUBDIRECTORY(hot)
ADD_SUBDIRECTORY(idx)













