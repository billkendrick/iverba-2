# Details on: Invenies Verba ("Find the Words")
- By Bill Kendrick <bill@newbreedsoftware.com>
- http://www.newbreedsoftware.com/iverba/

## How the Game Was Made
### The original
Though the source code to "Lex" was open sourced by Simple Machine
(see: http://www.simplemachine.co/2014/07/lex-is-open-source/),
as of this writing (September 2014), I've so far only glanced at it once,
which helped me understand the game's scoring a little better.

The original Invenies Verba, created in 2014, was produced on a
Linux laptop using a PHP script I created during the NOMAM 2014
"BASIC 10-Liners" contest, which allows me to create a BASIC program
listing as a plain ASCII text file (using, e.g.,"{heart}" and "{^a}"
to represent ATASCII characters 0 and 1).  A Makefile then used the
tool "Franny" to generate a disk image containing MyDOS, TurboBASIC XL,
a small bootstrap BASIC program, and the BASIC program listing.
It then launched the Atari800 emulator and booted the disk.
(See: http://newbreedsoftware.com/atari/linux2tbasicxl/)
MyDOS would boot and run TurboBASIC XL, and the bootstrap program
would `ENTER` the source code file, which could then be `RUN` or
`SAVE`-ed as a tokenized BASIC program file.

The official release of the game was created by taking the tokenized
version and running it through the TurboBASIC XL compiler, which
created a byte-code version of the game which ran under the
TBXL runtime.

Another PHP script was created that analyzed a dictionary file
(e.g., "/usr/share/dict/american-english") to determine the most
frequently-used letters and their frequency.  It then created a
dictionary file for the game based on the 15 most frequent letters
(e.g., for English, it finds ESIARTNOLDCUGPM).  You can use this yourself
to create new dictionaries (see "Extending", below).

### The rewrite
In 2021, I took the TurboBASIC XL source code and ported it,
more-or-less line-by-line, to the C language.  The build process
now uses the "cc65" compiler tools to generate an Atari executable
program, which it places on a disk.  The relatively new minimal,
game-oridented DOS "uDOS" (micro DOS) is now used instead of MyDOS.

This gives flexibility in adding new features, as well as
improving performance of the game.

## Some technical features
### Binary search
The game uses a binary search routine to find whether or not
a word you've entered exists in its dictionary.  It starts
at the middle, and depending on whether your word is earlier
or later in the dictionary, jumps backward or forward to
the beginning or end.  It then repeats this process, using
smaller and smaller steps.

Say a dictionary contained only these few words:

- Apple
- Banana
- Cherry
- Orange
- Pear
- Pretzel
- Strawberry

If you wanted to find out if the word "Banana" was there,
such a search would first compare that to the word in the
very middle: "Orange".

"Banana" is earlier, so it jumps up by half of the size of the dictionary,
to the very first word: "Apple".

"Banana" comes after "Apple", so it jumps forward by a quarter of the size of
the dictionary, to "Cherry".  "Banana" comes before "Cherry", so now it jumps
backward again, this time by an eighth the size of the dictionary, and finds
"Banana".

If instead you entered "Pineapple", it would go from "Orange"
to "Strawberry", then back up to "Pear", then forward to
"Pretzel".  It will not find the word, because it doesn't exist.

### Fine horizontal scrolling
As you enter letters for your word, fine horizontal scrolling is
used to move that line of the screen half a character to the left
whenever the word is an odd number of letters long.

This allows the text to always remain centered on the screen,
rather than the word moving left after every other letter is
entered.

The scroll register is also used when you try to enter a
non-existent word, by randomly 'shaking' the word around
for a moment while the screen flashes red and it plays an
unhappy sound effect.

### Half font effect
The Atari 8-bit has the ability to use user-defined character
sets (fonts).  1,024 bytes are used to define 128 ATASCII
characters.  (In most modes, they are monochrome, and 8x8.
There are 8 bits per byte, and 8 bytes per character.)

In a standard text mode (`GRAPHICS 0` in Atari BASIC and OS parlance),
you get those symbols, plus inverse-video (think "highlighted") versions
of the same, giving you 256 symbols to display.  The highest bit
of the character is used to enable inverse video.  The lower seven
bits choose which of the 128 characters to display.

In the large text modes used by this game (`GRAPHICS 1` and
`GRAPHICS 2`) only 64 symbols are available.  The TWO highest
bits of the character are used to pick which color from the
Atari's 4 playfield colors should be used.  The lower SIX bits
choose which of the 64 characters to display.  (As you might
have noticed, inverse-video is not offered in these modes.)

Normally, the 'top half' of a character set is used, which
includes the uppercase alphabet, punctuation, and numerals.
(ATASCII characters 32 (space) through 95 (underscore).)

So if you display some alphabetic text containing lowercase,
it will appear as uppercase, but in a different color (than
if you had entered the text in uppercase).  If you apply inverse
to the character, you get uppercase (NOT in inverse-video)
in a third color.  And inverse plus lowercase gives you the
letter (again in uppercase, and not inverse-video) in
a fourth color.

You can access the 'other half' of a character set in these
graphics modes by pointing the Atari's ANTIC chip at the second
half of the character set data (512 bytes, or "2 pages" later).

In Atari BASIC, try `GRAPHICS 2:POKE 756,226:?#6;"HELLO"`.
It will appear in lowercase.  Note also that the entire
large text area of the screen shows the heart symbol
(normally printed as character 0) rather than blank spaces
(character 32), due to this effect.  Use `POKE 756,224` to
get back to normal.  (And note that what's shown in the
`GRAPHICS 0`-style text window at the bottom is not affected.)

This game uses a full 1KB font, with all 128 symbols
redefined.  A Display List Interrupt (DLI) is utilized to
do this adjustment for one line -- where the game tiles
(the available letters) are shown on the screen.

This allows the full alphabet to appear in a completely
different style for those tiles than for the rest of the
interface (title screen, credits, score, the word you're
entering, etc.).

### NTSC vs PAL detection
The game attempts to play the same speed (in terms of how
quickly letter meters fill up, at least) regardless as
to whether you're playing on an NTSC- or PAL-based system.

The gameplay does not utilize Vertical Blank Interrupts
(because the game was ported from TurboBASIC XL!), but
does keep an eye on the realtime clock registers in the
Atari OS, which are ticked up every VBI.  On NTSC that's
60 times per second, and on PAL it's 50 times per second.

True PAL Ataris have both PAL GTIA and ANTIC chips,
and there's a register in GTIA that can be read to
determine if the system is PAL or NTSC.

However, some Atari users (I've personally done this myself)
have taken NTSC Atari systems and plugged PAL ANTIC chips
into them, to gain the longer VBI period provided by a PAL
system, and this allowing PAL-only games and demoscene demos
to run (with some slight adjustment of a monitor's vertical-hold
controls).

Therefore, a more reliable method of detecting whether a system
is PAL or NTSC is to, in a very tight loop, watch how far
ANTIC's `VCOUNT` register climbs before cycling back to zero.
This works because, while the PAL standard refresh less frequently,
it offers more scanlines than NTSC.  The `VCOUNT` register won't
go as high on an NTSC system.

### Vertical Blank Interrupt (VBI) driven sound
The original TurboBASIC XL version of the game used simple BASIC
`SOUND` calls to briefly play different notes corresponding to
different interactions (adding a letter, typing an invalid letter,
etc.)  Generally, it sound begin playing a sound, do some action
(more BASIC code), and then silence the sound.  The amount of time
spent with game logic was enough for the sound to play a reasonable
amount of time.

When ported to C, the sounds played for such a brief time that
it was hard to hear them.  Adding a delay loop would help, but
would slow down the rest of the game, including its responsiveness
to the player's keyboard input.

So instead, a small VBI routine was created which plays sounds;
chords, in fact, while the rest of the game logic is occuring.
Major chords are played when successfully adding letters, and
successfully submitting a valid word.  Minor chords are played
when erasing letters (e.g., with `[BACKSPACE]`) or if a word is
invalid.

To make it interesting, a progressive series of chords are
played as you enter more letters; it descends when you erase
letters.

### Mostly C; only some assembly
Only the Display List Interrupt (DLI) and Vertical Blank Interrupt (VBI)
described above, as well as a very small routine to invoke the Atari
OS's Central Input/Output (CIO) routine, were written in assembly
language.  The former two are inline, within the C source code ".c"
files (wrapped in `asm()` calls), while the latter is in an
assembly language ".s" file.

