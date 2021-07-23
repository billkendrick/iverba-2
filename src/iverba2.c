/*
  iverba2.c
  "Invenies Verba" for the Atari 8-bit

  By Bill Kendrick <bill@newbreedsoftware.com>

  * Original TurboBASIC XL code:
    August - September 2017

  * cc65 port:
    June 30, 2021 - July 23, 2021
*/

#define VERSION "PRE-RELEASE 4"
#define VERSION_DATE "2021-07-23"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <atari.h>
#include <peekpoke.h>
#include "sound.h"
#include "cio.h"

#pragma data-name (push,"FONT")
#include "../font/iverba2_fnt.h"
#pragma data-name (pop)

#define BOOL unsigned char
#define FALSE 0
#define TRUE 1

unsigned char * kbcode_to_atascii;
unsigned char wordlen, half_wordlen;
#define SRC_NOT_SET (wordlen + 1)
char tmp_msg[41];
char * grabbed_word, * best_word, * input, * avail, * blank;
BOOL * used;
unsigned char * src;
unsigned int * mad;
char * words, * lookups;
int num_words, num_letters;
int dict_ptr_mid;
long hiscore = 0, score;
BOOL got_high_score;
int best;
BOOL gameover;
unsigned char level, word_cnt;
int entry_len;
unsigned char * scr_mem;
unsigned char ltr_scores[26];
BOOL pal_speed;
BOOL dark = FALSE; /* FIXME: Allow changing & save setting */


/*
  CIO close file

  @return byte error code
*/
unsigned char disk_close(void) {
  OS.iocb[1].command = IOCB_CLOSE;
  return ciov(1);
}

/*
  CIO open file

  @param string filename
  @param byte mode (4=read, 8=write)
  @return byte error code
*/
unsigned char disk_open(char * fname, unsigned char mode) {
  disk_close();
  OS.iocb[1].command = IOCB_OPEN;
  OS.iocb[1].buffer = fname;
  OS.iocb[1].buflen = strlen(fname);
  OS.iocb[1].aux1 = mode;
  return ciov(1);
}

/*
  CIO read data

  @param pointer ptr
  @param unsigned int len
  @return byte error code
*/
unsigned char disk_read(unsigned char * ptr, unsigned int len) {
  OS.iocb[1].command = IOCB_GETCHR;
  OS.iocb[1].buffer = ptr;
  OS.iocb[1].buflen = len;
  return ciov(1);
}


/*
  Sleep a moment.

  @param int i -- how long to sleep (screen refreshes)
*/
void iv2_sleep(int i) {
  unsigned char target;

  target = OS.rtclok[2] + i;
  do { } while (OS.rtclok[2] != target);
}


/*
  Alters horizontal scroll register
  (waits for a VBI to complete first!)

  @param unsigned char h -- how far to scroll
  @sideeffects changes horiz. scroll register
*/
void SCROLL(unsigned char h) {
  unsigned char r;

  r = OS.rtclok[2];
  while (OS.rtclok[2] == r) {};

  ANTIC.hscrol = h;
}


/*
  Displays text on the screen.

  @param unsigned char x -- horizontal position (column, 0 = left, 19 = right)
  @param unsigned char y -- vertical position (row, 0 = top, 23 = bottom; note: screen has a custom display list)
  @param string str -- text to display
  @sideeffects displays the text
*/
void myprint(unsigned char x, unsigned char y, char * str) {
  int pos, i;
  unsigned char c;

  pos = y * 20 + x;
  for (i = 0; str[i] != '\0'; i++) {
    c = str[i];

    if (c < 32) {
      c = c + 64;
    } else if (c < 96) {
      c = c - 32;
    }

    scr_mem[pos + i] = c;
  }
}



/*
  Note table.  h/t Bobby Clark (Synthpopalooza)
  Source: https://atariage.com/forums/topic/216807-complete-pokey-note-table-for-all-distortion-settings/
*/
unsigned char note_table[] = {
  /*  0 C */  243, /* first word */
  /*  1 C# */ 230,
  /*  2 D */  217,
  /*  3 D# */ 204,
  /*  4 E */  193,
  /*  5 F */  182,
  /*  6 F# */ 172,
  /*  7 G */  162,
  /*  8 G# */ 153,
  /*  9 A */  144,
  /* 10 A# */ 136,
  /* 11 B */  128,
  /* 12 C */  121, /* second word */
  /* 13 C# */ 114,
  /* 14 D */  108,
  /* 15 D# */ 102,
  /* 16 E */   96,
  /* 17 F */   91,
  /* 18 F# */  85,
  /* 19 G */   81,
  /* 20 G# */  76,
  /* 21 A */   72,
  /* 22 A# */  68,
  /* 23 B */   64,
  /* 24 C */   60, /* third word */
  /* 25 C# */  57,
  /* 26 D */   53,
  /* 27 D# */  50,
  /* 28 E */   47,
  /* 29 F */   45,
  /* 30 F# */  42,
  /* 31 G */   40,
  /* 32 G# */  37,
  /* 33 A */   35,
  /* 34 A# */  33,
  /* 35 B */   31,
  /* 36 C */   30, /* success; next level */
  /* 37 C# */  28,
  /* 38 D */   26,
  /* 39 D# */  25,
  /* 40 E */   23,
  /* 41 F */   22,
  /* 42 F# */  21,
  /* 43 G */   19,
  /* 44 G# */  18,
  /* 45 A */   17,
  /* 46 A# */  16
};

