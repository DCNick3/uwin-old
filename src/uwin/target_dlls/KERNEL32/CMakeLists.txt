 
add_dll_target(
    NAME KERNEL32
    SRCS kernel32.c kernel32_stubs.c kernel32.def
    CFLAGS "SHELL:--entry _DllMain"
    DEPS KSVC
)
