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

project(slmix VERSION 0.0.1 LANGUAGES C)


##############################################################################
#
# Compile options
#

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


##############################################################################
#
# Subdirectory section
#

set(RE_LIBRARY re CACHE STRING "re_library")
set(REM_LIBRARY rem CACHE STRING "rem_library")
set(STATIC ON CACHE BOOL "Build static")
set(APP_MODULES_DIR ${CMAKE_SOURCE_DIR}/modules)
set(APP_MODULES aumix vidmix)
set(MODULES ice dtls_srtp turn opus avcodec fakevideo)

add_subdirectory(external/re)
add_subdirectory(external/rem)
add_subdirectory(external/baresip)

find_package(re CONFIG REQUIRED)

##############################################################################
#
# Source/Header section
#

set(SRCS
  src/main.c
  src/http.c
  src/sess.c
  src/avatar.c
)


##############################################################################
#
# Main target object
#

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} PRIVATE baresip rem re gd)
target_compile_definitions(${PROJECT_NAME} PRIVATE ${RE_DEFINITIONS})