unsigned char note_starts[] = { 0, 9, 18, 27 };


/* Volume levels (listed backwards from end to beginning of sound) */
unsigned char vol[] = {
   0,
   2,  4,  6,  8,
  10,  8, 12, 13,
  12, 14, 15,  8
};


unsigned char note1_ptr = 0;
unsigned char sound_timer = 0;
unsigned char note1, note2, note3;

/*
  Play a chord

  FIXME: Sound playing should be moved to a VBI routine

  @param BOOL major - major (vs minor) chord?
  @sideeffect plays a sound
*/
void play_chord(BOOL major) {
  note1 = note_table[note1_ptr + 0];
  if (major) {
    /* 1---2--3 e.g. E,G#,B */
    note2 = note_table[note1_ptr + 4];
  } else {
    /* A--B---C */
    note2 = note_table[note1_ptr + 3];
  }
  note3 = note_table[note1_ptr + 7];

  sound_timer = sizeof(vol) << 1;
}


/*
  Grab a word from the dictionary.

  @param int a -- the word to grab
  @sideeffect places the word in global string `grabbed_word`
*/
void grabword(int a) {
  int i, w, p;
  unsigned char b1, b2;

  p = a * half_wordlen;
  for (i = 0; i < half_wordlen; i++) {
    w = words[p + i];
    b1 = (w & 0x0F);
    b2 = (w & 0xF0) >> 4;

    grabbed_word[i * 2] = lookups[b1 * 2];
    grabbed_word[i * 2 + 1] = lookups[b2 * 2];
  }
  grabbed_word[wordlen] = '\0';
}

/*
  Search for a word in the dictionary.

  @param char * str -- string to search for
  @return BOOL true if found, false if not
*/
unsigned char binsearch(char * str) {
  char padded_str[9];
  int cut, cursor, smallstep_tries, i, cmp;
  BOOL found;

  /* Binary search starts with the cursor in the middle,
     and cuts things in half each time */
  cursor = dict_ptr_mid;
  cut = dict_ptr_mid * 2;
  found = FALSE;

  /* The dictionary pads each word with spaces, so
     pad our test string the same way */
  strcpy(padded_str, str);
  for (i = strlen(str); i < wordlen; i++) {
    padded_str[i] = ' ';
  }
  padded_str[wordlen] = '\0';

  /* Binary-search the dictionary */
  smallstep_tries = 0;
  do {
    grabword(cursor);

    cmp = strcmp(padded_str, grabbed_word);
    if (cmp == 0) {
      found = TRUE;
    } else {
      if (cmp > 0) {
        cursor += cut;
        if (cursor >= num_words) {
          cursor = num_words - 1;
        }
      } else {
        cursor -= cut;
        if (cursor < 0) {
          cursor = 0;
        }
      }
      cut = cut / 2;
      if (cut == 0) {
        cut = 1;
        smallstep_tries++;
      }
    }
  } while (found == FALSE && smallstep_tries <= 10);

  return found;
}


#pragma optimize (push, off)

/*
  Vertical Blank Interrupt -- play musical sounds
*/

void * OLDVEC;

void vbi(void) {
  /* if sound_timer > 0, then continue; otherwise we're all done */
  asm("ldx %v", sound_timer);
  asm("beq %g", __vbi_done);

  /* sound_timer-- */
  asm("dex");
  asm("stx %v", sound_timer);

  /* X = sound_timer >> 1 */
  asm("txa");
  asm("lsr");
  asm("tax");

  /* SOUND 0,note1,???,??? */
  asm("lda %v", note1);
  asm("sta %w", (unsigned)&POKEY_WRITE.audf1);

  /* SOUND 1,note1,???,??? */
  asm("lda %v", note2);
  asm("sta %w", (unsigned)&POKEY_WRITE.audf2);

  /* SOUND 2,note1,???,??? */
  asm("lda %v", note3);
  asm("sta %w", (unsigned)&POKEY_WRITE.audf3);

  /* A = vol[X] */
  asm("lda %v,x", vol);
  asm("lsr");
  asm("adc #$a0"); /* SOUND [0-2],???,10,vol */

  asm("sta %w", (unsigned)&POKEY_WRITE.audc1);
  asm("sta %w", (unsigned)&POKEY_WRITE.audc2);
  asm("sta %w", (unsigned)&POKEY_WRITE.audc3);

__vbi_done:
  asm("jmp (%v)", OLDVEC);
}

/*
  Display List Interrupt -- toggle font for a line
*/
void dli(void) {
  asm("pha");
  asm("txa");
  asm("pha");
  asm("tya");
  asm("pha");

  /* Set ANTIC.chbase ahead 2 pages, to utilize the
     other half of the character set... */
  asm("lda %w", (unsigned)&OS.chbas);
  asm("adc #2");
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.chbase);

  /* ...for one GRAPHICS 2 line... */
  asm("lda %w", (unsigned)&OS.chbas);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.wsync);

  /* ...then set it back to normal for the rest of the screen */
  asm("sta %w", (unsigned)&ANTIC.chbase);

  asm("pla");
  asm("tay");
  asm("pla");
  asm("tax");
  asm("pla");

  asm("rti");
}

