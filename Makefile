INCLUDEDIR = -I.
CC = gcc

include version.mk

CFLAGS = -g -Wall $(IPR_DEFINES) -DIPR_IPRUTILS
UTILS_VER = $(IPR_MAJOR_RELEASE).$(IPR_MINOR_RELEASE).$(IPR_FIX_LEVEL)

all: iprconfig iprupdate iprdump iprtrace iprdbg docs

iprconfig: iprconfig.c iprlib.o
	$(CC) $(CFLAGS) $(INCLUDEDIR) -o iprconfig iprconfig.c iprlib.o -lform -lpanel -lncurses -lmenu

iprupdate: iprupdate.c iprlib.o
	$(CC) $(CFLAGS) $(INCLUDEDIR) -o iprupdate iprlib.o iprupdate.c

iprdump:iprdump.c iprlib.o
	$(CC) $(CFLAGS) $(INCLUDEDIR) -o iprdump iprlib.o iprdump.c

iprtrace:iprtrace.c iprlib.o
	$(CC) $(CFLAGS) $(INCLUDEDIR) -o iprtrace iprlib.o iprtrace.c

iprdbg:iprdbg.c iprlib.o
	$(CC) $(CFLAGS) $(INCLUDEDIR) -o iprdbg iprlib.o iprdbg.c

iprlib.o: iprlib.c
	$(CC) $(CFLAGS) $(INCLUDEDIR) -o iprlib.o -c iprlib.c

docs: iprconfig.8 iprupdate.8 iprdump.8
	mkdir -p docs
	cp *.8 docs/
	gzip -f docs/iprconfig.8
	gzip -f docs/iprupdate.8
	gzip -f docs/iprdump.8

pdfs: docs
	nroff -man iprconfig.8 > docs/iprconfig.nroff
	nroff -man iprupdate.8 > docs/iprupdate.nroff
	nroff -man iprdump.8 > docs/iprdump.nroff
	a2ps -B --columns=1 -R -o docs/iprconfig.ps docs/iprconfig.nroff
	a2ps -B --columns=1 -R -o docs/iprupdate.ps docs/iprupdate.nroff
	a2ps -B --columns=1 -R -o docs/iprdump.ps docs/iprdump.nroff
	ps2pdf docs/iprconfig.ps docs/iprconfig.pdf
	ps2pdf docs/iprupdate.ps docs/iprupdate.pdf
	ps2pdf docs/iprdump.ps docs/iprdump.pdf
	rm docs/iprupdate.nroff docs/iprconfig.nroff docs/iprdump.nroff

utils: ./*.c ./*.h
	cd ..
	tar -zcpf iprutils-$(UTILS_VER)-src.tgz --directory ../ --exclude CVS iprutils

clean:
	rm -f iprupdate iprconfig iprdump iprtrace iprdbg iprupdate.ps iprupdate.pdf iprlib.o
	rm -f iprconfig.ps iprconfig.pdf iprdump.pdf iprdump.ps *.tgz *.rpm
	rm -rf docs

install: all
	install -d $(INSTALL_MOD_PATH)/sbin
	install --mode=755 iprconfig $(INSTALL_MOD_PATH)/sbin/iprconfig
	install --mode=755 iprupdate $(INSTALL_MOD_PATH)/sbin/iprupdate
	install --mode=755 iprdump $(INSTALL_MOD_PATH)/sbin/iprdump
	install --mode=755 iprtrace $(INSTALL_MOD_PATH)/sbin/iprtrace
	install --mode=700 iprdbg $(INSTALL_MOD_PATH)/sbin/iprdbg
	install -d $(INSTALL_MOD_PATH)/usr/share/man/man8
	install docs/iprconfig.8.gz $(INSTALL_MOD_PATH)/usr/share/man/man8/iprconfig.8.gz
	install docs/iprupdate.8.gz $(INSTALL_MOD_PATH)/usr/share/man/man8/iprupdate.8.gz
	install docs/iprdump.8.gz $(INSTALL_MOD_PATH)/usr/share/man/man8/iprdump.8.gz

rpm: *.c *.h *.8
	-make clean
	cd ..
	cp -f spec/iprutils.spec .
	cd .. && tar -zcpf iprutils-$(UTILS_VER)-src.tgz --exclude CVS iprutils
	mv ../iprutils-$(UTILS_VER)-src.tgz .
	rm -f iprutils.spec
	rpmbuild --nodeps -ts iprutils-$(UTILS_VER)-src.tgz
	cp /home/$(USER)/rpm/SRPMS/iprutils-$(UTILS_VER)-1.src.rpm .
