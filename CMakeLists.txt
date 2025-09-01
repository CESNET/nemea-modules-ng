cmake_minimum_required(VERSION 3.12)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 2)
set(VERSION_PATCH 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

project(nemea-modules-ng VERSION ${VERSION})

include(cmake/build_type.cmake)
include(cmake/installation.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(NM_NG_ENABLE_DOC_DOXYGEN "Enable build of code documentation" OFF)
option(NM_NG_BUILD_WITH_ASAN    "Build with Address Sanitizer (only for CMAKE_BUILD_TYPE=Debug)" OFF)
option(NM_NG_BUILD_WITH_UBSAN   "Build with Undefined Behavior Sanitizer (only for CMAKE_BUILD_TYPE=Debug)" OFF)
option(NM_NG_ENABLE_TESTS       "Build with tests of modules" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra ")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -ggdb3 -fsanitize=address")

if (NM_NG_BUILD_WITH_ASAN)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -fsanitize-recover=address")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -fsanitize-recover=address")
endif()

if (NM_NG_BUILD_WITH_UBSAN)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=undefined")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=undefined")
endif()

include(cmake/dependencies.cmake)

add_subdirectory(modules)
add_subdirectory(common)
add_subdirectory(pkg)
add_subdirectory(doc)

if (NM_NG_ENABLE_TESTS)
	include(cmake/tests.cmake)
endif()
