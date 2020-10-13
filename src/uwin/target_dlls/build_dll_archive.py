
import sys
import os

dllsfile = sys.argv[1]
overridesfile = sys.argv[2]

bindir = sys.argv[3]
dlls = sys.argv[4:]

def embedded_data_ref(name):
    return "ldr_binary_" + name + "_start"

with open(dllsfile, 'w') as f:
    f.write("#include <stdint.h>\n")
    for dll in dlls:
        f.write('const uint8_t %s[] = { ' % embedded_data_ref(dll + "_DLL"))
        with open(bindir + "/" + dll + ".DLL", 'rb') as dllf:
            f.write(', '.join('0x%02x' % x for x in dllf.read()))
        f.write(' };\n')


with open(overridesfile, 'w') as f:
    f.write("#ifndef INCLUDE_LDR_OVERRIDES_H\n#error \"Not for direct usage\"\n#endif\n\n")
    
    #f.write("#define LDR_OVERRIDES \\\n")
    #for x in dlls:
        #f.write("    " + x + " \\\n")
    #f.write("\n")
    
    for x in dlls:
        f.write("extern \"C\" const uint8_t %s[];\n" % embedded_data_ref(x + "_DLL"))
    f.write("\n")
    
    f.write("static struct ldr_override ldr_overrides[] = {\n")
    for x in dlls:
        f.write("    { \"%s\", %s },\n" % (x + ".DLL", embedded_data_ref(x + "_DLL")))
    f.write("};\n")
