# Copyright 2016 Comcast Cable Communications Management, LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required(VERSION 2.8.7)
project(svc_agt)
include(ExternalProject)
include(CTest)

set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/_install)
set(PREFIX_DIR ${CMAKE_CURRENT_BINARY_DIR}/_prefix)
set(PATCHES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/patches)
set(INCLUDE_DIR ${INSTALL_DIR}/include)
set(LIBRARY_DIR ${INSTALL_DIR}/lib${LIB_SUFFIX})
set(SA_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/../include)
set(SA_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/../src)
set(SA_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/../tests)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")

ExternalProject_Add(uthash
PREFIX ${CMAKE_BINARY_DIR}/uthash
GIT_REPOSITORY https://github.com/troydhanson/uthash
CONFIGURE_COMMAND ""
BUILD_COMMAND ""
INSTALL_COMMAND ""
LOG_DOWNLOAD ON
)
set(UTHASH_DIR ${CMAKE_BINARY_DIR}/uthash/src/uthash/include)
message (UTHASH_DIR "${UTHASH_DIR}")


set(SOURCES ${SA_SRC_DIR}/svc_agt.c
${SA_SRC_DIR}/svcagt_time.c
${SA_SRC_DIR}/svcagt_log.c
${SA_SRC_DIR}/svcagt_systemctl.c
${SA_SRC_DIR}/svcagt_db.c
${SA_SRC_DIR}/svcagt_files.c
${SA_SRC_DIR}/svcagt_startup_states.c
)


add_library(svc_agt SHARED ${SOURCES})
add_dependencies(svc_agt uthash)

include_directories(
${SA_INCLUDE_DIR}
${SA_SRC_DIR}
${UTHASH_DIR}
${INCLUDE_DIR}
)
include_directories (SYSTEM /usr/include)
message (STATUS "C FLAGS: ${CMAKE_C_FLAGS}")
link_directories ( ${LIBRARY_DIR} )
add_definitions(-std=c99)

if (BUILD_TESTING)
  add_subdirectory (tests)
endif (BUILD_TESTING)
