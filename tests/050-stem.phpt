--TEST--
Dictionary::stem returns stems for inflected words
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

$s = $dict->stem('worlds');
var_dump(in_array('world', $s, true));

var_dump($dict->stem('xyzzy'));
?>
--EXPECT--
bool(true)
array(0) {
}
