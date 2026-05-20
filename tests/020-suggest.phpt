--TEST--
Dictionary::suggest returns suggestions, [] for correct words
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

$s = $dict->suggest('helo');
var_dump(is_array($s));
var_dump(in_array('hello', $s, true));

$s2 = $dict->suggest('hello');
var_dump($s2);
?>
--EXPECT--
bool(true)
bool(true)
array(0) {
}
