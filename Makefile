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
	-rm build/iverba2.xex
	-rm build/hiscore.dat
	-rm build/*.dic
	-rm obj/*.o
	-rm asm/*.s
	-rm build/iverba2.map
	-rm font/iverba2_fnt.h
	-rm tools/font-to-h

build/en_us.dic:	tools/mkdict.php
	tools/mkdict.php /usr/share/dict/american-english build/en_us.dic 8

iverba2.atr:	disk/iverba2.atr.in build/iverba2.xex \
		build/en_us.dic build/hiscore.dat
	cp disk/iverba2.atr.in iverba2.atr
	${FRANNY} -A iverba2.atr -i build/iverba2.xex -o AUTORUN
	${FRANNY} -A iverba2.atr -i build/en_us.dic -o EN_US.DIC
	${FRANNY} -A iverba2.atr -i build/hiscore.dat -o HISCORE.DAT

build/iverba2.xex:	obj/iverba2.o obj/sound.o src/atari.cfg
	${LD65} \
		--cfg-path "src" \
		--lib-path "${CC65_LIB}" \
		-o build/iverba2.xex \
		-t atari \
		-m build/iverba2.map \
		obj/iverba2.o \
		obj/sound.o \
		atari.lib

obj/iverba2.o:	asm/iverba2.s
	${CA65} -I "${CC65_ASMINC}" -t atari asm/iverba2.s -o obj/iverba2.o

asm/iverba2.s:	src/iverba2.c font/iverba2_fnt.h src/sound.h
	${CC65} ${CC65_FLAGS} -I "${CC65_INC}" \
		-t atari \
		src/iverba2.c \
		-o asm/iverba2.s

obj/sound.o:	asm/sound.s
	${CA65} -I "${CC65_ASMINC}" -t atari asm/sound.s -o obj/sound.o

asm/sound.s:	src/sound.c src/sound.h
	${CC65} ${CC65_FLAGS} -I "${CC65_INC}" \
		-t atari \
		src/sound.c \
		-o asm/sound.s

font/iverba2_fnt.h:	font/iverba2_fnt.pbm tools/font-to-h
	tools/font-to-h

tools/font-to-h:	tools/font-to-h.c
	gcc tools/font-to-h.c -o tools/font-to-h

build/hiscore.dat:
	echo "\0\0\0" > build/hiscore.dat

# Creates a blank ATR, for generating a fresh "disk/iverba2.atr.in" by hand.
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

