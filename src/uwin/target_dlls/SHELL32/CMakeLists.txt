 
add_dll_target(
    NAME SHELL32
    SRCS shell32.c shell32_stubs.c shell32.def
    DEPS KSVC
)
