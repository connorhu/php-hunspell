--TEST--
new Analysis($raw) parses key:value tokens into $fields
--EXTENSIONS--
hunspell
--FILE--
<?php
$a = new Hunspell\Analysis('st:szótár po:noun ts:NOM al:szótárak hy:3');

var_dump($a->getRaw());
var_dump($a->getFields());
var_dump($a->get('po'));
var_dump($a->has('hy'));
var_dump($a->has('zz'));

/* repeated key collects into list */
$b = new Hunspell\Analysis('st:foo st:bar po:noun');
var_dump($b->get('st'));

/* malformed tokens are silently skipped */
$c = new Hunspell\Analysis('justaword po:noun  ');
var_dump($c->getFields());
?>
--EXPECT--
string(45) "st:szótár po:noun ts:NOM al:szótárak hy:3"
array(5) {
  ["st"]=>
  array(1) {
    [0]=>
    string(8) "szótár"
  }
  ["po"]=>
  array(1) {
    [0]=>
    string(4) "noun"
  }
  ["ts"]=>
  array(1) {
    [0]=>
    string(3) "NOM"
  }
  ["al"]=>
  array(1) {
    [0]=>
    string(10) "szótárak"
  }
  ["hy"]=>
  array(1) {
    [0]=>
    string(1) "3"
  }
}
array(1) {
  [0]=>
  string(4) "noun"
}
bool(true)
bool(false)
array(2) {
  [0]=>
  string(3) "foo"
  [1]=>
  string(3) "bar"
}
array(1) {
  ["po"]=>
  array(1) {
    [0]=>
    string(4) "noun"
  }
}
