# Simple makefile for Gram's Commander v3.3
#
# You should also make any necessary changes to the unixcfg.h
# and doscfg.h header files before building GC3.
#
# To use this Makefile, type `make dos' or `make unix'. Once
# everything has compiled properly, use `make install'.
#
# `make' on its own should work, but all sources will be
# recompiled each time.
#
# (c) Graham Wheeler, 1993
#

default:
	echo "You must specify a target, such as 'make dos' or 'make unix'"

dos: dosprep Makefile.2 gc3.key
	make -f Makefile.2

unix: unixprep Makefile.2 gc3.key
	make -f Makefile.2

dosprep: gcconfig.exe

unixprep: gcconfig

Makefile.2: site.def Makefile.gen
	-type site.def > Makefile.2
	-type Makefile.gen >> Makefile.2
	-cat site.def Makefile.gen > Makefile.2

gcconfig.exe: gcconfig.c
	-del gcconfig.exe
	-tcc gcconfig.c
	-bcc gcconfig.c
	.\gcconfig
	copy site.def+Makefile.gen Makefile.2

gcconfig: gcconfig.c
	-rm -f gcconfig
	-cc gcconfig.c
	-gcc gcconfig.c
	mv a.out gcconfig
	./gcconfig
	cat site.def Makefile.gen > Makefile.2

clean: Makefile.2
	make -f Makefile.2 clean
	-rm -f Makefile.2 site.*
	-del Makefile.2
	-del site.*

# The next entry is for the gc3 mail distribution list

post: package
	sh postSource
	
package: Makefile.2
	make -f Makefile.2 package

install: Makefile.2
	make -f Makefile.2 install

tar: Makefile.2
	make -f Makefile.2 tar

ship: Makefile.2
	make -f Makefile.2 ship

zip: Makefile.2
	make -f Makefile.2 zip

gc3: Makefile.2
	-make -f Makefile.2 gc3.exe
	-make -f Makefile.2 gc3

site.def: gcconfig.c
	gcconfig

site.env: site.def

site.ans:
	-bcc gcconfig.c
	-cc gcconfig.c
	gcconfig

