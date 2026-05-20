--TEST--
Dictionary::generate raises TypeError on non-string array element
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

try {
    $dict->generate('world', ['is:plural', 42]);
    echo "no throw\n";
} catch (\TypeError $e) {
    echo "type-error: " . preg_replace('/\s+/', ' ', $e->getMessage()) . "\n";
}
?>
--EXPECTREGEX--
type-error: .*Argument #2 \(\$model\) item #1 must be of type string, int given.*
