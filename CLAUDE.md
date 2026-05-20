# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A PHP extension (written in C) that exposes the [Hunspell](https://hunspell.github.io/) spell-checking library to PHP userland as a `hunspell` class. The code is built from PHP's `ext_skel` skeleton and targets the **PHP 5 / Zend Engine 2** C API — it uses `TSRMLS_CC`, `zend_object_store_get_object`, the older `RETURN_STRING(s, dup)` two-arg form, `zend_hash_copy` for default properties, etc. It will **not** compile against PHP 7+ without porting (zend_object embedding, removal of TSRM macros, new ZPP string handling).

External dependency: `libhunspell` with headers at `<prefix>/include/hunspell/hunspell.h`. `config.m4` searches `/usr/local` then `/usr` by default, or honors `--with-hunspell=<path>`.

## Build & test

This is a standard phpize-built shared extension. From the repo root:

```sh
phpize
./configure --with-hunspell           # or --with-hunspell=/opt/local etc.
make
make test                              # runs run-tests.php against tests/*.phpt
make install                           # installs hunspell.so into extension_dir
```

Run a single phpt test:

```sh
make test TESTS=tests/001.phpt
```

To use the built extension without installing, point PHP at the local build:

```sh
php -d extension=$(pwd)/modules/hunspell.so -r 'var_dump(extension_loaded("hunspell"));'
```

Cleaning between builds: `phpize --clean` (removes generated autoconf scaffolding) followed by re-running `phpize`.

## Architecture

Everything lives in three files:

- **`hunspell.c`** — all PHP method implementations, the module entry, and the object lifecycle hooks.
- **`php_hunspell.h`** — declares `ze_hunspell_object`, the C struct backing each PHP `hunspell` instance. It embeds a `zend_object zo` (PHP-5 style: zo is the *first* field, not a trailing `zend_object` as in PHP 7+), the `Hunhandle *dic`, and the dictionary/affix path strings.
- **`config.m4`** — autoconf build glue; resolves the libhunspell location and registers the extension via `PHP_NEW_EXTENSION`.

### Object lifecycle (important when modifying methods)

The class is registered in `PHP_MINIT_FUNCTION(hunspell)` with a custom `create_object` handler, `php_hunspell_object_new`, which `emalloc`s the `ze_hunspell_object`, zeroes the embedded `zend_object`, and registers `php_hunspell_object_free_storage` as the destructor. The free-storage callback `efree`s `aff_path`/`dic_path` and calls `Hunspell_destroy` on the live handle.

Every method retrieves the backing struct with `zend_object_store_get_object(getThis() TSRMLS_CC)`. Methods guard on `ze_obj->dic == NULL` and `RETURN_FALSE` if the handle isn't open — preserve this invariant when adding methods.

### Hunspell C API mapping

- `__construct($dic_path, $aff_path)` and `open()` → `Hunspell_create(aff_path, dic_path)`. Note: the *PHP* argument order is **dic first, aff second**, but the underlying `Hunspell_create` expects **aff first, dic second** — the C code swaps them at the call site. Don't "fix" this without also updating the public API.
- `spell($word)` → `Hunspell_spell` (returns int: 0 = misspelled).
- `suggest($word)` → calls `Hunspell_spell` first; returns `false` if the word is correctly spelled, otherwise returns a PHP array built from `Hunspell_suggest`'s `char **slist`.
- `get_encoding()` → `Hunspell_get_dic_encoding`.
- `close()` → `Hunspell_destroy` (does *not* null out `ze_obj->dic`, so the free-storage callback will double-free; consider this when touching close()).

### Known foot-guns in current code

- `spell()` and `suggest()` call `efree(word)` on the buffer returned by `zend_parse_parameters("s", ...)`. ZPP does **not** allocate a fresh copy for `"s"` — the pointer aliases the underlying zval's string. Freeing it corrupts the PHP heap. Any new string-input method should *not* `efree` ZPP-owned strings.
- `suggest()` calls `free(slist)` (libc free) but does not call `Hunspell_free_list(&slist, sl_count)`. This leaks the individual suggestion strings on each call.
- The `hunspell_open_dic` named-function stub at the top of `hunspell.c` is dead code.

## Tests

Tests use PHP's `.phpt` format under `tests/`. Currently only `001.phpt` exists and only verifies that the extension loads — there is no coverage of `spell`/`suggest`/encoding behavior. When adding tests, follow `.phpt` conventions (`--TEST--`, `--SKIPIF--`, `--FILE--`, `--EXPECT--` sections) and gate them on `extension_loaded("hunspell")` in `--SKIPIF--`.
