 
add_dll_target(
    NAME DDRAW
    SRCS ddraw.c uw_ddraw.c uw_ddraw_stubs.c uw_ddrawsurface.c uw_ddrawsurface_stubs.c uw_vtables.c ddraw.def
    DEPS KSVC
)
