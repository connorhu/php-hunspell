--TEST--
Dictionary::getEncoding / getWordChars / getVersion return strings
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

$enc = $dict->getEncoding();
var_dump(is_string($enc));
var_dump(strlen($enc) > 0);
var_dump(stripos($enc, 'utf') !== false);

$wc = $dict->getWordChars();
var_dump(is_string($wc));

$ver = $dict->getVersion();
var_dump(is_string($ver));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
