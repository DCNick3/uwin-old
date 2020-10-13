 
add_dll_target(
    NAME ADVAPI32
    SRCS advapi32.c advapi32.def
    DEPS KSVC
)
