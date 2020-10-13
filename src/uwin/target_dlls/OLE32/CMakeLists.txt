 
add_dll_target(
    NAME OLE32
    SRCS ole32.c ole32.def
    DEPS KSVC KERNEL32
)
