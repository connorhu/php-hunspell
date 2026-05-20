--TEST--
Dictionary::addDictionary stacks an extra .dic, fails loudly on bad path
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

var_dump($dict->spell('extraword'));   /* false initially */

$dict->addDictionary(__DIR__ . '/data/extra.dic');
var_dump($dict->spell('extraword'));   /* true after add */

try {
    $dict->addDictionary('/nonexistent.dic');
    echo "no throw\n";
} catch (Hunspell\Exception\DictionaryLoadException $e) {
    echo "add-dic-fail\n";
}
?>
--EXPECT--
bool(false)
bool(true)
add-dic-fail
