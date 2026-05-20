--TEST--
Exception interface and DictionaryLoadException are registered
--EXTENSIONS--
hunspell
--FILE--
<?php
var_dump(interface_exists('Hunspell\\Exception\\Exception'));
var_dump(class_exists('Hunspell\\Exception\\DictionaryLoadException'));
$ex = new Hunspell\Exception\DictionaryLoadException('msg');
var_dump($ex instanceof RuntimeException);
var_dump($ex instanceof Hunspell\Exception\Exception);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
