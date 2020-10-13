
add_exe_target(NAME JITTEST
    SRCS test.s
    DEPS KSVC
    CFLAGS "SHELL:--entry _start")

add_custom_command(OUTPUT test.s
    COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/mktest.py > test.s)
    
