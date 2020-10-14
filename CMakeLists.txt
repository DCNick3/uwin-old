cmake_minimum_required(VERSION 3.10)
project(uwin C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

option(USE_HALFIX "Use halfix for x86 cpu emulation" ON)
option(USE_JIT "Use jit for cpu emulation" OFF)
option(USE_JITFIX "Enable validation code for jit based on halfix" OFF)

include_directories(include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})

include_directories(ext-deps/boots-ext-di/include)
include_directories(ext-deps/inipp/inipp)

if("${SDL2_LIBRARIES}" STREQUAL "")
    # CMake is portable? Nope, lol.
    # Depending on how SDL2 was built its usage differs
    set(SDL2_LIBRARIES "SDL2::SDL2")
endif()

set(USED_LIBS m Threads::Threads ${SDL2_LIBRARIES})


set(HALFIX_SOURCES
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
        )

set(HALFIX_CPU_SOURCES
        ${HALFIX_SOURCES}
        src/uwin/halfix_cpu_loop.cpp)

set(AARCH64_JIT_SOURCES
        src/uwin-jit/aarch64_code_generator.cpp)

set(JIT_SOURCES
        ${SLJIT_SOURCES}
        src/uwin/jit_cpu_loop.cpp
        src/uwin-jit/basic_block.cpp
        src/uwin-jit/basic_block_compiler.cpp
        src/uwin-jit/basic_block_cache.cpp
        src/uwin-jit/executable_memory_allocator.cpp)

if(USE_JIT)
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
        list(APPEND JIT_SOURCES src/uwin-jit/aarch64_code_generator.cpp)

        # include vixl code generation library
        list(APPEND USED_LIBS vixl)

        add_subdirectory("ext-deps/vixl")
    else()
        message(FATAL_ERROR "This platform does not have jit backend.")
    endif()
endif()


set(UWIN_POSIX_SOURCES
        src/uwin/host/posix/memory_map.cpp
        src/uwin/host/posix/time.cpp
        src/uwin/host/posix/sighandler.cpp
        src/uwin/host/posix/jitmem.cpp)

set(UWIN_SOURCES
        ${DLLS_FILE}
        src/uwin/util/str.cpp
        src/uwin/util/mem.cpp
        src/uwin/ldr.cpp
        src/uwin/thread.cpp
        src/uwin/handletable.cpp
        src/uwin/syscall.cpp
        src/uwin/cfg.cpp
        src/uwin/file.cpp
        src/uwin/win32_mq.cpp
        src/uwin/sdl_ui_gmq.cpp
        src/uwin/sdl/kobj/ksurf_sdl.cpp
        src/fcaseopen.cpp
        src/main.cpp
        )

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

    option(ZYDIS_BUILD_TOOLS "" OFF)
    option(ZYDIS_BUILD_EXAMPLES "" OFF)
    add_subdirectory("ext-deps/zydis")
    add_subdirectory("ext-deps/libx87")
    list(APPEND USED_LIBS x87)
endif()

if (USE_JITFIX)
    if (!USE_JIT)
        message(FATAL_ERROR "jitfix requires jit")
    endif()
    message("Enabling JITFIX support")
    list(APPEND UWIN_SOURCES ${HALFIX_SOURCES})
    add_definitions(-DUW_USE_JITFIX)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DUW_DEBUG)
    #add_definitions(-DUW_JIT_TRACE)
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")

set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer")

#set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pg")
#set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -pg")


add_executable(uwin
        ${UWIN_SOURCES})

include(ExternalProject)
set(TARGET_DLLS_DIR ${CMAKE_CURRENT_BINARY_DIR}/target_dlls-prefix/src/target_dlls-build)
set(DLLS_FILE ${TARGET_DLLS_DIR}/dlls.c)
set(LDR_OVERRIDES_FILE ${TARGET_DLLS_DIR}/ldr_overrides.h)
ExternalProject_add(target_dlls
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/uwin/target_dlls
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS ${DLLS_FILE} ${LDR_OVERRIDES_FILE}
        BUILD_ALWAYS true
        CMAKE_ARGS
            -DCMAKE_TOOLCHAIN_FILE:string=${CMAKE_CURRENT_LIST_DIR}/src/uwin/target_dlls/Toolchain-mingw-w64.cmake
        )

target_sources(uwin PUBLIC ${DLLS_FILE})
set_source_files_properties(src/uwin/ldr.cpp PROPERTIES OBJECT_DEPENDS ${LDR_OVERRIDES_FILE})

include_directories(${TARGET_DLLS_DIR})

add_dependencies(uwin target_dlls)

#add_custom_command(OUTPUT ${DLLS_FILE} dummy_file_that_will_never_exist
#        COMMAND make -j1 -C ${CMAKE_SOURCE_DIR}/src/uwin/target_dlls DEST_FILE="${DLLS_FILE}")

if (USE_JIT)
    list(APPEND USED_LIBS Zydis)
endif()

target_link_libraries(uwin ${USED_LIBS})