#pragma optimize (pop);


/* Enable VBI */
void enable_vbi(void) {
  OLDVEC = OS.vvblkd;

  OS.critic = 1;
  OS.vvblkd = (void *) vbi;
  OS.critic = 0;

  ANTIC.nmien = NMIEN_VBI;
}


/* Enable DLI */
void enable_dli(void) {
  ANTIC.nmien = NMIEN_VBI;

  while (ANTIC.vcount < 124);
  OS.vdslst = (void *) dli;
  ANTIC.nmien = NMIEN_VBI | NMIEN_DLI;
}

/* Disable DLI */
void disable_dli(void) {
  ANTIC.nmien = NMIEN_VBI;
}


/*
  Set up the display.
*/
void setup_screen(void) {
  unsigned int dl, crossed, max_vcount;

  /* Start with a full-screen (no text-window) "GRAHICS 1" (20x24)
     display, which we'll modify */

  _graphics(17);

  /* Grab pointers to display list & screen memory */
  dl = PEEKW(560);
  scr_mem = (unsigned char *) PEEKW(88);

  /* All colors to black */
  OS.color0 = 0;
  OS.color1 = 0;
  OS.color2 = 0;
  OS.color3 = 0;
  OS.color4 = 0;

  /* Try to detect PAL vs NTSC ANTIC via VCOUNT test */
  pal_speed = FALSE;
  max_vcount = 0;
  crossed = 0;
  do {
    if (ANTIC.vcount > max_vcount) {
      max_vcount = ANTIC.vcount;
    }
    if (ANTIC.vcount == 0) {
      crossed++;
    }
  } while (crossed < 2);
  if (max_vcount > 133) {
    pal_speed = TRUE;
  }

  /* Set the character set to our redefined font */
  OS.chbas = ((int) iverba2_fnt / 256);

  OS.soundr = 0;

  /* One line of GRAPHICS 2
     (In-game, level & score go here;
     on title screen, game's name goes here) */
  POKE(dl+3,DL_LMS(DL_GRAPHICS2));
  POKEW(dl+4,(unsigned int) scr_mem);

  /* One blank scanline */
  POKE(dl+6,DL_BLK1);

  /* Another line of GRAPHICS 2
     (In-game, the words-per-level meter goes here;
     on title screen, author's name goes here) */
  POKE(dl+7,DL_GRAPHICS2);

  /* Eight blank scanlines (height of GRAPHICS 1) */
  POKE(dl+8,DL_BLK8);

  /* One line of GRAPHICS 1
     (not currently used?) */
  POKE(dl+9,DL_GRAPHICS1);

  /* Eight blank scanlines */
  POKE(dl+10,DL_BLK8);

  /* One line of GRAPHICS 2, which can scroll horizontally
     (for precise centering).
     (In-game, used for words as you type them)
  */
  POKE(dl+11,DL_HSCROL(DL_GRAPHICS2));

  /* One line of GRAPHICS 1, which can scroll horizontally
     (Not used in game). Also, set Load Memory Scan! */
  POKE(dl+12,DL_LMS(DL_HSCROL(DL_GRAPHICS1)));
  POKEW(dl+13,((unsigned int) scr_mem)+(5*20));

  /* One line of GRAPHICS 1.
     (In game, used to show accumulated score for current word.)
     Also, set Load Memory Scan again */
  POKE(dl+15,DL_LMS(DL_GRAPHICS1));
  POKEW(dl+16,((unsigned int) scr_mem)+(7*20));

  /* 18 blank scalines */
  POKE(dl+18,DL_BLK8);
  POKE(dl+19,DL_BLK8);

  /* One line of GRAPHICS 1 */
  POKE(dl+20,DL_GRAPHICS1);

  /* One blank scanline */
  POKE(dl+21,DL_DLI(DL_BLK1));

  /* One line of GRAPHICS 2 */
  POKE(dl+22,DL_GRAPHICS2);

  /* Default scroll setting */
  SCROLL(15);
}


/* Draw bare minimum of title screen */
void title(void) {
  /* Blank the screen */
  bzero(scr_mem, 20 * 24);

  /* Draw title, author, version */
  myprint(1, 0, "INVENIES VERBA 2.0");
  myprint(1, 1, "bill kendrick 2021");
  myprint(10 - strlen(VERSION) / 2, 3, VERSION);

  /* Based-on... */
  myprint(4, 5, "based on lex");
  myprint(1, 7, "by simple machine");
}


