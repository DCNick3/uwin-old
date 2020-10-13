 
add_dll_target(
    NAME VERSION
    SRCS version.c version_stubs.c version.def
    DEPS KSVC
)
