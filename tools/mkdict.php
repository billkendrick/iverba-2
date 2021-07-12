#!/usr/bin/php
<?php

/* See 'wordlist' package for available language lists */

if ($argc < 3) {
  echo "Usage: mkdict.php INFILE OUTFILE MAX_WORDLEN\n";
  echo "  (MAX_WORDLEN must be even)\n";
  echo "  Example: mkdict.php /usr/share/dict/american-english-large en_us.dic 8\n\n";
  exit(1);
}
$INFILE = $argv[1]; // "/usr/share/dict/american-english-large";

$MAX_LETTERS = 15; /* No more than 15 (we pack two letters into one byte, and need a blank character, too!) */
$MAX_WORDLEN = intval($argv[3]); /* Should be even */
if ($MAX_WORDLEN < 2 || $MAX_WORDLEN > 8 || ($MAX_WORDLEN % 2) <> 0) {
  echo "MAX_WORDLEN must be 2, 4, 6 or 8\n";
  echo "You entered: ".$argv[3]."\n";
  exit (1);
}

$OUTFILE = $argv[2];

echo "Reading: $INFILE\n";
echo "Max letters available: $MAX_LETTERS\n";
echo "Max word len: $MAX_WORDLEN\n";
echo "Writing: $OUTFILE\n";
echo "\n";

$cmd = "cat $INFILE";
$cmd .= ' | grep "^[abcdefghijklmnopqrstuvwxyz]\{3,' . $MAX_WORDLEN . '\}$"';

echo $cmd."\n";
$pi = popen($cmd, "r");
if ($pi === false) {
  echo "Error opening grep!\n";
  exit(1);
}

$letters = array();
for ($i = ord("a"); $i <= ord("z"); $i++) {
  $letters[chr($i)] = 0;
}

$total_letters = 0;

while (!feof($pi)) {
  $w = trim(fgets($pi));
  if (!feof($pi)) {
    for ($i = 0; $i < strlen($w); $i++) {
      $c = substr($w, $i, 1);
      $letters[$c]++;
      $total_letters++;
    }
  }
}
arsort($letters);

for ($i = 0; $i < 26 - $MAX_LETTERS; $i++) {
  array_pop($letters);
}

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

echo "min=$min, max=$max\n";

$letter_scores = array();

echo "Letters:\n";
foreach ($letters as $k=>$v) {
  $score = floor((($max / 3) - (floor(($v * 100) / $total_letters) / 3)));
  if ($score < 1) $score = 1;
  $letter_scores[$k] = $score;
  echo "  $k ($v)=>" . $score . " ";
  echo "\n";
}

$LETTERS = "";
foreach ($letters as $k=>$v) {
  $LETTERS .= $k;
}

echo "Letters: $LETTERS\n";

$pi = popen('grep "^[' . $LETTERS . ']\{3,' . $MAX_WORDLEN . '\}$" '.$INFILE, "r");
if ($pi === false) {
  echo "Error opening grep of $INFILE!\n";
  exit(1);
}
$words = array();
while (!feof($pi)) {
  $word = trim(fgets($pi));

  if (!feof($pi)) {
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

  for ($i = 0; $i < $MAX_WORDLEN + $BUF; $i = $i + 2) {
    $n1 = ch2n(substr($w, $i, 1));
    $n2 = ch2n(substr($w, $i + 1, 1));

    $b = ($n1 + ($n2 << 4));

    // echo substr($w, $i, 2) . " => $n1,$n2 => $b\n";

    fwrite($fo, chr($b));
  }
}

fclose($fo);

/*
$fo = fopen("lexwords.txt", "w");
fwrite($fo, print_r($words, true));
fclose($fo);
*/

function ch2n($c) {
  global $LETTERS;

  if ($c == " ") {
    return(0);
  } else {
    $n = 0;
    for ($i = 0; $i < strlen($LETTERS); $i++) {
      if ($c == substr($LETTERS, $i, 1)) {
        $n = ($i + 1);
      }
    }
    return($n);
  }
}
?>
