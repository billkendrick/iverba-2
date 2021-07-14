# Invenies Verba ("Find the Words")
By Bill Kendrick <bill@newbreedsoftware.com>
Version 1 (TurboBASIC XL): August - September 18, 2014
Version 2 (C): June 30, 202 - July 13, 2021

NOTE: This document is not yet up-to-date with 2.0

## About
This game is based loosely the game "Lex", by Simple Machine, LLC,
<http://www.simplemachine.co/>.  It was available for Android, iPhone
and on the web.  Its source code was made available under an
open source license, although this game is not based on that source.

## Objective
The game presents you with a random set of letters.  Use the letters
to construct words (of 3 or more letters each) and gain points for
each letter used.  As you use letters, new letters appear to replace
them.

Each letter is worth a certain score.  More common letters are worth
fewer points than less common letters. (e.g., letter "E" is worth
1 point, letter "P" is worth 3 points, etc.)

Double points are earned for each letter beyond the 4th
(e.g., the "S" in "CARES").

Each letter has a meter which fills up over time.  If any letter's
meter fills completely, the game ends.  More common letters' meters
fill up more quickly than less common letters (e.g., the letter "E"'s
meter fills up three times as fast as the letter "P"'s).

After every third word you enter, you progress to the next level.
Each level provides a multiplier bonus.  (e.g., letters worth 2 points
provide a score gain of 4 points on level 2, 6 points on level 3, etc.)
However, the speed at which meters fill increases at each new level.

## Start-up
When you first run the game, you'll be asked to choose a dictionary.
Currently, American English (EN_US), Spanish (ES_ES), and French (FR_FR)
are available.  (Other languages are possible, see the "Extending" section,
below.)

Press the key that corresponds to the dictionary you wish to play
(e.g., [A] for English, etc.)  The title screen will appear and the
dictionary will be loaded.

After you've chosen a dictionary, the game will remember your choice.
On subsequent loads, the game won't ask again.  However, you can press
the [OPTION] key from the title screen to bring the dictionary menu up
again.

Once the dictionary loads, the title screen will prompt you to press
the [START] key to begin the game.

## Game Screen
Random letters will appear at the bottom of the screen.  Above each letter
is the scoring value for the letter.  To the left of each letter is a
vertical meter that rises over time.

At the top left of the screen, the level (and hence score multiplier)
is shown.  (e.g., "2X")

Your current score, along with the highest score you've ever earned
during a game, is shown at the top center.  (e.g., "35/1459")

Also near the top, a bar going across the screen shows how far you've
progressed in the current level.

As you press keys to enter a word, they appear in the center of the screen.
Just below, the points you'd receive for the word (assuming it's valid)
are shown (e.g., "+6", "+7 +2", etc.)

## Playing the Game
Use the letter keys on the keyboard to enter letters and create words.

Press [RETURN] to submit the word.  (If the word doesn't exist in
the dictionary, a tone will sound.)

Press [BACKSPACE] to delete letters.  [SHIFT]+[BACKSPACE] and [ESC]
both delete the entire word.

## How the Game Was Made
Though the source code to "Lex" was open sourced by Simple Machine
(see: http://www.simplemachine.co/2014/07/lex-is-open-source/),
as of this writing (September 2014), I've so far only glanced at it once,
which helped me understand the game's scoring a little better.

Invenies Verba was produced on a Linux laptop using a PHP script
I created during the NOMAM 2014 "BASIC 10-Liners" contest, which allows
me to create a BASIC program listing as a plain ASCII text file
(using, e.g.,"{heart}" and "{^a}" to represent ATASCII characters
0 and 1).  A Makefile then uses Franny to generate a disk image
containing MyDOS, TurboBASIC XL, a small bootstrap BASIC program,
and the BASIC program listing, then launches the Atari800 emulator
(See: http://newbreedsoftware.com/atari/linux2tbasicxl/)

Another PHP script was created that analyzes a dictionary file
(e.g., "/usr/share/dict/american-english") to determine the most
frequently-used letters and their frequency.  It then creates a
dictionary file for the game based on the 15 most frequent letters
(e.g., for English, it finds ESIARTNOLDCUGPM).  You can use this yourself
to create new dictionaries (see "Extending", below).

## Building the Game from Source
Prerequisites:
 * Make
 * PHP
 * Franny
 * Atari800 emulator

 1. Type "make run" to convert the source from ASCII (iverba.txt)
    to ATASCII (iverba.lst) and assemble files into a disk image,
    and launch Atari800 emulator.  TurboBASIC XL will load,
    a bootstrap BASIC program will run, which in turn loads the
    ATASCII listing for the game, and run it.

 2. Hit [Break] to return to a BASIC prompt, and save the program:
    SAVE "D:IVERBA.TBS"

 3. Use Atari800's "Disk Management" feature (via [F1] option) to
    mount the TurboBASIC XL compiler disk image, "tbxl_compiler.atr"
    (found in the the "tbasic" subdirectory) onto drive 2 (D2:).

 4. Exit TurboBASIC XL and go to the MyDOS menu:
    DOS

 5. Launch the TubroBASIC XL compiler from DOS.  Type [L] (Load Memory),
    then enter:
    D2:COMPILER.OBJ

 6. Type [1] to get a listing of files on drive 1 (D1:) and select
    the "IVERBA.TBS" entry.  TurboBASIC XL will compile the code.

 7. When prompted for a filename, enter "D1:AUTORUN.CTB"

 8. When prompted to save again ("Noch einmal speichern (J/N)?"),
    press [N].  Press [Control]+[D] and then [J] to return to MyDOS.

 9. Delete unnecessary files from the disk image using the [D]
    (Delete File(s)) command, and then entering a file name, once each for:
    TBASIC.AR0
    AUTORUN.BAS
    PROGRAM.LST
    IVERBA.TBS

 10. Copy the runtime software for compiled TurboBASIC XL programs
     off of the "tbxl_compiler.atr" disk image on drive 2, and onto
     drive 1.  Press [C] (Copy File(s)), then enter:
     D2:RUNTIME.OBJ,D1:RUNTIME.AR0
     (Note: ".AR0" makes it the first thing that loads after DOS.SYS
     has loaded.)

 11. Now you should be able to boot into the "iverba.atr" disk image
     again and the compiled version of the game will automatically load!
     (From MyDOS, you could use [M] (Run at Address), the enter
     "E477")


## Extending
The game uses dictionaries that contain words made up of 15 ASCII
letters -- for example, the English dictionary that comes with the
game uses words consisting of ACDEGILMNOPRSTU.  This is so that the
letters can be 'packed', two letters per byte (allowing for a
character representing a blank space, to fill words that are shorter
than the maximum length).

The "mkdict.php" script reads a dictionary full of words
(e.g., /usr/share/dict/american-english on my Ubuntu laptop),
determines which letters occur the most, finds all words containing
only those letters, and packs them into a file -- along with a list
of which letters each half-byte value represents, and the scoring
each letter should get in the game, based on the letter's frequency
in the original dictionary file (e.g., "en_us.dic").

On Ubuntu, other dictionaries (word lists) are available; you can
see which ones there are by consulting the "wordlist" package;
for example, the "Provided by" section of the output of
"aptitue show wordlist", or the "Reverse Provides" section of the
output of "apt-cache showpkg wordlist".  For example,
"wnorwegian".

The other option the "mkdict.php" program accepts is the maximum length
of words, e.g. 8.  It may be necessary to make dictionaries with
smaller words, to fit everything within the Atari's memory constraints.
(As of Sept. 2014, the game has approx. 26.5KB free, before loading
a dictionary file.)
