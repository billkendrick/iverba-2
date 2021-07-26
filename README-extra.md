This content will be updated and placed into README.md
once the features are re-implemented in Invenies Verba 2.0...

...

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

...

## Extending

### Note: This pre-release does not offer multiple dictionaries

The game uses dictionaries that contain words made up of 15 ASCII
letters -- for example, the English dictionary that comes with the
game uses words consisting of ACDEGILMNOPRSTU.  This is so that the
letters can be 'packed', two letters per byte (allowing for a
character representing a blank space, to fill words that are shorter
than the maximum length).

The "`mkdict.php`" script reads a dictionary full of words
(e.g., `/usr/share/dict/american-english` on my Ubuntu Linux laptop),
determines which letters occur the most, finds all words containing
only those letters, and packs them into a file -- along with a list
of which letters each half-byte value represents, and the scoring
each letter should get in the game, based on the letter's frequency
in the original dictionary file (e.g., "`en_us.dic`").

On Ubuntu, other dictionaries (word lists) are available; you can
see which ones there are by consulting the "`wordlist`" package;
for example, the "Provided by" section of the output of
"`aptitude show wordlist`", or the "Reverse Provides" section of the
output of "`apt-cache showpkg wordlist`".  For example,
"`wnorwegian`".

The other option the "`mkdict.php`" program accepts is the maximum length
of words, e.g. 8.  It may be necessary to make dictionaries with
smaller words, to fit everything within the Atari's memory constraints.