void show_help(void) {
  /* Blank the screen */
  bzero(scr_mem, 20 * 24);

  SCROLL(0);

  /* Version # & date */
  myprint(10 - strlen(VERSION) / 2, 0, VERSION);
  myprint(10 - strlen(VERSION_DATE) / 2, 1, VERSION_DATE);

  /* ANTIC or NTSC */
  myprint(5, 2, "ANTIC DETECTED");
  if (pal_speed) {
    myprint(1, 2, "PAL");
  } else {
    myprint(0, 2, "NTSC");
  }

  /* Gameplay instructions... */

  myprint(3, 3, "use the letters to");
  /* N.B. No row 4 */
  myprint(3, 5, "make words; RETURN");
  /* N.B. No row 6 */
  myprint(5, 7, "to submit");

  myprint(0, 8, "avoid letter timeout");

  /* N.B. No row 9 */

  myprint(1, 10, "BKSPC, DELETE, ESC");
  myprint(3, 11, "to clear word");

  /* Hint about Dark mode toggle: */
  myprint(0, 13, "D toggles dark mode");
  myprint(1, 14, "from title screen");

  /* Wait for keypress */
  OS.ch = 255;
  do {
  } while (OS.ch == 255);
  OS.ch = 255;

  SCROLL(15);
}

/*
  Cache scores
  (Make it easy to look up letter scores using their ASCII values)

  @sideeffect Fills global array `ltr_scores[]`
*/
void cache_scores(void) {
  int j, l;

  for (j = 0; j < 26; j++) {
    ltr_scores[j] = 0;
  }

  for (j = 0; j < num_letters; j++) {
    l = (lookups[(j * 2)]) - 'a';
    ltr_scores[l] = lookups[(j * 2) + 1];
  }
}

void allocfail(void) {
  myprint(0, 9, "not enough ram");
  do { } while(1);
}

char * spinner = "I/-\\";

/*
  Load dictionary from disk

  @sideeffect sets global `num_words` to number of words in dictionary
  @sideeffect sets global `dict_ptr_mid` to `num_words / 2`
  @sideeffect sets global `wordlen` to max size of words in dictionary
  @sideeffect sets global `half_wordlen` to `wordlen / 2`
  @sideeffect sets global `num_letters` to number of letters used by words in dictionary
  @sideeffect fills global array `ltr_scores[]` (via `cache_scores()`)
  @sideeffect allocates space for global string `grabbed_word`
  @sideeffect allocates space for global string `best_word`
  @sideeffect allocates space for global string `input`
  @sideeffect allocates space for global string `avail`
  @sideeffect allocates space for global BOOL array `used`
  @sideeffect allocates space for global char array `src`
  @sideeffect allocates space for global int array `mad`
  @sideeffect allocates space for, and fills, global string `blank`
  @sideeffect allocates space for, and fills, global char array `words` (the dictionary buffer), or displays error & infinite-loops
  @sideeffect allocates space for, and fills, char array `lookups`
  @sideeffect displays how many words, and word size, on the screen
*/
void load_dict(void) {
  unsigned char res;
  int alloc, i, size, tot;
  unsigned char * dest;

/* FIXME: Allow dictionary selection */
/*
130 TRAP 30000:DIM FN$(16):OPEN #%1,4,%0,"D:DEFAULT.DAT":INPUT #%1,FN$:CLOSE #%1
135 POKE 731,255:OPEN #%1,4,%0,FN$
*/

  res = disk_open("D:EN_US.DIC", IOCB_READ);

  if (res != 1) {
    sprintf(tmp_msg, "error %d", res);
    myprint(10 - strlen(tmp_msg) / 2, 8, tmp_msg);
    myprint(2, 9, "can't open dict");
    OS.color4 = 64;
    do { } while(1);
  }

  myprint(6, 9, "preping");

  /* Grab details of the dictionary: how many words, how big are words? */
  disk_read((unsigned char *) &num_words, 2);
  dict_ptr_mid = num_words / 2;

  disk_read((unsigned char *) &wordlen, 1);
  half_wordlen = wordlen / 2;

  /* Allocate space for things based on the word length dictated by the
     dictionary being used */
  grabbed_word = (char *) malloc((wordlen + 1) * sizeof(char));
  best_word = (char *) malloc((wordlen + 1) * sizeof(char));
  input = (char *) malloc((wordlen + 1) * sizeof(char));
  avail = (char *) malloc((wordlen + 1) * sizeof(char));
  used = (BOOL *) malloc((wordlen) * sizeof(BOOL));
  src = (unsigned char *) malloc((wordlen) * sizeof(char));
  mad = (unsigned int *) malloc((wordlen) * sizeof(int));

  /* Allocate space for `blank` string & fill it, too */
  blank = (char *) malloc((wordlen + 1) * sizeof(char));
  for (i = 0; i < wordlen; i++) {
    blank[i] = ' ';
  }
  blank[wordlen] = '\0';

  /* Show stats on the title */
  sprintf(tmp_msg, "%d %d-letter words", num_words, wordlen);
  myprint(0, 8, tmp_msg);

  /* Grab how many letters used in the dictionary & alloc space */
  disk_read((unsigned char *) &num_letters, 1);
  lookups = (char *) malloc((num_letters * 2) * sizeof(char));

  /* Allocate space for the dictionary (this is the big one!) */
  alloc = num_words * half_wordlen;

  words = (char *) malloc(alloc * sizeof(char));
  if (words == NULL) {
    allocfail();
  }

  /* Read the letters and their scores; make a reverse-look-up */
  disk_read((unsigned char *) lookups, num_letters * 2);
  cache_scores();

  /* Load dictionary (in 8 chunks, so we can show meters) */
  myprint(6, 9, "loading");

  dest = words;
  size = alloc / 8;
  tot = 0;
  for (i = 0; i < 8; i++) {
    scr_mem[9 * 20 + 5] = spinner[i % 4] - 32;
    scr_mem[9 * 20 + 13] = spinner[i % 4] - 32;

    disk_read(dest, size);

    dest = dest + size;
    tot = tot + size;
    if (tot + size > alloc) {
      size = alloc - tot;
    }
  }

  disk_close();
}

