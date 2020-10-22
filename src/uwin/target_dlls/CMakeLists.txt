cmake_minimum_required(VERSION 3.13)

project(uwin-dlls C ASM)

set(COMPONENT_NAMES KSVC INIT IFC20 VERSION KERNEL32 USER32 GDI32 ADVAPI32 OLE32 WINMM DDRAW WSOCK32 SHELL32 MSS32 BINKW32 JITTEST)

set(BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(DBG_DIR "${CMAKE_CURRENT_BINARY_DIR}/dbg")

file(MAKE_DIRECTORY ${BIN_DIR})
file(MAKE_DIRECTORY ${DBG_DIR})

set(BASELINE_CFLAGS -march=i586 -nostdlib -O2 -g)
set(BASELINE_LFLAGS -nostdlib -Wl,--enable-stdcall-fixup -Wl,--disable-runtime-pseudo-reloc)
set(BASELINE_DLL_LFLAGS "SHELL:--entry 0")

set(DLL_TARGETS CACHE INTERNAL "DLL_TARGETS")


function(add_target)
    set(options)
    set(oneValueArgs NAME EXT)
    set(multiValueArgs CFLAGS LFLAGS DEPS)
    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN})
    
    set(OUTPUT_NAME "${TARGET_NAME}.${TARGET_EXT}")
    set(DBG_PATH "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}")
    set(NODBG_PATH "${BIN_DIR}/${OUTPUT_NAME}")
    
    set(TARGET "${TARGET_NAME}")
    target_compile_options(${TARGET} PRIVATE ${BASELINE_CFLAGS})
    target_compile_options(${TARGET} PRIVATE ${TARGET_CFLAGS})
    target_link_options(${TARGET} PRIVATE ${BASELINE_LFLAGS})
    target_link_options(${TARGET} PRIVATE ${TARGET_LFLAGS})
    target_link_libraries(${TARGET} gcc ${TARGET_DEPS})
    set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${OUTPUT_NAME} PREFIX "" SUFFIX "")
    
    string(TOLOWER ${TARGET_NAME} TARGET_NAME_LOWER)
    set(ELF_FILE "${DBG_DIR}/${TARGET_NAME_LOWER}.elf")
    
    add_custom_command(OUTPUT ${ELF_FILE} ${NODBG_PATH}
        COMMAND ${CMAKE_OBJCOPY} -O elf32-i386 --only-keep-debug ${DBG_PATH} ${ELF_FILE} && ${CMAKE_STRIP} ${DBG_PATH} -o ${NODBG_PATH}
        DEPENDS ${TARGET}
    )
    
    add_custom_target(${TARGET}_DBG ALL
        DEPENDS ${ELF_FILE} ${NODBG_PATH}
    )
    
    message("Added ${TARGET_NAME}")
    
endfunction()

function(add_dll_target)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs CFLAGS LFLAGS SRCS DEPS)
    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN})
    
    set(TARGET "${TARGET_NAME}")
    add_library(${TARGET} SHARED ${TARGET_SRCS})
    target_link_options(${TARGET} PRIVATE ${BASELINE_DLL_LFLAGS})
    add_target(EXT "DLL" NAME ${TARGET} CFLAGS ${TARGET_CFLAGS} LFLAGS ${TARGET_LFLAGS} DEPS ${TARGET_DEPS})
    
    set(DLL_TARGETS "${DLL_TARGETS}" "${TARGET}" CACHE INTERNAL "DLL_TARGETS")
endfunction()

function(add_exe_target)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs CFLAGS LFLAGS SRCS CONFIGURATIONS DEPS)
    cmake_parse_arguments(TARGET "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN})
    
    set(TARGET "${TARGET_NAME}")
    add_executable(${TARGET} ${TARGET_SRCS})
    add_target(EXT "EXE" NAME ${TARGET} CFLAGS ${TARGET_CFLAGS} LFLAGS ${TARGET_LFLAGS} DEPS ${TARGET_DEPS})
endfunction()

include_directories("${CMAKE_CURRENT_LIST_DIR}/include" "${CMAKE_CURRENT_LIST_DIR}/../../../include/uwin")

foreach(component IN LISTS COMPONENT_NAMES)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/${component}")
endforeach()

message("dll targets: ${DLL_TARGETS}")

set(DLLS_C_FILE ${CMAKE_CURRENT_BINARY_DIR}/dlls.c)
set(OVERRIDES_H_FILE ${CMAKE_CURRENT_BINARY_DIR}/ldr_overrides.h)
set(DBG_TARGETS ${DLL_TARGETS})
list(TRANSFORM DBG_TARGETS APPEND _DBG)

add_custom_command(
    DEPENDS ${DBG_TARGETS} ${DLL_TARGETS} "${CMAKE_CURRENT_LIST_DIR}/build_dll_archive.py"
    OUTPUT ${DLLS_C_FILE} ${OVERRIDES_H_FILE}
    COMMAND python3 "${CMAKE_CURRENT_LIST_DIR}/build_dll_archive.py" ${DLLS_C_FILE} ${OVERRIDES_H_FILE} ${BIN_DIR} ${DLL_TARGETS}
)

add_custom_target(generated_files ALL DEPENDS ${DLLS_C_FILE} ${OVERRIDES_H_FILE})