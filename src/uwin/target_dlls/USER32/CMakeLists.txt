 
add_dll_target(
    NAME USER32
    SRCS user32.c user32_stubs.c user32.def
    CFLAGS "SHELL:--entry _DllMain"
    DEPS KSVC KERNEL32
)
