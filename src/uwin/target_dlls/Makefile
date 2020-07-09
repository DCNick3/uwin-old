
DLL_NAMES=KSVC IFC20 INIT VERSION KERNEL32 USER32 GDI32 ADVAPI32 OLE32 WINMM DDRAW WSOCK32 SHELL32 MSS32 BINKW32
DLL_FILES=$(foreach dll,$(DLL_NAMES),bin/$(dll).DLL)

EXE_NAMES=JITTEST
EXE_FILES=$(foreach exe,$(EXE_NAMES),bin/$(exe).EXE)

FILES=$(DLL_FILES) $(EXE_FILES)

MINGW_PREFIX?=i686-w64-mingw32-
MINGW_CC=$(MINGW_PREFIX)gcc
MINGW_CFLAGS=-I$(realpath ../../../include/uwin/) -I$(realpath include) -L$(realpath bin)
MINGW_STRIP=$(MINGW_PREFIX)strip
MINGW_OBJCOPY=$(MINGW_PREFIX)objcopy

DEST_FILE?=dlls.c

# convert to lowercase
lc = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))

all: dlls.c

clean:
	for dir in $(DLL_NAMES); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -rf bin debug
	rm -rf $(DEST_FILE)
	
mkbin:
	mkdir -p bin/ debug/

$(DLL_NAMES): mkbin
	# overrides are bad. TODO: rewrite makefiles to use normal env variables
	make CC="$(MINGW_CC)" STRIP="$(MINGW_STRIP)" OBJCOPY="$(MINGW_OBJCOPY)" CFLAGS="$(MINGW_CFLAGS)" -C $@
	cp $@/$@.DLL bin/
	cp $@/$(call lc,$@).elf debug/
	cp $@/lib$@.a bin/ 2> /dev/null|| true
	
$(EXE_NAMES): mkbin
	make CC="$(MINGW_CC)" STRIP="$(MINGW_STRIP)" OBJCOPY="$(MINGW_OBJCOPY)" CFLAGS="$(MINGW_CFLAGS)" -C $@
	cp $@/$@.EXE bin/

	
$(DLL_FILES): $(DLL_NAMES)
$(EXE_FILES): $(EXE_NAMES)

dlls.c: $(DLL_FILES) $(EXE_FILES)
	python3 build_dll_archive.py $(DEST_FILE) $(DLL_FILES)

.PHONY: $(DLL_NAMES) mkbin all
