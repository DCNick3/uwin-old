 
add_dll_target(
    NAME BINKW32
    SRCS binkw32.c binkw32_stubs.c binkw32.def
    DEPS KSVC
)
