#!/usr/bin/php
<?php

/* See 'wordlist' package for available language lists */


/* Confirm & read in command-line arguments and set other
   built-in options: */

if ($argc < 4) {
  echo "Usage: mkdict.php INFILE OUTFILE MAX_WORDLEN MAX_LETTERS\n";
  echo "  (MAX_WORDLEN must be even; MAX_LETTERS must be <= 15)\n";
  echo "  Example: mkdict.php /usr/share/dict/american-english-large en_us.dic 8\n\n";
  exit(1);
}

$INFILE = $argv[1];
$OUTFILE = $argv[2];
$MAX_WORDLEN = intval($argv[3]); /* Should be even */
$MAX_LETTERS = intval($argv[4]); /* No more than 15 (we pack two letters into one byte, and need a blank character, too!) */

if ($MAX_WORDLEN < 2 || $MAX_WORDLEN > 8 || ($MAX_WORDLEN % 2) <> 0) {
  echo "MAX_WORDLEN must be 2, 4, 6 or 8\n";
  echo "You entered: '" . $argv[3] . "'\n";
  exit(1);
}

if ($MAX_LETTERS > 15) {
  echo "MAX_LETTERS must be <= 15\n";
  echo "You entered: '" . $argv[4] . "'\n";
  exit(1);
}


/* Display the options we're being run with: */

echo "Reading: $INFILE\n";
echo "Max letters available: $MAX_LETTERS\n";
echo "Max word len: $MAX_WORDLEN\n";
echo "Writing: $OUTFILE\n";
echo "\n";


/* Map characters with diacritics to non-diacritic-having
   versions (e.g., "á" to "a"). */

$ESZETT = "<"; /* next key after the "0" on Atari keyboards */

$diacritic_removals = array(
  "á" => "a",
  "â" => "a",
  "à" => "a",
  "ą" => "a",
  "ć" => "c",
  "ç" => "c",
  "é" => "e",
  "ê" => "e",
  "è" => "e",
  "ë" => "e",
  "ę" => "e",
  "í" => "i",
  "î" => "i",
  "ì" => "i",
  "ï" => "i",
  "ł" => "l",
  "ń" => "n",
  "ñ" => "n",
  "ó" => "o",
  "ô" => "o",
  "ò" => "o",
  "ö" => "o",
  "ő" => "o",
  "ś" => "s",
  "ß" => $ESZETT,
  "ú" => "u",
  "û" => "u",
  "ù" => "u",
  "ü" => "u",
  "ű" => "u",
  "ÿ" => "y",
  "ź" => "z",
  "ż" => "z",
);


/* Create a reverse-map, so once we determine which
   normalized (A-Z, no diacritic) letters to use,
   we can look for words that contain the diacritic
   variations.

   (i.e., if "a" is one of the letters for which we'll
   be including words, we'll also want words with
   "á", "â", etc.)
*/
$normal_to_diacritic = array();
foreach ($diacritic_removals as $dia=>$normal) {
  if (!array_key_exists($normal, $normal_to_diacritic)) {
    $normal_to_diacritic[$normal] = array();
  }
  $normal_to_diacritic[$normal][] = $dia;
}

/* Collect each individual diacritic
   (for our first grep) */
$diacritics_for_grep = implode(array_keys($diacritic_removals));


/* Read all properly-lengthed words -- ignoring uppercase
   to avoid proper nouns (names, places, etc.), punctuation
   (as in contractions like "can't" or possesives like "bulldog's"),
   and other non-alphabetic characters */
$cmd = "cat $INFILE";
$cmd .= ' | grep "^[abcdefghijklmnopqrstuvwxyz' . $diacritics_for_grep . ']\{3,' . $MAX_WORDLEN . '\}$"';

echo $cmd."\n";
$pi = popen($cmd, "r");
if ($pi === false) {
  echo "Error opening grep!\n";
  exit(1);
}


/* First, read every word to determine the frequency
   of the normalized certain letters */

$letters = array();
for ($i = ord("a"); $i <= ord("z"); $i++) {
  $letters[chr($i)] = 0;
}
$letters[$ESZETT] = 0;

$total_letters = 0;

while (!feof($pi)) {
  $w = trim(fgets($pi));
  if (!feof($pi)) {
    for ($i = 0; $i < strlen($w); $i++) {
      $c = substr($w, $i, 1);
    }

    $w = replace_diacritics_with_normalized($w);

    for ($i = 0; $i < strlen($w); $i++) {
      $c = substr($w, $i, 1);
      $letters[$c]++;
      $total_letters++;
    }
  }
}
arsort($letters);

/* Remove the least-common letters, up until the
   max. # of letters we allow.

   (N.B. 27 because we support "a"-"z" plus German eszett ("ß");
   letters with diacrtics have been 'normalized' to "a"-"z") */
