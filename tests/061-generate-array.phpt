--TEST--
Dictionary::generate with an array model uses Hunspell_generate2
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

$out = $dict->generate('world', ['is:plural']);
var_dump(is_array($out));
?>
--EXPECT--
bool(true)
