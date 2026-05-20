--TEST--
Dictionary::analyzeRaw returns raw morphology strings
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

$r = $dict->analyzeRaw('szótár');
var_dump(is_array($r));
var_dump(count($r) >= 1);

$found = false;
foreach ($r as $line) {
    if (str_contains($line, 'st:szótár') && str_contains($line, 'po:noun')) {
        $found = true;
    }
}
var_dump($found);

/* unknown word: empty array */
var_dump($dict->analyzeRaw('xyzzy'));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
array(0) {
}
