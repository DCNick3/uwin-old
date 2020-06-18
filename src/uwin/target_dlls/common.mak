

# convert to lowercase
lc = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))

all: $(NAME)

clean:
	rm -f $(NAME)

LOC_CFLAGS+=-march=i586 -nostdlib -shared -O2 --entry 0 -Wl,--enable-stdcall-fixup -Wl,--disable-runtime-pseudo-reloc -g
LOC_LIBS+=-lgcc

$(NAME): $(CFILES) $(OTHER_FILES)
	$(CC) $(CFLAGS) $(LOC_CFLAGS) -o $@ $+  $(LIBS) $(LOC_LIBS)
	$(OBJCOPY) -O elf32-i386 --only-keep-debug $@ $(call lc,$(basename $@)).elf
	$(STRIP) $@

PHONY: depend

