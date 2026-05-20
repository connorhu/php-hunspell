--TEST--
Hunspell\Dictionary class is registered and final
--EXTENSIONS--
hunspell
--FILE--
<?php
var_dump(class_exists('Hunspell\\Dictionary'));
$r = new ReflectionClass('Hunspell\\Dictionary');
var_dump($r->isFinal());
var_dump($r->getNamespaceName());
?>
--EXPECT--
bool(true)
bool(true)
string(8) "Hunspell"
