# Invenies Verba ("Find the Words")
- By Bill Kendrick <bill@newbreedsoftware.com>
- http://www.newbreedsoftware.com/iverba/
- Version 1 (TurboBASIC XL): August - September 18, 2014
- Version 2 (C): June 30, 202 - July 23, 2021

## About
This game is based loosely the game "Lex", by Simple Machine, LLC,
<http://www.simplemachine.co/>.  It was available for Android, iPhone
and on the web.  Its source code was made available under an
open source license, although this game is not based on that source.

See [`DETAILS.md`](DETAILS.md) for more information.

## Objective
The game presents you with a random set of letters.  Use the letters
to construct words (of 3 or more letters each) and gain points for
each letter used.  As you utilize letters, new ones appear in their
place.

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
For example, American English (`EN_US`), British English (`EN_UK`),
Polish (`PL_PL`), etc.

Press the key that corresponds to the dictionary you wish to play.

Once the dictionary loads, the title screen will prompt you to press
a key to begin the game.

The `[START]` key begins a standard game.

The `[P]` key begins a practice game.  Your score is not eligible to
be recorded as the high score.  The game does not end when any letter's
meter fills completely; you can continue playing until you abort the
game manually (see below).

## Game Screen
Random letters will appear in the middle of the screen.  Above each letter
is the scoring value for the letter.  To the left of each letter is a
vertical meter that rises over time.

At the top left of the screen, the level (and hence score multiplier)
is shown.  (e.g., "`2X`")

Your current score, along with the highest score you've ever earned
during a game, is shown at the top center.  (e.g., "`35/1459`")

Also near the top, a bar going across the screen shows how far you've
progressed in the current level.

As you press keys to enter a word, they appear in the center of the screen.
Just below, the points you'd receive for the word (assuming it's valid)
are shown (e.g., "`+6`", "`+7 +2`", etc.)

## Playing the Game
Use the letter keys on the keyboard to enter letters and create words.

Remember, they must be at least 3 letters long!  Also, each letter may
only be used once per word; in fact, the dictionary of valid words ONLY
includes words that contain one of any given letter.  (So "dad" and "mom"
are not possible.)

Use the alphabetic keys on the keyboard (`[A]` through `[Z]`).
When playing with the German dictionary, use the `[<]` key (directly
to the right of the `[0]`) to enter the letter 'eszett', "ẞ".

Press `[BACKSPACE]` to delete letters.  `[SHIFT]`+`[BACKSPACE]` deletes
the entire word.

Press `[RETURN]` to submit the word.  (If the word doesn't exist in
the dictionary, a tone will sound and the screen will flash red.)

Press `[ESC]` to abort your game (required, when playing in
practice mode!)

After the game ends, press `[ESC]` (or a console key, like `[Start]`)
to return to the title screen.

