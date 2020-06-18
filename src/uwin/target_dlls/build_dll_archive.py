
import sys
import os

dlls = sys.argv[2:]

with open(sys.argv[1], 'w') as f:
    f.write("#include <stdint.h>\n")
    for dll in dlls:
        name = os.path.basename(dll).replace('.', '_')
        f.write('const uint8_t _binary_%s_start[] = { ' % name)
        with open(dll, 'rb') as dllf:
            f.write(', '.join('0x%02x' % x for x in dllf.read()))
        f.write(' };\n')
