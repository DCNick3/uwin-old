
NAME=JITTEST.EXE
CFILES=
OTHER_FILES=test.s
include ../common.mak

LOC_CFLAGS+= --entry _start
LOC_LIBS+= -lKSVC

test.s: mktest.py
	python3 mktest.py > test.s

