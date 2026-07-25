cmake_minimum_required(VERSION 3.5)
project(qbittorrent-portable C)

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
    link_libraries(ntdll)
endif ()

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_C_FLAGS "-pipe -Wall -Wextra -nostartfiles -Wl,--gc-sections,--build-id=none -static")
    set(CMAKE_C_FLAGS_DEBUG "-g -D_DEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-s -O3 -Os -fmerge-all-constants -fno-asynchronous-unwind-tables")
    if (CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(CMAKE_C_FLAGS_RELEASE "-Os -s -fmerge-all-constants -fno-asynchronous-unwind-tables")
    endif ()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_C_FLAGS}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_mingw_ntdll_crt.c"
            "size_t __cdecl strlen(const char *_Str);int main(void) {return strlen(\"str\");}")
    try_compile(NTDLL_RESULT ${CMAKE_CURRENT_BINARY_DIR}
            "${CMAKE_CURRENT_BINARY_DIR}/test_mingw_ntdll_crt.c"
            LINK_LIBRARIES ntdllcrt)
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/test_mingw_ntdll_crt.c")
    if(NTDLL_RESULT)
        link_libraries(ntdll ntdllcrt)
    else()
        link_libraries(ntdll)
    endif()
endif ()

set (CMAKE_SHARED_LIBRARY_PREFIX "")
set (CMAKE_SHARED_MODULE_PREFIX "")
set (SRC hook_knowndir.h native.h dll-hijack/hijack.h dll-hijack/native_api.h)
set(SOURCES_MINHOOK
  "minhook/src/buffer.c"
  "minhook/src/hook.c"
  "minhook/src/trampoline.c"
  "minhook/src/buffer_native.h"
  "minhook/src/thread_native.h"
  "minhook/src/ldr_native.h"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(SOURCES_HDE "minhook/src/hde/hde64.c")
else()
  set(SOURCES_HDE "minhook/src/hde/hde32.c")
endif()
add_library(minhook STATIC ${SOURCES_MINHOOK} ${SOURCES_HDE})
link_libraries(minhook)
# Search only the toolchain's own (arch-correct) link directories so a 32-bit
# build resolves the 32-bit import lib and does not accidentally pick up the
# 64-bit one from a mixed msys2 PATH (which silently breaks the x86 dir_test link).
find_library(sys_version NAMES version PATHS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES} NO_DEFAULT_PATH)
if (NOT sys_version)
    find_library(sys_version NAMES version)
endif ()
add_library(version SHARED version.c dll-hijack/version.h dll-hijack/version.def ${SRC})
add_library(iphlpapi SHARED iphlpapi.c dll-hijack/iphlpapi.h dll-hijack/iphlpapi.def ${SRC})
add_library(winmm SHARED winmm.c dll-hijack/winmm.h dll-hijack/winmm.def ${SRC})
if (ENABLE_TESTS)
    enable_testing()
    add_executable(dir_test test.c)
    target_link_libraries(dir_test shell32 "${sys_version}" uuid ole32)
    # dir_test statically imports version.dll; the loader resolves it from the
    # executable's own directory, so the hijack shim must be built and sitting
    # next to dir_test.exe before the test runs. Force both into the same dir
    # and make the shim a build dependency of the test.
    set_target_properties(dir_test version PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    add_dependencies(dir_test version)
    # CTest gate: dir_test exits non-zero on any ABI/liveness/path failure, and
    # hard-fails if the shim installed no hooks (result would be meaningless).
    add_test(NAME dir_test COMMAND dir_test)
    # Convenience target for a manual run outside ctest.
    add_custom_target(check
        COMMAND dir_test
        DEPENDS dir_test version
    )
endif()