for ($i = 0; $i < 27 - $MAX_LETTERS; $i++) {
  array_pop($letters);
}


/* Determine frequencies of the remaining letters we'll use,
   so we can assign scores to them */
$min = 100;
$max = 0;

foreach ($letters as $k=>$v) {
  $pct = floor(($v * 100) / $total_letters);
  if ($pct > $max) {
    $max = $pct;
  }
  if ($pct < $min) {
    $min = $pct;
  }
}


/* Collect (and echo) the letter scores */

$letter_scores = array();

echo "min=$min, max=$max\n";
echo "Letters:\n";

$cnt = 0;
foreach ($letters as $k=>$v) {
  $score = floor((($max / 3) - (floor(($v * 100) / $total_letters) / 3)));
  if ($score < 1) $score = 1;
  $letter_scores[$k] = $score;
  $cnt++;
  echo "  $cnt: $k ($v)=>$score";
  echo "\n";
}


/* Collect all of the letters, plus any diacritical variations,
   for grep'ing the dictionary */
$LETTERS = "";
$LETTERS_AZ = "";
foreach ($letters as $k=>$v) {
  $LETTERS_AZ .= $k;
  $LETTERS .= $k;

  if (array_key_exists($k, $normal_to_diacritic)) {
    foreach ($normal_to_diacritic[$k] as $dia) {
      $LETTERS .= $dia;
    }
  }
}



echo "Letters: $LETTERS\n";
echo "(normalized to: $LETTERS_AZ)\n";

$pi = popen('grep "^[' . $LETTERS . ']\{3,' . $MAX_WORDLEN . '\}$" '.$INFILE, "r");
if ($pi === false) {
  echo "Error opening grep of $INFILE!\n";
  exit(1);
}

$seen_words = array();
$words = array();
while (!feof($pi)) {
  $word = trim(fgets($pi));

  if (!feof($pi)) {
    $word = replace_diacritics_with_normalized($word);

    if (!in_array($word, $seen_words)) {
      $ltr = array();
      $ok = true;
      for ($i = 0; $i < strlen($word); $i++) {
        $c = substr($word, $i, 1);
        if (in_array($c, $ltr)) {
          $ok = false;
        }
        $ltr[] = $c;
      }
  
      if ($ok) {
        $words[] = $word;
      }

      $seen_words[] = $word;
    }
  }
}
pclose($pi);

$num_words = count($words);

echo "Loaded $num_words words\n";

$BUF = ($MAX_WORDLEN % 2);

echo "Buffer = " . $BUF . "\n";
if ($BUF == 1) {
  echo "WASTING SPACE!\n";
}

echo "Total chars (padded) = " . ($num_words * ($MAX_WORDLEN + $BUF)) . "\n";

echo "Bytes = " . ceil(($num_words * ($MAX_WORDLEN + $BUF)) / 2) . "\n";


sort($words);

$fo = fopen($OUTFILE . ".txt", "w");
fprintf($fo, "%s\n", implode("\n", $words));
fclose($fo);


$fo = fopen($OUTFILE, "wb");
if ($fo === false) {
  echo "Error opening $OUTFILE!\n";
  exit(1);
}

fwrite($fo, chr($num_words & 255));
fwrite($fo, chr(($num_words >> 8) & 255));
fwrite($fo, chr($MAX_WORDLEN + $BUF));
fwrite($fo, chr($MAX_LETTERS + 1));
fwrite($fo, " " . chr(0));

foreach ($letter_scores as $l=>$s) {
  fwrite($fo, $l . chr($s));
}

foreach ($words as $w) {
  $w = str_pad($w, $MAX_WORDLEN + $BUF, " ");

//  echo "\n$w\n";

  for ($i = 0; $i < $MAX_WORDLEN + $BUF; $i = $i + 2) {
    $n1 = ch2n(substr($w, $i, 1));
    $n2 = ch2n(substr($w, $i + 1, 1));

    $b = ($n1 + ($n2 << 4));

//    echo substr($w, $i, 2) . " => $n1,$n2 => $b\n";

    fwrite($fo, chr($b));
  }
}

fclose($fo);

echo "\n\n";

function ch2n($c) {
  global $LETTERS_AZ;

  if ($c == " ") {
    return(0);
  } else {
    $n = 0;
    for ($i = 0; $i < strlen($LETTERS_AZ); $i++) {
      if ($c == substr($LETTERS_AZ, $i, 1)) {
        $n = ($i + 1);
      }
    }
    return($n);
  }
}

function replace_diacritics_with_normalized($w) {
  global $diacritic_removals;

  foreach ($diacritic_removals as $dia => $replacement) {
    $w = str_replace($dia, $replacement, $w);
  }
  return ($w);
}

?>