/*
  Load high score

  @sideeffect fills global long `hiscore`, or shows a "no high score file" message momentarily
*/
void load_high_score(void) {
  unsigned char res;

  res = disk_open("D:HISCORE.DAT", IOCB_READ);

  if (res == 1) {
    disk_read((unsigned char *) &hiscore, 3);
  } else {
    myprint(1, 9, "no high score file");
    iv2_sleep(50);
    bzero(scr_mem + 9 * 20, 20);
  }

  disk_close();
}

/*
  Save high score

  @sideeffect displays message showing success or failure of high score saving
*/
void save_high_score(void) {
  unsigned char * ptr;
  FILE * fi;

  disable_dli();

  /* FIXME: Replace with CIO calls? */
  fi = fopen("HISCORE.DAT", "wb");
  if (fi != NULL) {
    ptr = (unsigned char *) &hiscore;
    fputc(ptr[0], fi);
    fputc(ptr[1], fi);
    fputc(ptr[2], fi);

    fclose(fi);

    enable_dli();
    myprint(2, 10, "high score saved");
    iv2_sleep(50);
  } else {
    enable_dli();
    myprint(2, 10, "cannot save high");
    iv2_sleep(50);
  }
}


/*
  Convert an available letter's "madness" value to
  an animated flame character to display on screen.

  @param unsigned char madness -- how 'mad' the letter is (0 to 8)
  @param int offset -- an offset (to avoid all animations lining up)
  @return char the caller should display
*/
unsigned char get_mad_sym(unsigned char madness, int offset) {
  if (madness == 0) {
    return(0);
  } else {
    return((1 + (madness - 1) * 3) + (((OS.rtclok[2] >> 2) + offset) % 3));
  }
}


/*
  Show available letters

  @sideeffect displays available letters and their 'madness' meters
*/
void show_avail(void) {
  int x, i;
  char c, mad_sym;

  x = 10 - ((wordlen / 2) * 2);

  for (i = 0; i < wordlen; i++) {
    c = avail[i];
    if (used[i]) {
      c = c - 32; /* change color */
    }

    mad_sym = get_mad_sym(mad[i] >> 8, i);

    sprintf(tmp_msg, "%c%c", mad_sym + ' ', c);
    myprint(x, 9, tmp_msg);

    sprintf(tmp_msg, "%2d", ltr_scores[avail[i] - 'a']);
    myprint(x, 8, tmp_msg);

    x += 2;
  }
}

