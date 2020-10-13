 
add_dll_target(
    NAME WINMM
    SRCS winmm.c winmm_stubs.c winmm.def
    DEPS KSVC KERNEL32
)
