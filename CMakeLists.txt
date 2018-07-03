cmake_minimum_required(VERSION 2.8)
project(dll-hijack C)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release." FORCE)
endif (NOT CMAKE_BUILD_TYPE)
option(ENABLE_TESTS "Enable tests." TRUE)

if (MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MT /wd4996 /wd4068")
    set(CompilerFlags
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
    foreach (CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach ()
endif ()

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_C_FLAGS "-pipe -Wall -Wextra -nostartfiles -Wl,--gc-sections,--build-id=none")
    set(CMAKE_C_FLAGS_DEBUG "-g")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -s")
    if (CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(CMAKE_C_FLAGS_RELEASE "-Os -s")
    endif ()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_C_FLAGS}")
endif ()

set (CMAKE_SHARED_LIBRARY_PREFIX "")
set (CMAKE_SHARED_MODULE_PREFIX "")
set (SRC hijack.h)
aux_source_directory(minhook/src MinHook)
aux_source_directory(minhook/src/hde MinHook)
find_library(sys_version NAMES version)
add_library(version SHARED version.c version.h hook_knowndir.h ${SRC} ${MinHook})
add_library(iphlpapi SHARED iphlpapi.c iphlpapi.h hook_knowndir.h ${SRC} ${MinHook})
if (ENABLE_TESTS)
    enable_testing()
    add_executable(dir_test test.c)
    target_link_libraries(dir_test shell32 "${sys_version}" uuid ole32)
    add_custom_target(check
        COMMAND dir_test
        DEPENDS dir_test version
    )
endif()