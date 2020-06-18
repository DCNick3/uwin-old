#ifndef TARGET_SYSCALL_H
#define TARGET_SYSCALL_H

#define __USER_CS	(0x23)
#define __USER_DS	(0x2B)
#define __USER_FS	(0x33)
#define __USER_GS	(0x43)



#define TARGET_LDT_ENTRIES      8192
#define TARGET_LDT_ENTRY_SIZE	8

#define TARGET_GDT_ENTRIES             9
#define TARGET_GDT_ENTRY_TLS_ENTRIES   3
#define TARGET_GDT_ENTRY_TLS_MIN       6
#define TARGET_GDT_ENTRY_TLS_MAX       (TARGET_GDT_ENTRY_TLS_MIN + TARGET_GDT_ENTRY_TLS_ENTRIES - 1)

/*
struct target_modify_ldt_ldt_s {
    unsigned int  entry_number;
    abi_ulong base_addr;
    unsigned int limit;
    unsigned int flags;
};*/


#endif /* TARGET_SYSCALL_H */
