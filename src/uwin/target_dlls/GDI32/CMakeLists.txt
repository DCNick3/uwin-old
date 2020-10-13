 
add_dll_target(
    NAME GDI32
    SRCS gdi32.c gdi32_stubs.c gdi32.def
    DEPS KSVC
)
