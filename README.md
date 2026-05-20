# php-hunspell

[![CI](https://github.com/connorhu/php-hunspell/actions/workflows/ci.yml/badge.svg)](https://github.com/connorhu/php-hunspell/actions/workflows/ci.yml)

PHP 8.2+ binding for the [Hunspell](https://hunspell.github.io/) spell-checker and morphological analyzer. Exposes spell-check, suggestions, morphological analysis (`analyze`, `stem`, `generate`), runtime dictionary mutation (`add`, `addWithAffix`, `remove`, `addDictionary`), and dictionary metadata.

## Requirements

- PHP 8.2 or newer (NTS or ZTS)
- libhunspell 1.6.0 or newer + its development headers
- pkg-config
- A C++17-capable compiler

## Install via PIE (recommended)

```sh
pie install nepenektar/php-hunspell
```

## Install via phpize

```sh
git clone https://github.com/connorhu/php-hunspell.git
cd php-hunspell
phpize
./configure --enable-hunspell
make
sudo make install
echo "extension=hunspell.so" | sudo tee /etc/php/conf.d/hunspell.ini
```

## Usage

```php
<?php
$dict = new Hunspell\Dictionary('/path/to/hu_HU.aff', '/path/to/hu_HU.dic');

$dict->spell('szótár');                   // bool
$dict->suggest('szóter');                 // list<string>
$dict->stem('szótárak');                  // list<string>
$dict->analyze('szótárak');               // list<Hunspell\Analysis>

$a = $dict->analyze('szótárak')[0];
$a->getRaw();                             // raw morph string
$a->getFields();                          // array<string, list<string>>
$a->get('st');                            // stems (list<string>)
$a->has('hy');                            // bool — does this analysis carry a hyphenation hint?

$dict->generate('ház', 'kertben');        // generate form of ház matching kertben's morphology
$dict->addWithAffix('Németh', 'világ');   // proper noun inherits világ's affix flags
```

## Error handling

```php
try {
    $dict = new Hunspell\Dictionary($aff, $dic);
} catch (Hunspell\Exception\Exception $e) {
    // Any Hunspell-layer failure (currently only DictionaryLoadException)
}
```

## License

PHP License 3.01.
