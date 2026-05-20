--TEST--
Dictionary::generate with a string model uses Hunspell_generate
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

/* generate the form of 'world' that matches the morphology of 'worlds' */
$out = $dict->generate('world', 'worlds');
var_dump(is_array($out));
/* For this dict we cannot guarantee a specific output, only that the call works. */
?>
--EXPECT--
bool(true)
