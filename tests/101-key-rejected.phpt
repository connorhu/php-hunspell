--TEST--
Constructor with non-null $key raises InvalidArgumentException
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
try {
    $dict = new Hunspell\Dictionary("$base.aff", "$base.dic", 'secret');
    echo "no throw\n";
} catch (\InvalidArgumentException $e) {
    echo "rejected: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
rejected: Encrypted dictionary keys are not supported in this release
