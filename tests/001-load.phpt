--TEST--
Dictionary::__construct loads or throws DictionaryLoadException
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';

/* success */
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");
var_dump($dict instanceof Hunspell\Dictionary);

/* failure: missing files */
try {
    new Hunspell\Dictionary('/nonexistent.aff', '/nonexistent.dic');
    echo "no throw\n";
} catch (Hunspell\Exception\DictionaryLoadException $e) {
    echo "load-fail: " . get_class($e) . "\n";
}

/* failure: non-null $key */
try {
    new Hunspell\Dictionary("$base.aff", "$base.dic", 'somekey');
    echo "no throw\n";
} catch (\InvalidArgumentException $e) {
    echo "key-rejected\n";
}
?>
--EXPECT--
bool(true)
load-fail: Hunspell\Exception\DictionaryLoadException
key-rejected
