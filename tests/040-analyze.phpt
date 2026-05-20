--TEST--
Dictionary::analyze returns parsed Hunspell\Analysis objects
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

$analyses = $dict->analyze('szótár');
var_dump(is_array($analyses));
var_dump(count($analyses) >= 1);

$a = $analyses[0];
var_dump($a instanceof Hunspell\Analysis);
var_dump($a->get('st'));
var_dump($a->get('po'));
var_dump($a->has('hy'));

/* unknown word: empty */
var_dump($dict->analyze('xyzzy'));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
array(1) {
  [0]=>
  string(8) "szótár"
}
array(1) {
  [0]=>
  string(4) "noun"
}
bool(true)
array(0) {
}