/*
  Deal letters -- for any available letter that's not set,
  set it.  While doing so, ensure a certain percentage of
  vowels are in the list.

  @sideeffect sets letters within global char array `avail`, as appropriate
  @sideeffect zeros corresponding elements within int array `mad`
  @sideeffect shows available letters (by calling `show_avail()`)
*/
void deal_letters(void) {
  int i, j, vowels, z;
  char c;
  BOOL need_vowel, is_vowel, retry;

  vowels = 0;

  for (i = 0; i < wordlen; i++) {
    c = avail[i];
    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {
      vowels++;
    }
  }

  need_vowel = (((vowels * 4) / wordlen) < 1);

  for (i = 0; i < wordlen; i++) {
    if (avail[i] == ' ') {

      do {
        z = POKEY_READ.random % num_letters;
        c = lookups[z * 2];
        is_vowel = (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');

        retry = (need_vowel && !is_vowel);
        for (j = 0; j < wordlen; j++) {
          if (avail[j] == c) {
            retry = TRUE;
          }
        }
      } while (retry);

      avail[i] = c;
      mad[i] = 0;

      if (is_vowel) {
        vowels++;
        need_vowel = (((vowels * 4) / wordlen) < 1);
      }
    }
  }

  show_avail();
}

/*
  Game start -- init a new a game.

  @sideeffect sets global BOOL `gameover` to FALSE
  @sideeffect sets global `level` to 1
  @sideeffect zeroes global `score`
  @sideeffect sets global `got_high_score` to FALE
  @sideeffect blanks global string `best_word`
  @sideeffect zeroes global `best`
  @sideeffect clears the screen
  @sideeffect zeroes global `entry_len`
  @sideeffect copies white-space padded `blank` string to into global `input`
  @sideeffect fills global string `avail` with dealt letters (via `deal_letters()`)
  @sideeffect zeros corresponding elements within int array `mad` (via `deal_letters()`)
  @sideeffect fills global BOOL array `used` with FALSE
  @sideeffect shows available letters (by calling `show_avail()`) (via `deal_letters()`)
*/
void game_start(void) {
  int i;

  gameover = FALSE;
  note1_ptr = 0;
  level = 1;
  score = 0;
  got_high_score = FALSE;
  best_word[0] = '\0';
  best = 0;
  bzero(scr_mem, 20 * 24);
  entry_len = 0;
  strcpy(input, blank);
  strcpy(avail, blank);
  for (i = 0; i < wordlen; i++) {
    used[i] = FALSE;
  }

  deal_letters();
  /* N.B.: deal_letters() already calls show_avail() */

  OS.ch = 255;
}

/*
  Level start -- start of a new level

  @sideeffect sets global `level` to 15 if it exceeds it
  @sideeffect changes color palette
  @sideeffect displays level number
  @sideeffect zeroes global `word_cnt`
  @sideeffect displays empty word count meter
*/
void level_start(void) {
  /* Cap out at 15 */
  if (level > 15) {
    level = 15;
  }

  /* Each level uses a different color from the Atari's palette of 15 hues (not counting black/greys/white) */
  if (dark) {
    OS.color4 = (level << 4) + 2;
  } else {
    OS.color4 = (level << 4) + 14;
  }
  OS.color1 = (level << 4) + 8;

  /* Display the level number aka multiplier at the top left */
  sprintf(tmp_msg, "%dX", level);
  myprint(0, 0, tmp_msg);

  /* Reset word count */
  word_cnt = 0;
  memset(scr_mem + 20, 3 + 128, 20);

  iv2_sleep(1);
}


/*
  Calculate score for a word

  @sideeffect updates global `cur_score`
  @sideeffect updates global `bonus`
  @sideeffect displays current word's score calculations
*/
int cur_score = 0, bonus = 0; /* Used outside the function! */

void calc_score(void) {
  int i, add;

  /* Zero calculations; blank out the message showing the old score */
  cur_score = 0;
  bonus = 0;
  bzero(scr_mem + 20 * 7, 20);

  /* Tally */
  for (i = 0; i < entry_len; i++) {
    add = ltr_scores[input[i] - 'a'] * level;

    if (i < 5) {
      cur_score += add;
    } else {
      bonus += (add * 2);
    }
  }

  /* Display the new score */
  if (cur_score > 0) {
    if (bonus > 0) {
      sprintf(tmp_msg, "+%d +%d", cur_score, bonus);
    } else {
      sprintf(tmp_msg, "+%d", cur_score);
    }

    myprint(10 - strlen(tmp_msg) / 2, 7, tmp_msg);
  }
}


/*
  Draw the current word (including fine-scrolling horizontally
  to ensure centering) entered by the user, found in global string
  `input`.

  @sideeffect displays the current word
  @sideeffect affects hardware scroll register
  @sideeffect updates global `cur_score` (via `calc_score()`)
  @sideeffect updates global `bonus` (via `calc_score()`)
  @sideeffect displays current word's score calculations (via `calc_score()`)
*/
void draw_word(void) {
  bzero(scr_mem + 3 * 20, 20);
  SCROLL(15 - (entry_len % 2) * 3);
  
  myprint(10 - entry_len / 2, 3, input);
  calc_score();
}


/*
  Show score (& maintain high score)

  @sideeffect updates global `hiscore` as appropriate
  @sideeffect updates global `got_high_score` as appropriate
  @sideeffect displays scores
*/
void show_score(void) {
  /* Track high score */
  if (score > hiscore) {
    hiscore = score;
    got_high_score = TRUE;
  }

  /* Draw score & high score */
  sprintf(tmp_msg, "%ld/%ld", score, hiscore);
  myprint(5, 0, tmp_msg);
}


/*
  Handle keypresses during the game.

  * [Backspace] - Delete a letter from current word input
  * [Sh]+[Backspace] or [Esc] - Delete entire current word input

  @sideeffect plays sounds
  @sideeffect alters horizontal scroll register
  @sideeffect flashes colors 
  @sideeffect updates global string `input`
  @sideeffect updates global `entry_len`
  @sideeffect updates global BOOl array `used`
  @sideeffect updates global char array `src`
  @sideeffect updates global char array `avail` 
  @sideeffect updates global `word_cnt`
*/
void pressed_a_key(void) {
  unsigned char ch, c, pick;
  int i, add;

  /* Grab a key, if any */
  ch = OS.ch;
  OS.ch = 255;

  if (ch == KEY_DELETE && entry_len > 0) {
    /* [Backspace] - Delete a letter */
    note1_ptr = ((note1_ptr - 1) % (sizeof(note_table) - 8));
    play_chord(FALSE);

    entry_len--;

    used[src[entry_len]] = FALSE;
    src[entry_len] = SRC_NOT_SET;
    input[entry_len] = '\0';

    draw_word();
    show_avail();

    SOUND(3, 0, 0, 0);
  }

  if (ch == KEY_RETURN && entry_len >= 3) {
    /* [Return] - Submit the word */
    SOUND(3, 0, 0, 15);
    iv2_sleep(1);
    SOUND(3, 0, 0, 0);

    if (binsearch(input)) {
      /* Valid word! */

      /* Increment score */
      add = cur_score + bonus;
      score = score + add;
      show_score();

      /* Track our best word */
      if (add > best) {
        best = add;
        strcpy(best_word, input);
      }

      /* Blank the word input */
      input[0] = '\0';
      for (i = 0; i < entry_len; i++) {
        avail[src[i]] = ' ';
        used[src[i]] = FALSE;
        src[i] = SRC_NOT_SET;
      }
      entry_len = 0;

      /* Get more letters & show them */
      deal_letters();

      /* Erase word & score calculations */
      draw_word();

      /* Add to word count meter */
      word_cnt++;
      if (word_cnt < 3) {
        memset(scr_mem + 20, 3, word_cnt * 7);
      } else {
        memset(scr_mem + 20, 3, 20);
      }

      /* Play a happy sound */
  
      note1_ptr = ((note1_ptr + 1) % (sizeof(note_table) - 8));
      play_chord(TRUE);
      note1_ptr = note_starts[word_cnt];
    } else {
      /* Invalid word! */

      /* Red screen & obnoxious sound */
      OS.color4 = 34;
      play_chord(FALSE);

      /* "Shake" the word they input */
      for (i = 0; i < 10; i++) {
        SCROLL((POKEY_READ.random % 3) + 12);
        iv2_sleep(1);
      }

      /* Reset scroll & colors, and silence sounds */
      SCROLL(15 - (entry_len % 2) * 3);
      if (dark) {
        OS.color4 = (level << 4) + 2;
      } else {
        OS.color4 = (level << 4) + 14;
      }
    }
  }

  /* Anything else (only bother if there's room for more letters) */
  if (entry_len < wordlen) {
    /* Use the OS's look-up table to see if it's an A-Z key */
    c = kbcode_to_atascii[ch];

    if (c >= 'a' && c <= 'z') {
      /* See if that letter is in the available letters list (currently dealt),
         AND not already used in the current input word */
      pick = SRC_NOT_SET;
      for (i = 0; i < wordlen; i++) {
        if (avail[i] == c && used[i] == FALSE) {
          pick = i;
        }
      }

      if (pick != SRC_NOT_SET) {
        /* Valid letter! */

        /* Add to the end of the input word */
        input[entry_len] = c;
        input[entry_len + 1] = '\0';
  
        /* Denote that the letter was used; track the source of each letter
           (map letters in the input to the list of available letters dealt to us) */
        used[pick] = TRUE;
        src[entry_len] = pick;

        /* Track input word length */
        entry_len++;

        /* Update the word, and available letters, accordingly */
        draw_word();
        show_avail();

        note1_ptr = ((note1_ptr + 1) % (sizeof(note_table) - 8));
        play_chord(TRUE);
      } else {
        /* Invalid (or already used) letter! */

        play_chord(FALSE);
      }
    }
  }

  if ((ch == KEY_ESC || ch == (KEY_DELETE | KEY_SHIFT)) && entry_len > 0) {
    /* [Esc] or [Shift] + [Backspace] (aka [Delete]): Delete entire word */
    SOUND(3, 200, 10, 8);

    /* Blank the input */
    input[0] = '\0';
    entry_len = 0;

    /* Mark all available letters as 'unused', and un-map them */
    for (i = 0; i < wordlen; i++) {
      used[i] = FALSE;
      src[i] = SRC_NOT_SET;
    }

    /* Update the (now blank) word, and available letters, accordingly */
    draw_word();
    show_avail();

    SOUND(3, 0, 0, 0);
  }
}


/* FIXME: Here be dragons that need fine-tuning -bjk 2021.07.16 */

#define ANTI_SPEED 32 /* the higher the number, the slower things increase */

/*
  Increase values of, and show animated flame meters, denoting
  how 'mad' each letter is

  @sideeffect updates elements of global `mad[]` array, as appropriate
  @sideeffect displays meters
*/
void show_meters(void) {
  int x, i, s, m;

  x = 10 - half_wordlen * 2;

  for (i = 0; i < wordlen; i++) {
    s = ltr_scores[avail[i] - 'a'];

    if (pal_speed) {
      /* PAL; clicks less often, so add madness more quickly */
      mad[i] += (((11 - s) << 8) * level) / (ANTI_SPEED * 50);
    } else {
      /* NTSC; clicks more often, so add madness less quickly */
      mad[i] += (((11 - s) << 8) * level) / (ANTI_SPEED * 60);
    }

    m = mad[i] >> 8;
    if (m > 8) {
      m = 8;
      gameover = TRUE;
    }

    scr_mem[9 * 20 + x] = get_mad_sym(m, i);

    x += 2;
  }
}

/*
  Game loop!

  Play the game; accept input, increment letter meters.

  @sideeffects sets global BOOL `gameover` if the game has ended
  @sideeffects <many others not listed here>
*/
void game_loop(void) {
  BOOL next_level;

  next_level = FALSE;
  do {
    /* Handle keypresses */
    if (OS.ch != 255) {
      pressed_a_key();
    }

    /* Every few screen refreshes, update meters
       (if any meter has filled completely, the game ends
       and we'll drop out of this loop and return to caller) */
    if (OS.rtclok[2] & 2) {
      show_meters();
    }

    /* If word count has hit three, progress to the next level
       (we'll drop out of this loop and return to caller) */
    if (word_cnt == 3) {
      level++;
      next_level = TRUE;
      note1_ptr = 0;
    }
  } while (!gameover && !next_level);
}


/*
. Game over sequence -- Show best word, save high score (if needed)
*/
void game_over(void) {
  int i;

  /* Reset scroll register */
  SCROLL(15);

  /* Show the best word they got during this game... */
  bzero(scr_mem + 3 * 20, 20);
  if (best_word[0] != '\0') {
    sprintf(tmp_msg, "best word: %s", best_word);
  } else {
    /* Player was, like, just sitting there...!? */
    sprintf(tmp_msg, "try that again");
  }
  myprint(10 - (strlen(tmp_msg) / 2), 3, tmp_msg);

  /* ...and the best word's score */
  bzero(scr_mem + 5 * 20, 20);
  if (best > 0) {
    sprintf(tmp_msg, "%d points", best);
    myprint(10 - (strlen(tmp_msg) / 2), 5, tmp_msg);
  }

  /* Congratulate them if they got the high score;
     save the new high score to disk */
  if (got_high_score) {
    myprint(3, 7, "new high score");
    save_high_score();
  }

  /* Game-over sound effect */
  sound_timer = 0;
  SOUND(0, 0, 0, 0);
  SOUND(1, 0, 0, 0);
  SOUND(2, 0, 0, 0);

  for (i = 0; i < 254; i += 16) {
    SOUND(3, i, 10, 4);
    iv2_sleep(1);
  }
  SOUND(3, 0, 0, 0);

  /* Prompt them to press [Esc] to continue
     (any console key works, too) */
  myprint(2, 15, "PRESS ESCAPE...");

  OS.ch = 255;
  while (OS.ch != KEY_ESC && GTIA_READ.consol == 7) { }

  /* Stay here until console key released, if it was pressed! */
  do { } while (GTIA_READ.consol != 7);
  OS.ch = 255;
}

void show_high_score(void) {
  sprintf(tmp_msg, "highscore=%ld", hiscore);
  myprint(10 - strlen(tmp_msg) / 2, 15, tmp_msg);
}

void show_title_prompts(void) {
  myprint(4, 9, "press START");
  myprint(0, 13, "press HELP for more");
}

/* Main! */
void main(void) {
  int i;

  /* Point to the XL OS's keyboard code -> ATASCII look-up table */
  kbcode_to_atascii = (char *) OS.keydef;

  /* Init sound registers */
  SOUND_INIT();
  
  /* Set up display & show bare-bones title */
  setup_screen();
  title();

  /* Fade-in effect */
  for (i = 0; i < 15; i++) {
    if (!dark) {
      OS.color4 = 16 + i; // => 30
    } else {
      OS.color4 = 16 + (i / 4);
    }
    OS.color1 = 16 + (i / 2); // => 24
    OS.color2 = (i * 7) / 10; // => 10
    OS.color3 = 32 + (i / 7); // => 34
    while (ANTIC.vcount < 124);
    while (ANTIC.vcount < 124);
  }

  /* Load dictionary & high score */
  load_high_score();
  show_high_score();
  load_dict();

  enable_vbi();
  enable_dli();

  /* Main loop: */
  do {
    /* Prompt user to press [Start] to begin, or help */
    show_title_prompts();

/* FIXME */
/*
196 DT=INSTR(FN$,"."):POSITION %0,12:? #6;"option - change lang";"current=";FN$(%3,DT-%1)
*/

    /* Show current high score
       (might update within this loop!) */
    show_high_score();

    do {
      /* [D] key toggles dark mode */
      if (OS.ch == KEY_D) {
        play_chord(dark);
        dark = !dark;
        OS.ch = 255;

        if (dark) {
          OS.color4 = (1 << 4) + 2;
        } else {
          OS.color4 = (1 << 4) + 14;
        }
      } else if (OS.ch == KEY_H || OS.ch == KEY_HELP) {
        show_help();
        title();
        show_title_prompts();
        show_high_score();
      }
    } while(GTIA_READ.consol != 6);


    /* Start a new game */ 
    game_start();

    /* Game loop: */  
    do {
      /* Start a new level */
      level_start();

      /* Play the level */
      game_loop();
    } while (!gameover);

    /* Show game over sequence */
    game_over();

    /* Show title screen again.
       (Yes, this is our second call to this within main(), but after the
       first one, we load the dictionary from disk, which is not something
       we need to do more than once) */
    title();
  } while (1);
}

/* FIXME: Port this to C */
/*
. Choose a dictionary!
30000 GRAPHICS 18:DIM FNS$(160):CLOSE #%1:DICTS=%0:? #6;"`select a language`"
30005 TRAP 30020:OPEN #%1,6,%0,"D:*.DIC"
30010 INPUT #%1,FN$:IF FN$(11,13)="DIC" THEN FNS$(DICTS*8+%1)=FN$(3,10):DICTS=DICTS+%1
30015 GOTO 30010
30020 CLOSE #%1:X=%0:Y=%1
30030 FOR I=0 TO DICTS-1
30040   POSITION X,Y:? #6;CHR$(97+I);" ";:? #6;FNS$(I*8+%1,I*8+8)
30060   Y=Y+%1:IF Y=12 THEN Y=%1:X=X+10
30070 NEXT I
30080 TRAP 30080:POKE 731,%0:GET C:I=C-65:FN$="D:":FN$(%3)=FNS$(I*8+%1,I*8+8):SP=INSTR(FN$," "):FN$(SP)=".DIC"
30090 OPEN #%1,8,%0,"D:DEFAULT.DAT":?#%1;FN$:CLOSE #%1:RUN
*/

