 
add_dll_target(
    NAME WSOCK32
    SRCS wsock32.c wsock32_stubs.c wsock32.def
    DEPS KSVC
)
