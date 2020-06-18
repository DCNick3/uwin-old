
import sys
import os

import pefile

target_module_name = sys.argv[1]
callees = sys.argv[2:]

FUN = set()

for x in callees:
  pe = pefile.PE(x)
  for entry in pe.DIRECTORY_ENTRY_IMPORT:
    modname = str(entry.dll, 'utf8').lower()
    if modname == target_module_name.lower() + ".dll":
      for imp in entry.imports:
        impname = str(imp.name, 'utf8')
        FUN.add(impname)
FUN = list(sorted(FUN))
print(FUN)

os.makedirs(target_module_name, exist_ok=True)
os.chdir(target_module_name)
with open('Makefile', 'w') as f:
  f.write("\nNAME=%s.DLL\nCFILES=%s_stubs.c %s.c\nOTHER_FILES=%s.def\ninclude ../common_dll.mak\n\n" % (target_module_name.upper(), target_module_name.lower(), target_module_name.lower(), target_module_name.lower()))
  #f.write("\t$(CC) $(CFLAGS)  -nostdlib -shared -Os --entry 0 -o $@ $+ -lKSVC\n\t$(STRIP) $@")
with open(target_module_name.lower() + '.def', 'w') as f:
  f.write("LIBRARY %s.DLL\nEXPORTS\n" % target_module_name.upper())
  f.write('\n'.join(FUN) + '\n')
with open(target_module_name.lower() + "_stubs.c", 'w') as f:
  f.write("\n#include <stubhlp.h>\n\n")
  for x in FUN:
    f.write("STUB(%s)\n" % x)
with open(target_module_name.lower() + ".c", 'w') as f:
  f.write("\n#include <windows.h>\n\n")
  
