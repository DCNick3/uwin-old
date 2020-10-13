 
add_dll_target(
    NAME MSS32
    SRCS mss32.c mss32_stubs.c mss32.def
    DEPS KSVC
)
