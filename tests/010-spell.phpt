--TEST--
Dictionary::spell returns bool for known and unknown words
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");
var_dump($dict->spell('hello'));    /* true */
var_dump($dict->spell('worlds'));   /* true via SFX A */
var_dump($dict->spell('xyzzy'));    /* false */
?>
--EXPECT--
bool(true)
bool(true)
bool(false)
