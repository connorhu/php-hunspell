--TEST--
Dictionary::add, addWithAffix, remove update the in-memory handle
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

var_dump($dict->spell('foobar'));      /* false */

$dict->add('foobar');
var_dump($dict->spell('foobar'));      /* true */

$dict->remove('foobar');
var_dump($dict->spell('foobar'));      /* false */

/* addWithAffix: inherit "world"'s affix group A → "newword" + "newwords" both spell */
$dict->addWithAffix('newword', 'world');
var_dump($dict->spell('newword'));     /* true */
var_dump($dict->spell('newwords'));    /* true via SFX A */
?>
--EXPECT--
bool(false)
bool(true)
bool(false)
bool(true)
bool(true)
