#
# CMakeLists.txt
#
# Copyright (C) 2022 Sebastian Reimers
#

##############################################################################
#
# Project and Versioning
#

cmake_minimum_required(VERSION 3.18)

set(CMAKE_C_COMPILER clang)

project(slmix VERSION 0.4.0 LANGUAGES C)


##############################################################################
#
# Compile options
#

option(USE_SD_SOCK "Enable systemd socket" OFF)
option(USE_UNIX_SOCK "Enable UNIX socket" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options("/W3")
else()
  add_compile_options(
    -pedantic
    -Wall
    -Wbad-function-cast
    -Wcast-align
    -Wextra
    -Wmissing-declarations
    -Wmissing-prototypes
    -Wnested-externs
    -Wno-strict-aliasing
    -Wold-style-definition
    -Wshadow -Waggregate-return
    -Wstrict-prototypes
    -Wuninitialized
    -Wvla
  )
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wshorten-64-to-32 -Watomic-implicit-seq-cst)
endif()

if(USE_SD_SOCK)
  list(APPEND MIX_DEFS -DSLMIX_SD_SOCK)
  list(APPEND LINKLIBS systemd)
endif()

if(USE_UNIX_SOCK)
  list(APPEND MIX_DEFS -DSLMIX_UNIX_SOCK)
endif()

##############################################################################
#
# Subdirectory section
#

set(RE_LIBRARY re CACHE STRING "re_library")
set(REM_LIBRARY rem CACHE STRING "rem_library")
set(STATIC ON CACHE BOOL "Build static")
set(APP_MODULES_DIR ${CMAKE_SOURCE_DIR}/modules)
set(APP_MODULES amix vmix)
set(MODULES ice dtls_srtp turn opus avcodec fakevideo auresamp)

add_subdirectory(external/re)
add_subdirectory(external/baresip)

find_package(re CONFIG REQUIRED)

list(APPEND RE_DEFINITIONS
  -D_GNU_SOURCE
)

include_directories(
    include
    external/re/include
    external/baresip/include
)

##############################################################################
#
# Source/Header section
#

set(SRCS_LIB
  src/avatar.c
  src/chat.c
  src/http.c
  src/sess.c
  src/users.c
  src/ws.c
  src/mix.c
)

set(SRCS_EXE
  src/main.c
)


##############################################################################
#
# Target objects
#

add_library(${PROJECT_NAME}-lib STATIC ${SRCS_LIB})
target_compile_definitions(${PROJECT_NAME}-lib PRIVATE ${RE_DEFINITIONS} ${MIX_DEFS}
  SLMIX_VERSION="${PROJECT_VERSION}")

add_executable(${PROJECT_NAME} ${SRCS_EXE})
target_link_libraries(${PROJECT_NAME} PRIVATE slmix-lib baresip re gd ${LINKLIBS})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${RE_DEFINITIONS} ${MIX_DEFS}
  SLMIX_VERSION="${PROJECT_VERSION}")
