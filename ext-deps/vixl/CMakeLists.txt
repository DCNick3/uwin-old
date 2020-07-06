
aux_source_directory(vixl/src VIXL_SOURCES)
aux_source_directory(vixl/src/aarch64 VIXL_SOURCES)

add_definitions(-DVIXL_CODE_BUFFER_MALLOC -DVIXL_INCLUDE_TARGET_A64)



add_library(vixl STATIC ${VIXL_SOURCES})

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(vixl PUBLIC -DVIXL_DEBUG)
endif()

target_include_directories(vixl PUBLIC vixl/src)
