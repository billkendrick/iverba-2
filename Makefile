CC65=/usr/bin/cc65
CA65=/usr/bin/ca65
LD65=/usr/bin/ld65
CC65_HOME=/usr/share/cc65/
CC65_INC=/usr/share/cc65/include/
CC65_ASMINC=/usr/share/cc65/asminc/
CC65_LIB=/usr/share/cc65/lib/
CC65_CFG=/usr/share/cc65/cfg/
FRANNY=/usr/local/franny/bin/franny
XXD=/usr/bin/xxd

CC65_FLAGS=-Osir --add-source


all:	iverba2.atr

run:	all
	atari800 -nobasic iverba2.atr

clean:
	-rm iverba2.atr
	-rm iverba2.xex
	-rm hiscore.dat
	-rm *.dic
	-rm *.o
	-rm iverba2.s
	-rm sound.s
	-rm iverba2.map
	-rm iverba2_fnt.h
	-rm font-to-h

en_us.dic:	mkdict.php
	./mkdict.php /usr/share/dict/american-english en_us.dic 8

iverba2.atr:	iverba2.atr.in iverba2.xex \
		en_us.dic hiscore.dat
	cp iverba2.atr.in iverba2.atr
	${FRANNY} -A iverba2.atr -i iverba2.xex -o AUTORUN
	${FRANNY} -A iverba2.atr -i en_us.dic -o EN_US.DIC
	${FRANNY} -A iverba2.atr -i hiscore.dat -o HISCORE.DAT

iverba2.xex:	iverba2.o sound.o atari.cfg
	${LD65} \
		--cfg-path "cfg" \
		--lib-path "${CC65_LIB}" \
		-o iverba2.xex \
		-t atari \
		-m iverba2.map \
		iverba2.o \
		sound.o \
		atari.lib

iverba2.o:	iverba2.s
	${CA65} -I "${CC65_ASMINC}" -t atari iverba2.s -o iverba2.o

iverba2.s:	iverba2.c iverba2_fnt.h sound.h
	${CC65} ${CC65_FLAGS} -I "${CC65_INC}" \
		-t atari \
		iverba2.c \
		-o iverba2.s

sound.o:	sound.s
	${CA65} -I "${CC65_ASMINC}" -t atari sound.s -o sound.o

sound.s:	sound.c sound.h
	${CC65} ${CC65_FLAGS} -I "${CC65_INC}" \
		-t atari \
		sound.c \
		-o sound.s

iverba2_fnt.h:	iverba2_fnt.pbm font-to-h
	./font-to-h

font-to-h:	font-to-h.c
	gcc font-to-h.c -o font-to-h

hiscore.dat:
	echo "\0\0\0" > hiscore.dat

# Creates a blank ATR, for generating a fresh "disk/gemdrop.atr.in" by hand.
# Steps:
# 1. Download "udos.atr" from this ABBUC thread
#    http://www.abbuc.de/community/forum/viewtopic.php?f=3&t=10347)
#    (FIXME: is there an official home for this tool besides this forum
#    thread?)
# 2. Boot an Atari (or emulator) with "udos.atr" in D1:, and this disk
#    image ("blank.atr") in D2:
# 3. From the XDOS prompt, run UDOSINIT.COM
# 4. Press [2] to select D2: as the target drive
# 5. Press [Shift]+[W] to write uDOS to D2:
# 6. Rename "blank.atr" to "iverba2.atr.in"
#    (e.g., "mv blank.atr iverba2.atr.in")
#    to copy over the existing file; be sure to commit!
blank.atr:
	${FRANNY} -C blank.atr -d s -f a
	${FRANNY} -F blank.atr

