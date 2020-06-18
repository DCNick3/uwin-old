cmake_minimum_required(VERSION 3.17)
project(uwin C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

option(USE_HALFIX "Use halfix for x86 cpu emulation" ON)
option(USE_JIT "Use jit for cpu emulation" OFF)

include_directories(include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)

set(HALFIX_CPU_SOURCES
        src/halfix/ops/arith.c
        src/halfix/ops/bit.c
        src/halfix/ops/ctrlflow.c
        src/halfix/ops/misc.c
        src/halfix/ops/simd.c
        src/halfix/ops/stack.c
        src/halfix/ops/string.c
        src/halfix/access.c
        src/halfix/cpu.c
        src/halfix/decoder.c
        src/halfix/eflags.c
        src/halfix/fpu.c
        src/halfix/libcpu.c
        src/halfix/mmu.c
        src/halfix/opcodes.c
        src/halfix/prot.c
        src/halfix/seg.c
        src/halfix/smc.c
        src/halfix/softfloat.c
        src/halfix/trace.c
        src/uwin/halfix_cpu_loop.c)

set(SLJIT_SOURCES
        src/sljit/sljitLir.c)

set(JIT_SOURCES
        ${SLJIT_SOURCES}
        src/uwin/jit_cpu_loop.c
        src/uwin-jit/basic_block_cache.cpp)

set(UWIN_POSIX_SOURCES
        src/uwin/host/posix/memory_map.c
        src/uwin/host/posix/time.c
        src/uwin/host/posix/sighandler.c
        src/uwin/host/posix/jitmem.c)

set(DLLS_FILE ${CMAKE_CURRENT_BINARY_DIR}/dlls.c)

set(UWIN_SOURCES
        ${DLLS_FILE}
        src/uwin/util/file.c
        src/uwin/util/str.c
        src/uwin/util/mem.c
        src/uwin/ldr.c
        src/uwin/thread.c
        src/uwin/handletable.c
        src/uwin/syscall.c
        src/uwin/cfg.c
        src/uwin/file.cc
        src/uwin/win32_mq.c
        src/uwin/sdl_ui.c
        src/uwin/sdl_gmq.c
        src/fcaseopen.c
        src/main.c
        src/stb_ds.c
        src/ini.c
        src/uwin/syncronization.c)

if (UNIX)
    list(APPEND UWIN_SOURCES ${UWIN_POSIX_SOURCES})
endif()

if (USE_HALFIX)
    list(APPEND UWIN_SOURCES ${HALFIX_CPU_SOURCES})
    add_definitions(-DUW_USE_HALFIX)
endif()

if (USE_JIT)
    list(APPEND UWIN_SOURCES ${JIT_SOURCES})
    add_definitions(-DUW_USE_JIT)
endif()

if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
    add_definitions(-DUW_DEBUG)
endif (CMAKE_BUILD_TYPE EQUAL "DEBUG")

#set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

add_executable(uwin
        ${UWIN_SOURCES})


add_custom_command(OUTPUT ${DLLS_FILE} dummy_file_that_will_never_exist
        COMMAND make -j1 -C ${CMAKE_SOURCE_DIR}/src/uwin/target_dlls DEST_FILE="${DLLS_FILE}")

target_link_libraries(uwin m Threads::Threads SDL2::SDL2)
