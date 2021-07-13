/*
  iverba2.c
  "Invenies Verba" for the Atari 8-bit

  By Bill Kendrick <bill@newbreedsoftware.com>

  * Original TurboBASIC XL code:
    August - September 2017

  * cc65 port:
    June 30, 2021 - July 13, 2021
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <atari.h>
#include <peekpoke.h>
#include "sound.h"

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

void sleep(int i) {
  unsigned char target;

  target = OS.rtclok[2] + i;
  do { } while (OS.rtclok[2] != target);
}

void SCROLL(unsigned char h) {
  unsigned char r;

  r = OS.rtclok[2];
  while (OS.rtclok[2] == r) {};

  ANTIC.hscrol = h;
}

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
. Grab the 'A'th word
20 P=ADR(W$)+(A*H_WORDLEN):LADR=ADR(L$):AADR=ADR(A$):FOR I=%0 TO H_WORDLEN-%1:W=PEEK(P+I)
25 B1=W&15:B2=(W&240)/16:DPOKE AADR+I*%2,PEEK(LADR+B1*%2)+PEEK(LADR+B2*%2)*256:NEXT I:RETURN
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
. Binary-search for word 'I$'
*/
unsigned char binsearch(char * str) {
  char padded_str[9];
  int cut, cursor, smallstep_tries, i, cmp;
  BOOL found;

/*
30 CUT=MID*%2:A=MID:FOUND=%0:WHILE LEN(I$)<WORDLEN:I$(LEN(I$)+1)=" ":WEND:LAST=%0
*/

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

/*
40 GOSUB 20:IF A$=I$ OR LAST>10 THEN FOUND=(A$=I$):RETURN
50 CUT=INT(CUT/%2):IF CUT=%0 THEN CUT=%1:LAST=LAST+%1
60 IF A$<I$:A=A+CUT:IF A>=NUMWORDS:A=NUMWORDS-%1:ENDIF:ELSE:A=A-CUT:ENDIF
70 GOTO 40
*/

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

/*
Display List Interrupts
*/
#pragma optimize (push, off)
void dli(void)
{
  asm("pha");
  asm("txa");
  asm("pha");
  asm("tya");
  asm("pha");

  asm("lda %w", (unsigned)&OS.chbas);
  asm("adc #2");
  asm("sta %w", (unsigned)&ANTIC.wsync);
  asm("sta %w", (unsigned)&ANTIC.chbase);

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
  asm("sta %w", (unsigned)&ANTIC.chbase);

  asm("pla");
  asm("tay");
  asm("pla");
  asm("tax");
  asm("pla");

  asm("rti");
}
#pragma optimize (pop);


/*
. Set up screen
*/
void setup_screen(void) {
  unsigned int dl;

/*
110 GRAPHICS 17:DL=DPEEK(560):SC=DPEEK(88):POKE 712,30:POKE 708,0:POKE 709,24:POKE 710,15:POKE 711,34
112 POKE 65,%0
*/
  _graphics(17);
  dl = PEEKW(560);
  scr_mem = (unsigned char *) PEEKW(88);

  OS.color4 = 30;
  OS.color0 = 0;
  OS.color1 = 24;
  OS.color2 = 15;
  OS.color3 = 34;

  OS.chbas = ((int) iverba2_fnt / 256);

  OS.soundr = 0;

/*
115 POKE DL+%3,7+64:DPOKE DL+4,SC:POKE DL+6,%0:POKE DL+7,7:POKE DL+8,112:POKE DL+9,6:POKE DL+10,112
*/
  POKE(dl+3,DL_LMS(DL_GRAPHICS2));
  POKEW(dl+4,(unsigned int) scr_mem);
  POKE(dl+6,DL_BLK1);
  POKE(dl+7,DL_GRAPHICS2);
  POKE(dl+8,DL_BLK8);
  POKE(dl+9,DL_GRAPHICS1);
  POKE(dl+10,DL_BLK8);

/*
116 POKE DL+11,7+16:POKE DL+12,6+16+64:DPOKE DL+13,SC+(5*20):POKE DL+15,6+64:DPOKE DL+16,SC+(7*20)
*/
  POKE(dl+11,DL_HSCROL(DL_GRAPHICS2));
  POKE(dl+12,DL_LMS(DL_HSCROL(DL_GRAPHICS1)));
  POKEW(dl+13,((unsigned int) scr_mem)+(5*20));
  POKE(dl+15,DL_LMS(DL_GRAPHICS1));
  POKEW(dl+16,((unsigned int) scr_mem)+(7*20));

/*
117 POKE DL+18,112:POKE DL+19,112:POKE DL+20,6:POKE DL+21,%0:POKE DL+22,7:POKE 54276,15
*/
  POKE(dl+18,DL_BLK8);
  POKE(dl+19,DL_BLK8);
  POKE(dl+20,DL_GRAPHICS1);
  POKE(dl+21,DL_DLI(DL_BLK1));
  POKE(dl+22,DL_GRAPHICS2);

  SCROLL(15);


  ANTIC.nmien = NMIEN_VBI;
  while (ANTIC.vcount < 124);
  OS.vdslst = (void *) dli;
  ANTIC.nmien = NMIEN_VBI | NMIEN_DLI;
}

void title(void) {
  bzero(scr_mem, 20 * 24);

/*
120 ? #6;" INVENIES VERBA 1.0":? #6;" `bill kendrick {#18}{#16}{#17}{#20}`":? #6
*/
  myprint(1, 0, "INVENIES VERBA 2.0");
  myprint(1, 1, "bill kendrick 2021");
/*
125 ? #6;"  based on lex for":? #6:? #6;"iphone {#6} android{#12} by":? #6;" simple machine llc"
*/
  myprint(4, 3, "based on lex");
  myprint(0, 5, "for iphone & android");
  myprint(1, 7, "by simple machine");
  myprint(4, 12, "PRERELEASE 1");
  myprint(5, 13, "2021-07-11");
}


/*
. Make it easy to look up letter scores using ASCII value
1150 PROC CACHE_SCORES
1155   DIM LTR_SCORES(26)
1160   FOR J=%1 TO NUMLETTERS-%1
1170     L=ASC(L$(J*%2+%1,J*%2+%1)):L=L-ASC("a")
1180     LTR_SCORES(L)=ASC(L$(J*%2+%2,J*%2+%2))
1190   NEXT J
1195 ENDPROC
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

/*
. Load dictionary
*/
void load_dict(void) {
  FILE * fi;
  int alloc, i;

/* FIXME: Allow dictionary selection */
/*
130 TRAP 30000:DIM FN$(16):OPEN #%1,4,%0,"D:DEFAULT.DAT":INPUT #%1,FN$:CLOSE #%1
135 POKE 731,255:OPEN #%1,4,%0,FN$
*/
  fi = fopen("EN_US.DIC", "rb");
  /* FIXME: Check for error */

/*
140 GET #%1,NUMWORDS_L:GET #1,NUMWORDS_H:NUMWORDS=(NUMWORDS_H*256)+NUMWORDS_L:MID=INT(NUMWORDS/%2)
150 GET #%1,WORDLEN:H_WORDLEN=WORDLEN/%2:DIM A$(WORDLEN),I$(WORDLEN),BEST$(WORDLEN),AVAIL$(WORDLEN),BLANK$(20),C$(%1)
151 DIM USED(WORDLEN),SRC(WORDLEN)
*/
  num_words = fgetc(fi) + fgetc(fi) * 256;
  dict_ptr_mid = num_words / 2;

  wordlen = fgetc(fi);
  half_wordlen = wordlen / 2;

  grabbed_word = (char *) malloc((wordlen + 1) * sizeof(char));
  best_word = (char *) malloc((wordlen + 1) * sizeof(char));
  input = (char *) malloc((wordlen + 1) * sizeof(char));
  avail = (char *) malloc((wordlen + 1) * sizeof(char));
  used = (BOOL *) malloc((wordlen) * sizeof(BOOL));
  src = (unsigned char *) malloc((wordlen) * sizeof(char));
  mad = (unsigned int *) malloc((wordlen) * sizeof(int));

/*
155 BLANK$=" ":BLANK$(20)=" ":BLANK$(2)=BLANK$:A$=BLANK$
*/
  blank = (char *) malloc((wordlen + 1) * sizeof(char));
  for (i = 0; i < wordlen; i++) {
    blank[i] = ' ';
  }
  blank[wordlen] = '\0';

/*
160 ? #6;NUMWORDS;" ";WORDLEN;"-letter words"
*/
  sprintf(tmp_msg, "%d %d-letter words", num_words, wordlen);
  myprint(0, 8, tmp_msg);

/*
170 ALLOC=NUMWORDS*H_WORDLEN:DIM W$(ALLOC):W$=" ":W$(ALLOC-%1)=" ":W$(%2)=W$
*/
  alloc = num_words * half_wordlen;

  if (_heapmaxavail() < alloc) {
    myprint(0, 9, "not enough ram");
    do { } while(1);
  }

  words = (char *) malloc(alloc * sizeof(char));

/*
180 GET #%1,NUMLETTERS:DIM L$(NUMLETTERS*%2):L$(NUMLETTERS*2)="{heart}":BGET #%1,ADR(L$),NUMLETTERS*%2:EXEC CACHE_SCORES
*/
  num_letters = fgetc(fi);
  lookups = (char *) malloc((num_letters * 2) * sizeof(char));
  fread(lookups, sizeof(char), num_letters * 2, fi);
  cache_scores();

/* 
185 POSITION 5,9:? #6;"loading...":BGET #%1,ADR(W$),ALLOC:CLOSE #%1
*/
  myprint(5, 9, "loading...");

  fread(words, sizeof(char), alloc, fi);

  fclose(fi);
}

/*
. Load high score
186 TRAP 190:OPEN #%1,4,%0,"D:HISCORE.DAT":INPUT #%1,HISCORE:CLOSE #%1
*/
void load_high_score(void) {
  unsigned char * ptr;
  FILE * fi;

  fi = fopen("HISCORE.DAT", "rb");
  if (fi != NULL) {
    ptr = (unsigned char *) &hiscore;
    ptr[0] = fgetc(fi);
    ptr[1] = fgetc(fi);
    ptr[2] = fgetc(fi);

    fclose(fi);
  } else {
    myprint(1, 9, "no high score file");
    sleep(50);
    bzero(scr_mem + 9 * 20, 20);
  }
}

/*
. Save high score
2030 TRAP 2040:CLOSE #%1:OPEN #%1,8,%0,"D:HISCORE.DAT":? #%1;HISCORE:CLOSE #%1
*/
void save_high_score(void) {
  unsigned char * ptr;
  FILE * fi;

  fi = fopen("HISCORE.DAT", "wb");
  if (fi != NULL) {
    ptr = (unsigned char *) &hiscore;
    fputc(ptr[0], fi);
    fputc(ptr[1], fi);
    fputc(ptr[2], fi);

    fclose(fi);

    myprint(2, 10, "high score saved");
    sleep(50);
  } else {
    myprint(2, 10, "cannot save high");
    sleep(50);
  }
}


/*
. More set-up and create font
190 DIM MAD$(9),MAD(WORDLEN):MAD$=" `{^a}{^b}{^d}{^e}{^g}{^h}{^i}{^j}`":CH=(PEEK(106)-16)*256:MOVE 224*256,CH,1024
191 MOVE ADR("{#0}{#0}{#0}{#0}{#0}{#0}{#0}{#3}{#0}{#0}{#0}{#0}{#0}{#0}{#3}{#3}{#85}{#170}{#85}{#170}{#85}{#170}{#85}{#170}"),CH+8,24
192 MOVE ADR("{#0}{#0}{#0}{#0}{#0}{#3}{#3}{#3}{#0}{#0}{#0}{#0}{#3}{#3}{#3}{#3}"),CH+32,16
193 MOVE ADR("{#0}{#0}{#0}{#3}{#3}{#3}{#3}{#3}{#0}{#0}{#3}{#3}{#3}{#3}{#3}{#3}{#0}{#3}{#3}{#3}{#3}{#3}{#3}{#3}{#3}{#3}{#3}{#3}{#3}{#3}{#3}{#3}"),CH+56,32
*/

char mad_symbols[] = " !\"$%'()*";

/*
. Show available letters
1300 PROC SHOW_AVAIL
*/
void show_avail(void) {
  int x, i;
  char c;

/*
1305   X=10-((WORDLEN/%2)*%2)
*/
  x = 10 - ((wordlen / 2) * 2);

/*
1310   FOR I=%1 TO WORDLEN
1320     C$=AVAIL$(I,I):C=ASC(C$)
1330     IF USED(I) THEN C$=CHR$(C-32)
*/

  for (i = 0; i < wordlen; i++) {
    c = avail[i];
    if (used[i]) {
      c = c - 32; /* change color */
    }

/*
1340     POSITION X,9:M=MAD(I)+%1:?#6;MAD$(M,M);C$
*/
    sprintf(tmp_msg, "%c%c", mad_symbols[mad[i] >> 8], c);
    myprint(x, 9, tmp_msg);

/*
1350     S=LTR_SCORES(C-ASC("a")):POSITION X+(S<10),8:?#6;S
1360     IF S<10 THEN ?#6;" "
*/
    sprintf(tmp_msg, "%2d", ltr_scores[avail[i] - 'a']);
    myprint(x, 8, tmp_msg);

/*
1370     X=X+%2
1380   NEXT I
1390 ENDPROC
*/
    x += 2;
  }
}

/*
. Deal letters
1200 PROC DEAL_LETTERS
*/
void deal_letters(void) {
  int i, j, vowels, z;
  char c;
  BOOL need_vowel, is_vowel, retry;

/*
. Make sure we always have some vowels
1201   VOWELS=%0
*/
  vowels = 0;

/*
1202   FOR I=1 TO WORDLEN
1203     C$=AVAIL$(I,I)
1204     IF C$="a" OR C$="e" OR C$="i" OR C$="o" OR C$="u" THEN VOWELS=VOWELS+%1
1205   NEXT I
*/
  for (i = 0; i < wordlen; i++) {
    c = avail[i];
    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {
      vowels++;
    }
  }

/*
1206   NEED_VOWEL=((VOWELS/WORDLEN)<0.25)
*/
  need_vowel = (((vowels * 4) / wordlen) < 1);

/*
1210   FOR I=1 TO WORDLEN
1220     IF AVAIL$(I,I)=" "
*/
  for (i = 0; i < wordlen; i++) {
    if (avail[i] == ' ') {

/*
1230       REPEAT
1231         Z=RAND((LEN(L$)/%2)-%1)+%1:C$=L$(Z*%2+%1,Z*%2+%1):IS_VOWEL=(C$="a" OR C$="e" OR C$="i" OR C$="o" OR C$="u")
*/
      do {
        z = POKEY_READ.random % num_letters;
        c = lookups[z * 2];
        is_vowel = (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');

/*      
1232         RETRY=((NEED_VOWEL=%1 AND (NOT IS_VOWEL))+(INSTR(AVAIL$, C$)<>0))
1233 .          RETRY=%0
1234 .          FOR J=%1 TO WORDLEN
1235 .            IF AVAIL$(J,J)=C$ THEN RETRY=%1
1236 .          NEXT J
1238 .        ENDIF
*/

        retry = (need_vowel && !is_vowel);
        for (j = 0; j < wordlen; j++) {
          if (avail[j] == c) {
            retry = TRUE;
          }
        }
/*
1239       UNTIL RETRY=%0
*/
      } while (retry);

/*
1240       AVAIL$(I,I)=C$:MAD(I)=%0
*/
      avail[i] = c;
      mad[i] = 0;

/*
1245       IF (IS_VOWEL) THEN VOWELS=VOWELS+1:NEED_VOWEL=((VOWELS/WORDLEN)<0.25)
*/
      if (is_vowel) {
        vowels++;
        need_vowel = (((vowels * 4) / wordlen) < 1);
      }
    }
  }
/*
1250     ENDIF
1260   NEXT I
1270   EXEC SHOW_AVAIL
1280 ENDPROC
*/
  show_avail();
}

/*
. Game start
*/
void game_start(void) {
  int i;

/*
300 GAMEOVER=%0:LEVEL=%1:SCORE=%0:BEST$="":BEST=%0:? #6;"{clear}";:ENTRY_LEN=%0:I$=BLANK$
*/
  gameover = FALSE;
  level = 1;
  score = 0;
  got_high_score = FALSE;
  best_word[0] = '\0';
  best = 0;
  bzero(scr_mem, 20 * 24);
  entry_len = 0;
  strcpy(input, blank);

/*
310 AVAIL$=BLANK$:FOR I=%1 TO WORDLEN:USED(I)=%0:NEXT I:EXEC DEAL_LETTERS:EXEC SHOW_AVAIL
*/
  strcpy(avail, blank);
  for (i = 0; i < wordlen; i++) {
    used[i] = FALSE;
  }

  deal_letters();
  /* N.B.: deal_letters() already calls show_avail() */
}

/*
. Level start
*/
void level_start(void) {
/*
400 IF LEVEL>15 THEN LEVEL=15
410 POKE 712,(LEVEL*16)+14:POKE 709,(LEVEL*16)+8:POSITION %0,%0:?#6;LEVEL;"X":EXEC SHOW_SCORE
*/
  if (level > 15) {
    level = 15;
  }
  OS.color4 = (level << 4) + 14;
  OS.color1 = (level << 4) + 8;
  sprintf(tmp_msg, "%dX", level);
  myprint(0, 0, tmp_msg);

/*
420 WORD_CNT=%0:COLOR %3+128:PLOT %0,%1:DRAWTO 19,%1
*/
  word_cnt = 0;
  memset(scr_mem + 20, 3, 20);

/*
430 PAUSE %1
*/
  sleep(1);
}


/*
. Calculate score
1030 PROC CALC_SCORE
*/
int cur_score = 0, bonus = 0; /* Used outside the function! */

void calc_score(void) {
  int i, add;

/*
1040   CUR_SCORE=%0:BONUS=%0:POSITION %0,7:?#6;BLANK$
*/
  cur_score = 0;
  bonus = 0;
  bzero(scr_mem + 20 * 7, 20);

/*
1045   IF ENTRY_LEN>%0
1050     FOR I=1 TO ENTRY_LEN
*/
  for (i = 0; i < entry_len; i++) {
/*
1060       ADD=LTR_SCORES(ASC(I$(I,I))-ASC("a"))*LEVEL
*/
    add = ltr_scores[input[i] - 'a'] * level;

/*
1070       IF I<5:CUR_SCORE=CUR_SCORE+ADD:ELSE:BONUS=BONUS+ADD*%2:ENDIF
*/
    if (i < 5) {
      cur_score += add;
    } else {
      bonus += (add * 2);
    }


/*
1080     NEXT I
*/
  }

/*
1100     POSITION 8,7:?#6;"+";CUR_SCORE;
1110     IF BONUS>0 THEN ?#6;" +";BONUS
*/
  if (cur_score > 0) {
    if (bonus > 0) {
      sprintf(tmp_msg, "+%d +%d", cur_score, bonus);
    } else {
      sprintf(tmp_msg, "+%d", cur_score);
    }

    myprint(10 - strlen(tmp_msg) / 2, 7, tmp_msg);
  }

/*
1120   ENDIF
1130 ENDPROC
*/
}


/*
. Draw word (including fine-scrolling horizontally)
*/
void draw_word(void) {
  /*
1010 PROC DRAW_WORD:POSITION %0,%3:?#6;BLANK$:POKE 54276,15-(ENTRY_LEN MOD %2)*%3
1020 POSITION 10-(ENTRY_LEN/%2),%3:?#6;I$:EXEC CALC_SCORE:ENDPROC
  */

  bzero(scr_mem + 3 * 20, 20);
  SCROLL(15 - (entry_len % 2) * 3);
  
  myprint(10 - entry_len / 2, 3, input);
  calc_score();
}


/*
. Show score (& maintain high score)
1000 PROC SHOW_SCORE
*/
void show_score(void) {
/*
1001   IF SCORE>HISCORE THEN HISCORE=SCORE
*/
  if (score > hiscore) {
    hiscore = score;
    got_high_score = TRUE;
  }

/*
1005   POSITION 5,%0:?#6;SCORE;"/";HISCORE
1009 ENDPROC
*/

  sprintf(tmp_msg, "%ld/%ld", score, hiscore);
  myprint(5, 0, tmp_msg);
}



/*
.     Pressed a key
*/
void pressed_a_key(void) {
  unsigned char ch, c, pick;
  int i, add;

/*
510   GET C
*/
  ch = OS.ch;
  OS.ch = 255;

/*
520   IF C=126 AND ENTRY_LEN>%0
.       Backspace
*/
  if (ch == KEY_DELETE && entry_len > 0) {
/*
525     SOUND %0,145,10,8
*/
    SOUND(0, 145, 10, 8);

/*
530     USED(SRC(ENTRY_LEN))=%0:SRC(ENTRY_LEN)=-%1:I$(ENTRY_LEN,ENTRY_LEN)=" ":ENTRY_LEN=ENTRY_LEN-%1
*/
    entry_len--;

    used[src[entry_len]] = FALSE;
    src[entry_len] = SRC_NOT_SET;
    input[entry_len] = '\0';

/*
540     EXEC DRAW_WORD:EXEC SHOW_AVAIL
*/
    draw_word();
    show_avail();
/*
545     SOUND %0,%0,%0,%0
550   ENDIF
*/
    SOUND(0, 0, 0, 0);
  }

/*
560   IF C=155 AND ENTRY_LEN>=%3
.       Return
*/
  if (ch == KEY_RETURN && entry_len >= 3) {
/*
565     SOUND %0,%0,%0,15:PAUSE %1:SOUND %0,%0,%0,%0
*/
    SOUND(0, 0, 0, 15);
    sleep(1);
    SOUND(0, 0, 0, 0);

/*
570     GOSUB 30
580     IF FOUND
*/

    if (binsearch(input)) {

/*
581       ADD=CUR_SCORE+BONUS:SCORE=SCORE+ADD:EXEC SHOW_SCORE
*/
      add = cur_score + bonus;
      score = score + add;
      show_score();

/*
582       IF ADD>BEST:BEST=ADD:BEST$=I$:ENDIF
*/
      if (add > best) {
        best = add;
        strcpy(best_word, input);
      }

/*
583       FOR I=%1 TO ENTRY_LEN:I$(I,I)=" ":AVAIL$(SRC(I),SRC(I))=" ":USED(SRC(I))=%0:SRC(I)=-%1:NEXT I
*/
      input[0] = '\0';
      for (i = 0; i < entry_len; i++) {
        avail[src[i]] = ' ';
        used[src[i]] = FALSE;
        src[i] = SRC_NOT_SET;
      }

/*
584       ENTRY_LEN=%0:EXEC DEAL_LETTERS:EXEC DRAW_WORD
*/
      entry_len = 0;
      deal_letters();
      draw_word();

/*
585       WORD_CNT=WORD_CNT+%1:COLOR %3:PLOT %0,%1:DRAWTO 6*WORD_CNT,%1:FOR A=15 TO %0 STEP -%1:SOUND %0,61,10,A:NEXT A
*/
      word_cnt++;
      memset(scr_mem + 20, 3, word_cnt * 6);
      for (i = 0; i < 16; i++) {
        SOUND(0, 61, 10, 15 - i);
        sleep(1);
      }

/*
586     ELSE
*/
    } else {
/*
587       POKE 712,34:SOUND %0,220,12,10:FOR I=%0 TO 10:POKE 54276,RAND(%3)+12:PAUSE %1:NEXT I:POKE 54276,15-(ENTRY_LEN MOD %2)*%3:POKE 712,(LEVEL*16)+14:SOUND %0,%0,%0,%0
*/
      OS.color4 = 34;
      SOUND(0, 220, 12, 10);

      for (i = 0; i < 10; i++) {
        SCROLL((POKEY_READ.random % 3) + 12);
        sleep(1);
      }

      SCROLL(15 - (entry_len % 2) * 3);
      OS.color4 = (level << 4) + 14;
      SOUND(0, 0, 0, 0);

/*
588     ENDIF
589   ENDIF
*/
    }
  }

/*
590   IF C>=ASC("A") AND C<=ASC("Z") AND ENTRY_LEN<WORDLEN
.       A-Z
*/
  if (entry_len < wordlen) {
    c = kbcode_to_atascii[ch];

    if (c >= 'a' && c <= 'z') {
/*
600     C=C!32:C$=CHR$(C)
605     PICK=%0
610     FOR I=%1 TO WORDLEN
620       IF AVAIL$(I,I)=C$ AND USED(I)=%0 THEN PICK=I
650     NEXT I
*/

      pick = SRC_NOT_SET;
      for (i = 0; i < wordlen; i++) {
        if (avail[i] == c && used[i] == FALSE) {
          pick = i;
        }
      }

/*
660     IF PICK>%0
*/
      if (pick != SRC_NOT_SET) {
/*
665       SOUND %0,61,10,8
670       ENTRY_LEN=ENTRY_LEN+%1:I$(ENTRY_LEN,ENTRY_LEN)=C$:USED(PICK)=%1:SRC(ENTRY_LEN)=PICK
*/
        SOUND(0, 61, 10, 8);

        input[entry_len] = c;
        input[entry_len + 1] = '\0';
  
        used[pick] = TRUE;
        src[entry_len] = pick;
  
        entry_len++;

/*
675       SOUND %0,%0,%0,%0
680       EXEC DRAW_WORD:EXEC SHOW_AVAIL
*/
        draw_word();
        show_avail();
        SOUND(0, 0, 0, 0);
      } else {
/*
690     ELSE
695       SOUND %0,255,10,8:PAUSE %2:SOUND %0,%0,%0,%0
700     ENDIF
*/
        SOUND(0, 255, 10, 8);
        sleep(2);
        SOUND(0, 0, 0, 0);
      }
    }

/*
710   ENDIF
*/
  }

/*
711   IF (C=27 OR C=156) AND ENTRY_LEN>%0
.       Delete word (Esc or Shift+Bksp)
*/
  if ((ch == KEY_ESC || ch == (KEY_DELETE | KEY_SHIFT)) && entry_len > 0) {
/*
712     FOR I=%1 TO ENTRY_LEN:I$(I,I)=" ":USED(SRC(I))=%0:SRC(I)=-%1:NEXT I
713     ENTRY_LEN=%0:EXEC DRAW_WORD:EXEC SHOW_AVAIL
715   ENDIF
720 ENDIF
*/
    SOUND(0, 200, 10, 8);

    input[0] = '\0';
    for (i = 0; i < wordlen; i++) {
      used[i] = FALSE;
      src[i] = SRC_NOT_SET;
    }
    entry_len = 0;
    draw_word();
    show_avail();

    SOUND(0, 0, 0, 0);
  }
}


/*
. Show meters
*/
void show_meters(void) {
  int x, i, s, m;

/*
800 X=10-((WORDLEN/%2)*%2)
*/

  x = 10 - half_wordlen * 2;

/*
810 FOR I=%1 TO WORDLEN
*/
  for (i = 0; i < wordlen; i++) {
/*
820   S=LTR_SCORES(ASC(AVAIL$(I,I))-ASC("a"))
*/
    s = ltr_scores[avail[i] - 'a'];
/*    
830   MAD(I)=MAD(I)+((11-S)/(200-(LEVEL*4)))
*/
    mad[i] += ((11 - s) << 8) / (1280 - (level * 8)); /* TODO Fine tune */

/*    
840   POSITION X,9:M=MAD(I)+%1:IF M>9:M=9:GAMEOVER=1:ENDIF
850   ?#6;MAD$(M,M):X=X+%2
*/
    m = mad[i] >> 8;
    if (m > 8) {
      m = 8;
      gameover = TRUE;
    }

    scr_mem[9 * 20 + x] = mad_symbols[m] - 32;

    x += 2;
/*
860 NEXT I
*/
  }
}


/*
. Game loop!
*/
void game_loop(void) {
  BOOL next_level;

  next_level = FALSE;
  do {
  /*
  500 IF PEEK(764)<255
  */
    if (OS.ch != 255) {
      pressed_a_key();
    }
    show_meters();

/*
. End-of-game or end-of-level checks
980 IF GAMEOVER THEN 2000
990 IF WORD_CNT=%3 THEN LEVEL=LEVEL+%1:GOTO 400
999 GOTO 500
*/
    if (word_cnt == 3) {
      level++;
      next_level = TRUE;
    }
  } while (!gameover && !next_level);
}


/*
. Game over; show best word, save high score
*/
void game_over(void) {
  int i;

/*
2000 POSITION %0,%3:?#6;BLANK$:POKE 54276,15
2010 POSITION %0,%3:?#6;"`best word:`";BEST$
*/
  SCROLL(15);

  bzero(scr_mem + 3 * 20, 20);
  if (best_word[0] != '\0') {
    sprintf(tmp_msg, "best word: %s", best_word);
  } else {
    sprintf(tmp_msg, "try that again");
  }
  myprint(10 - (strlen(tmp_msg) / 2), 3, tmp_msg);

/*
2020 POSITION %0,5:?#6;BLANK$:POSITION %0,5:?#6;BEST
*/
  bzero(scr_mem + 5 * 20, 20);
  if (best > 0) {
    sprintf(tmp_msg, "%d points", best);
    myprint(10 - (strlen(tmp_msg) / 2), 5, tmp_msg);
  }

  if (got_high_score) {
    save_high_score();
    myprint(3, 7, "new high score");
  }

/*
2040 PAUSE %1:POKE 764,255
2050 POSITION %0,7:?#6;BLANK$:POSITION %3,7:?#6;"PRESS A KEY...";
2060 IF PEEK(764)=255 AND PEEK(53279)=7 THEN 2060
2070 POKE 764,255:GOTO 300
*/

  for (i = 0; i < 254; i += 16) {
    SOUND(0, i, 10, 4);
    sleep(1);
  }
  SOUND(0, 0, 0, 0);

  myprint(2, 15, "PRESS ESCAPE...");

  OS.ch = 255;
  while (OS.ch != KEY_ESC && GTIA_READ.consol == 7) { }
  do { } while (GTIA_READ.consol != 7);
  OS.ch = 255;
}



void main(void) {
  kbcode_to_atascii = (char *) OS.keydef;
  SOUND_INIT();
  
  setup_screen();
  title();
  load_dict();
  load_high_score();

  do {
/*
195 POKE 756,CH/256:POSITION %3,9:? #6;"press start..."
*/

    myprint(3, 9, "press start...");

/* FIXME */
/*
196 DT=INSTR(FN$,"."):POSITION %0,12:? #6;"option - change lang";"current=";FN$(%3,DT-%1)
*/

/*
197 POSITION %0,15:? #6;"hiscore=";HISCORE
*/
    sprintf(tmp_msg, "highscore=%ld", hiscore);
    myprint(10 - strlen(tmp_msg) / 2, 15, tmp_msg);

/*
199 IF PEEK(53279)=%3 THEN 30000
*/
/* FIXME */

/*
200 IF PEEK(53279)<>6 THEN 199
*/

    do {
/* FIXME: Do something interesting */
    } while(GTIA_READ.consol != 6);
  
    game_start();
  
    do {
      level_start();
      game_loop();
    } while (!gameover);

    game_over();
    title();
  } while (1);

  ANTIC.nmien = NMIEN_VBI;
}

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
