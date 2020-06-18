
#ifndef STUB_HLP_H
#define STUB_HLP_H

#include <ksvc.h>

#define STRINGIFY(x) #x
#define STUB(name) void __declspec(dllexport) name(void) {kpanic(__FILE__ ":" STRINGIFY(__LINE__) " -> stub of " #name " called");}

#endif
 
