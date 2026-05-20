--TEST--
clone $dict throws Error (clone_obj handler is NULL)
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");
try {
    $clone = clone $dict;
    echo "cloned\n";
} catch (\Error $e) {
    echo "clone-forbidden: " . $e->getMessage() . "\n";
}
?>
--EXPECTF--
clone-forbidden: Trying to clone an uncloneable object of class Hunspell\Dictionary
