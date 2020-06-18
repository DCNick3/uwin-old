
# I don't want to bother myself with getting the gcc call convention right, so here's a bunch of assembly code =)

.global _ifc20_stub_create_mouse
_ifc20_stub_create_mouse:
  ret
  
.global _ifc20_stub_destroy_mouse
_ifc20_stub_destroy_mouse:
  ret

.global _ifc20_stub_initialize_mouse
_ifc20_stub_initialize_mouse:
  mov $0, %eax
  ret $12


.global _ifc20_stub_create_enclosure
_ifc20_stub_create_enclosure:
  ret

.global _ifc20_stub_destroy_enclosure
_ifc20_stub_destroy_enclosure:
  ret
  
.global _ifc20_stub_initialize_enclosure
_ifc20_stub_initialize_enclosure:
  mov $0, %eax
  ret $52
  
