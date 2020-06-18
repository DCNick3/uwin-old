
import re
import sys

classname = sys.argv[1]
ifaceptr_name = 'LP' + classname.upper()

rex = re.compile(r"\s*STDMETHOD_?\((?:([^,]*),)?(\w+)\)\(THIS_?\s*(.*)\) PURE;\s*")
def stripcomments(text):
    return re.sub('//.*?\n|/\*.*?\*/', '', text, flags=re.S)

aaa = stripcomments(sys.stdin.read()).split('\n')

matches = [ rex.match(x) for x in aaa if x.strip() ]

print(matches)

forward_defs = '\n'.join([ '{0} WINAPI {3}_{1}({4} iface{2});'.format(x.group(1) or 'HRESULT', x.group(2), ', ' + x.group(3) if x.group(3).strip() else '', classname, ifaceptr_name) for x in matches ])

print(forward_defs)
