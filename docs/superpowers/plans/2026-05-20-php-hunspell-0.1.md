# php-hunspell 0.1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the legacy PHP 5–era `hunspell.c` extension with a modern PHP 8.2+ C++17 extension that exposes the full libhunspell P0+P1 API as `Hunspell\Dictionary` and `Hunspell\Analysis`, ships with day-0 PIE distribution metadata and GitHub Actions CI (NTS / ZTS / macOS / ASAN).

**Architecture:** Single PHP extension named `hunspell`, written in C++17, built with `phpize` and a `config.m4` that uses `PKG_CHECK_MODULES([HUNSPELL], [hunspell >= 1.6.0])`. Two PHP classes (`Hunspell\Dictionary` for the binding, `Hunspell\Analysis` as a readonly value object for parsed morphological analyses). One marker interface plus one concrete exception class. Multi-file C++ layout (`hunspell.cc`, `dictionary.cc`, `analysis.cc`, `exceptions.cc`).

**Tech Stack:** C++17, libhunspell ≥ 1.6.0, PHP 8.2+, phpize, pkg-config, PIE, GitHub Actions, phpt test format.

**Spec reference:** `docs/superpowers/specs/2026-05-20-php-hunspell-0.1-design.md` is the source of truth. Every task below traces back to a numbered section in that spec.

---

## File map (state after 0.1 is done)

```
.github/workflows/ci.yml            CI matrix (5 jobs)
composer.json                       PIE metadata, "type": "php-ext"
config.m4                           pkg-config detection, C++17, multi-file PHP_NEW_EXTENSION
php_hunspell.h                      shared header
hunspell.cc                         module entry (MINIT/MSHUTDOWN/MINFO)
dictionary.cc                       Hunspell\Dictionary methods + shared free helper
analysis.cc                         Hunspell\Analysis + parse_analysis_line()
exceptions.cc                       Hunspell\Exception\* registration
tests/data/test.aff                 minimal English-ish .aff
tests/data/test.dic                 ~5 words with morph fields
tests/001-load.phpt                 __construct success + load failure
tests/010-spell.phpt                spell true/false
tests/020-suggest.phpt              suggest returns [] (not false) on correct words
tests/030-analyze-raw.phpt          raw morph strings
tests/040-analyze.phpt              parsed Analysis objects
tests/041-analysis-userland.phpt    new Analysis($raw) ≡ Dictionary::analyze
tests/050-stem.phpt                 stem
tests/060-generate-string.phpt      generate with string model
tests/061-generate-array.phpt       generate with array model
tests/062-generate-typeerror.phpt   non-string array element → TypeError
tests/070-add-affix.phpt            addWithAffix + spell
tests/080-add-dic.phpt              addDictionary success + failure
tests/090-meta.phpt                 getEncoding / getWordChars / getVersion
tests/100-clone-forbidden.phpt      clone → Error
tests/101-key-rejected.phpt         non-null $key → InvalidArgumentException
README.md                           build/install/example + CI badge
CLAUDE.md                           rewritten to match the 0.1 architecture
docs/superpowers/specs/...          (already committed)
docs/superpowers/plans/...          (this file)
```

The legacy `hunspell.c`, `php_hunspell.h`, `config.m4`, and `tests/001.phpt` are deleted as part of Task 1.

---

## Task 1: Wipe the legacy code and stand up an empty C++ extension

**Goal:** End the task with a buildable, loadable empty extension. `php -m | grep -i hunspell` prints `hunspell`. No classes registered yet.

**Files:**
- Delete: `hunspell.c`, `php_hunspell.h`, `config.m4`, `tests/001.phpt`
- Create: `config.m4`, `php_hunspell.h`, `hunspell.cc`

- [ ] **Step 1: Remove the legacy files**

```bash
git rm hunspell.c php_hunspell.h config.m4 tests/001.phpt
```

- [ ] **Step 2: Write the new `config.m4`**

Create `config.m4`:

```m4
dnl config.m4 for the hunspell PHP extension

PHP_ARG_ENABLE([hunspell],
  [whether to enable hunspell support],
  [AS_HELP_STRING([--enable-hunspell], [Enable hunspell support])])

if test "$PHP_HUNSPELL" != "no"; then
  PHP_REQUIRE_CXX()
  PKG_CHECK_MODULES([HUNSPELL], [hunspell >= 1.6.0])
  PHP_EVAL_INCLINE($HUNSPELL_CFLAGS)
  PHP_EVAL_LIBLINE($HUNSPELL_LIBS, HUNSPELL_SHARED_LIBADD)
  PHP_SUBST(HUNSPELL_SHARED_LIBADD)
  PHP_NEW_EXTENSION([hunspell],
    [hunspell.cc dictionary.cc analysis.cc exceptions.cc],
    [$ext_shared],,
    [-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -std=c++17])
fi
```

- [ ] **Step 3: Write the shared header `php_hunspell.h`**

```cpp
#ifndef PHP_HUNSPELL_H
#define PHP_HUNSPELL_H

extern "C" {
#include "php.h"
}

#define PHP_HUNSPELL_VERSION "0.1.0"

extern zend_module_entry hunspell_module_entry;
#define phpext_hunspell_ptr &hunspell_module_entry

#endif /* PHP_HUNSPELL_H */
```

- [ ] **Step 4: Write a minimal `hunspell.cc` (module entry only, no classes yet)**

```cpp
#include "php_hunspell.h"

extern "C" {
#include "ext/standard/info.h"
#include <hunspell/hunspell.h>
}

static PHP_MINIT_FUNCTION(hunspell) {
    return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(hunspell) {
    return SUCCESS;
}

static PHP_MINFO_FUNCTION(hunspell) {
    php_info_print_table_start();
    php_info_print_table_header(2, "hunspell support", "enabled");
    php_info_print_table_row(2, "module version", PHP_HUNSPELL_VERSION);
    php_info_print_table_row(2, "libhunspell version", "(pkg-config)");
    php_info_print_table_end();
}

zend_module_entry hunspell_module_entry = {
    STANDARD_MODULE_HEADER,
    "hunspell",
    nullptr,                /* functions */
    PHP_MINIT(hunspell),
    PHP_MSHUTDOWN(hunspell),
    nullptr, nullptr,
    PHP_MINFO(hunspell),
    PHP_HUNSPELL_VERSION,
    STANDARD_MODULE_PROPERTIES
};

extern "C" {
    ZEND_GET_MODULE(hunspell)
}
```

- [ ] **Step 5: Create the empty per-feature C++ files referenced by `config.m4`**

```bash
: > dictionary.cc
: > analysis.cc
: > exceptions.cc
```

These will be filled in by later tasks. Empty files compile cleanly.

- [ ] **Step 6: Build and verify it loads**

```bash
phpize
./configure --enable-hunspell
make -j4
php -d extension="$(pwd)/modules/hunspell.so" -m | grep -i hunspell
```

Expected: prints `hunspell`.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "$(cat <<'EOF'
rewrite: replace legacy PHP 5 code with empty C++17 skeleton

Drops hunspell.c / php_hunspell.h / config.m4 / tests/001.phpt.
New config.m4 uses pkg-config (libhunspell >= 1.6.0), C++17, and a
multi-file PHP_NEW_EXTENSION. hunspell.cc is the module entry only;
dictionary.cc / analysis.cc / exceptions.cc are empty placeholders.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 2: Add the test dictionary

**Goal:** A tiny `.aff`/`.dic` pair that every subsequent phpt can load.

**Files:**
- Create: `tests/data/test.aff`, `tests/data/test.dic`

- [ ] **Step 1: Create `tests/data/test.aff`**

```
SET UTF-8
TRY abcdefghijklmnopqrstuvwxyz
WORDCHARS abcdefghijklmnopqrstuvwxyz
SFX A Y 1
SFX A 0 s .
```

This declares UTF-8 encoding, a simple suggest alphabet, and one affix group "A" that appends "s" unconditionally (for plural-style suggestions).

- [ ] **Step 2: Create `tests/data/test.dic`**

```
5
hello po:greet
world/A po:noun
foo po:test ts:NOM
szótár po:noun ts:NOM al:szótárak hy:3
example po:noun
```

The header `5` is the word count. `world/A` participates in the SFX A affix group, so `worlds` will be accepted via the affix. Each line after the word carries morphological annotations Hunspell will echo back from `analyze()`.

- [ ] **Step 3: Sanity-check the dictionary via libhunspell directly (no PHP yet)**

```bash
hunspell -d ./tests/data/test -a <<< "hello world worlds foo bar"
```

Expected: each known word echoed as `*` (correct), `bar` echoed with suggestions. If `hunspell` CLI is not installed, skip this step.

- [ ] **Step 4: Commit**

```bash
git add tests/data
git commit -m "$(cat <<'EOF'
test: add minimal .aff/.dic fixture with morph fields

Five words covering plain entries, an affix-group entry, and entries
with morphological annotations to exercise analyze() and the parser.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 3: Register the exception interface and class

**Goal:** `Hunspell\Exception\Exception` interface and `Hunspell\Exception\DictionaryLoadException` class are registered and reflectable.

**Files:**
- Modify: `php_hunspell.h`, `hunspell.cc`
- Create: `exceptions.cc`
- Test: `tests/002-exceptions-registered.phpt` (sanity, can be deleted later if redundant)

- [ ] **Step 1: Write the failing test**

`tests/002-exceptions-registered.phpt`:

```
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
```

- [ ] **Step 2: Run the test, expect FAIL**

```bash
make test TESTS=tests/002-exceptions-registered.phpt
```

Expected: FAIL (`Class "Hunspell\Exception\DictionaryLoadException" not found`).

- [ ] **Step 3: Add extern declarations to `php_hunspell.h`**

Insert before the closing `#endif`:

```cpp
extern zend_class_entry *hunspell_exception_ce;             /* interface */
extern zend_class_entry *hunspell_dictionary_load_exception_ce;

void hunspell_register_exception_classes(void);
```

- [ ] **Step 4: Write `exceptions.cc`**

```cpp
#include "php_hunspell.h"

extern "C" {
#include "zend_exceptions.h"
#include "zend_interfaces.h"
}

zend_class_entry *hunspell_exception_ce = nullptr;
zend_class_entry *hunspell_dictionary_load_exception_ce = nullptr;

void hunspell_register_exception_classes(void) {
    zend_class_entry ce_iface;
    INIT_NS_CLASS_ENTRY(ce_iface, "Hunspell\\Exception", "Exception", nullptr);
    hunspell_exception_ce = zend_register_internal_interface(&ce_iface);

    zend_class_entry ce_dle;
    INIT_NS_CLASS_ENTRY(ce_dle, "Hunspell\\Exception", "DictionaryLoadException", nullptr);
    hunspell_dictionary_load_exception_ce =
        zend_register_internal_class_ex(&ce_dle, zend_ce_exception);
    /* extend RuntimeException specifically */
    hunspell_dictionary_load_exception_ce =
        zend_register_internal_class_ex(&ce_dle, spl_ce_RuntimeException);
    zend_class_implements(hunspell_dictionary_load_exception_ce, 1, hunspell_exception_ce);
    hunspell_dictionary_load_exception_ce->ce_flags |= ZEND_ACC_FINAL;
}
```

Note: `spl_ce_RuntimeException` is exposed via `ext/spl/spl_exceptions.h`. Add `#include "ext/spl/spl_exceptions.h"` inside the `extern "C"` block at the top.

- [ ] **Step 5: Call the registration from `hunspell.cc` MINIT**

In `hunspell.cc`, replace the MINIT body:

```cpp
static PHP_MINIT_FUNCTION(hunspell) {
    hunspell_register_exception_classes();
    return SUCCESS;
}
```

- [ ] **Step 6: Rebuild and run the test**

```bash
make -j4
make test TESTS=tests/002-exceptions-registered.phpt
```

Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add php_hunspell.h hunspell.cc exceptions.cc tests/002-exceptions-registered.phpt
git commit -m "$(cat <<'EOF'
feat(exceptions): register Hunspell\\Exception\\* hierarchy

Adds the marker interface Hunspell\\Exception\\Exception and the
concrete DictionaryLoadException class (extends RuntimeException,
implements the marker). Registered in MINIT.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 4: Register Hunspell\Dictionary class skeleton

**Goal:** The class exists, its constructor accepts arguments, but no PHP methods are wired yet. Custom object handlers are in place so `Hunhandle*` can be stored.

**Files:**
- Modify: `php_hunspell.h`, `hunspell.cc`
- Create: `dictionary.cc`
- Test: `tests/003-dictionary-class.phpt`

- [ ] **Step 1: Write the failing test**

`tests/003-dictionary-class.phpt`:

```
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
```

- [ ] **Step 2: Run, expect FAIL**

```bash
make test TESTS=tests/003-dictionary-class.phpt
```

Expected: FAIL (`Class "Hunspell\Dictionary" not found`).

- [ ] **Step 3: Extend `php_hunspell.h`**

Add inside the header, before `#endif`:

```cpp
extern "C" {
#include <hunspell/hunspell.h>
}

typedef struct {
    Hunhandle *handle;
    zend_object std;        /* must be last */
} php_hunspell_object;

extern zend_class_entry *hunspell_dictionary_ce;

static inline php_hunspell_object *php_hunspell_from_obj(zend_object *obj) {
    return reinterpret_cast<php_hunspell_object *>(
        reinterpret_cast<char *>(obj) - XtOffsetOf(php_hunspell_object, std));
}

void hunspell_register_dictionary_class(void);
```

- [ ] **Step 4: Write `dictionary.cc` with object handlers + class registration only**

```cpp
#include "php_hunspell.h"

extern "C" {
#include "zend_exceptions.h"
}

zend_class_entry *hunspell_dictionary_ce = nullptr;
static zend_object_handlers hunspell_dictionary_handlers;

static zend_object *hunspell_dictionary_create(zend_class_entry *ce) {
    php_hunspell_object *obj = static_cast<php_hunspell_object *>(
        zend_object_alloc(sizeof(*obj), ce));
    zend_object_std_init(&obj->std, ce);
    object_properties_init(&obj->std, ce);
    obj->std.handlers = &hunspell_dictionary_handlers;
    obj->handle = nullptr;
    return &obj->std;
}

static void hunspell_dictionary_free(zend_object *zobj) {
    php_hunspell_object *obj = php_hunspell_from_obj(zobj);
    if (obj->handle != nullptr) {
        Hunspell_destroy(obj->handle);
        obj->handle = nullptr;
    }
    zend_object_std_dtor(&obj->std);
}

static const zend_function_entry hunspell_dictionary_methods[] = {
    PHP_FE_END
};

void hunspell_register_dictionary_class(void) {
    zend_class_entry ce;
    INIT_NS_CLASS_ENTRY(ce, "Hunspell", "Dictionary", hunspell_dictionary_methods);
    hunspell_dictionary_ce = zend_register_internal_class(&ce);
    hunspell_dictionary_ce->create_object = hunspell_dictionary_create;
    hunspell_dictionary_ce->ce_flags |= ZEND_ACC_FINAL;

    memcpy(&hunspell_dictionary_handlers, &std_object_handlers, sizeof(zend_object_handlers));
    hunspell_dictionary_handlers.offset = XtOffsetOf(php_hunspell_object, std);
    hunspell_dictionary_handlers.free_obj = hunspell_dictionary_free;
    hunspell_dictionary_handlers.clone_obj = nullptr;   /* forbid cloning at engine level */
}
```

- [ ] **Step 5: Call the registration from `hunspell.cc` MINIT**

```cpp
static PHP_MINIT_FUNCTION(hunspell) {
    hunspell_register_exception_classes();
    hunspell_register_dictionary_class();
    return SUCCESS;
}
```

- [ ] **Step 6: Rebuild and run the test**

```bash
make -j4
make test TESTS=tests/003-dictionary-class.phpt
```

Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add php_hunspell.h hunspell.cc dictionary.cc tests/003-dictionary-class.phpt
git commit -m "$(cat <<'EOF'
feat(dictionary): register Hunspell\\Dictionary class skeleton

Custom object handlers store a Hunhandle* (initialised to nullptr).
free_obj calls Hunspell_destroy when non-null. clone_obj is nullptr,
so the engine forbids cloning. No PHP methods yet.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 5: Implement `Dictionary::__construct` and 001-load.phpt

**Goal:** Constructor loads the dictionary or throws `DictionaryLoadException`; non-null `$key` argument throws `InvalidArgumentException`.

**Files:**
- Modify: `dictionary.cc`
- Create: `tests/001-load.phpt`

- [ ] **Step 1: Write the failing test**

`tests/001-load.phpt`:

```
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
```

- [ ] **Step 2: Run, expect FAIL**

```bash
make test TESTS=tests/001-load.phpt
```

Expected: FAIL (constructor does not exist).

- [ ] **Step 3: Implement `Dictionary::__construct`**

Add to `dictionary.cc` above the methods table:

```cpp
static zend_object_handlers *unused = nullptr;  /* placeholder, remove if not used */

ZEND_BEGIN_ARG_INFO_EX(arginfo_dictionary_construct, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, affPath, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, dicPath, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, key, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, __construct) {
    zend_string *aff_path, *dic_path;
    zend_string *key = nullptr;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(aff_path)
        Z_PARAM_STR(dic_path)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(key)
    ZEND_PARSE_PARAMETERS_END();

    if (key != nullptr) {
        zend_throw_exception(zend_ce_argument_count_error,   /* placeholder, replaced below */
            "Encrypted dictionary keys are not supported in this release", 0);
        /* use \InvalidArgumentException — see below */
    }

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    obj->handle = Hunspell_create(ZSTR_VAL(aff_path), ZSTR_VAL(dic_path));
    if (obj->handle == nullptr) {
        zend_throw_exception_ex(hunspell_dictionary_load_exception_ce, 0,
            "Failed to load Hunspell dictionary (aff='%s', dic='%s')",
            ZSTR_VAL(aff_path), ZSTR_VAL(dic_path));
        return;
    }
}
```

Fix the `$key` rejection to throw `\InvalidArgumentException`. Add at top of file inside `extern "C"`:

```cpp
extern "C" {
#include "ext/spl/spl_exceptions.h"
}
```

Then change the rejection block to:

```cpp
    if (key != nullptr) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Encrypted dictionary keys are not supported in this release", 0);
        return;
    }
```

- [ ] **Step 4: Wire the method into the methods table**

Replace the existing `hunspell_dictionary_methods[]` block with:

```cpp
static const zend_function_entry hunspell_dictionary_methods[] = {
    PHP_ME(Hunspell_Dictionary, __construct, arginfo_dictionary_construct,
           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_FE_END
};
```

PHP_ME uses the class name with an underscore separator. The actual PHP-level name is taken from the namespace registered via `INIT_NS_CLASS_ENTRY`.

- [ ] **Step 5: Rebuild and run**

```bash
make -j4
make test TESTS=tests/001-load.phpt
```

Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add dictionary.cc tests/001-load.phpt
git commit -m "$(cat <<'EOF'
feat(dictionary): implement __construct with $key rejection

Loads the dictionary via Hunspell_create; throws
DictionaryLoadException on NULL and InvalidArgumentException when
the (P2) $key argument is non-null.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 6: Implement `Dictionary::spell` and 010-spell.phpt

**Goal:** `spell(string): bool` returns true for known words, false for unknown.

**Files:**
- Modify: `dictionary.cc`
- Create: `tests/010-spell.phpt`

- [ ] **Step 1: Write the failing test**

`tests/010-spell.phpt`:

```
--TEST--
Dictionary::spell returns bool for known and unknown words
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");
var_dump($dict->spell('hello'));    /* true */
var_dump($dict->spell('worlds'));   /* true via SFX A */
var_dump($dict->spell('xyzzy'));    /* false */
?>
--EXPECT--
bool(true)
bool(true)
bool(false)
```

- [ ] **Step 2: Run, expect FAIL**

```bash
make test TESTS=tests/010-spell.phpt
```

Expected: FAIL (`spell()` not defined).

- [ ] **Step 3: Implement `spell`**

Add to `dictionary.cc` above the methods table:

```cpp
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_spell, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, spell) {
    zend_string *word;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(word)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    int ok = Hunspell_spell(obj->handle, ZSTR_VAL(word));
    RETURN_BOOL(ok != 0);
}
```

- [ ] **Step 4: Add to the methods table**

```cpp
static const zend_function_entry hunspell_dictionary_methods[] = {
    PHP_ME(Hunspell_Dictionary, __construct, arginfo_dictionary_construct,
           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(Hunspell_Dictionary, spell, arginfo_dictionary_spell, ZEND_ACC_PUBLIC)
    PHP_FE_END
};
```

- [ ] **Step 5: Rebuild and run**

```bash
make -j4
make test TESTS=tests/010-spell.phpt
```

Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add dictionary.cc tests/010-spell.phpt
git commit -m "$(cat <<'EOF'
feat(dictionary): implement spell()

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 7: Add the shared `hunspell_strlist_to_array_and_free` helper, implement `suggest`, 020-suggest.phpt

**Goal:** `suggest(string): array` returns suggestions for misspelled words, empty array for correct words (no `false`). The helper that turns Hunspell `char**` lists into PHP arrays and frees them via `Hunspell_free_list` is in place for the next four list-returning methods.

**Files:**
- Modify: `dictionary.cc`
- Create: `tests/020-suggest.phpt`

- [ ] **Step 1: Write the failing test**

`tests/020-suggest.phpt`:

```
--TEST--
Dictionary::suggest returns suggestions, [] for correct words
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

$s = $dict->suggest('helo');
var_dump(is_array($s));
var_dump(in_array('hello', $s, true));

$s2 = $dict->suggest('hello');
var_dump($s2);
?>
--EXPECT--
bool(true)
bool(true)
array(0) {
}
```

- [ ] **Step 2: Run, expect FAIL**

```bash
make test TESTS=tests/020-suggest.phpt
```

Expected: FAIL (`suggest()` not defined).

- [ ] **Step 3: Add the shared helper to `dictionary.cc`**

Insert near the top of the file, after the includes but before the method implementations:

```cpp
static void hunspell_strlist_to_array_and_free(
        Hunhandle *handle, char **slst, int n, zval *return_value) {
    array_init_size(return_value, n);
    for (int i = 0; i < n; i++) {
        add_next_index_string(return_value, slst[i]);
    }
    Hunspell_free_list(handle, &slst, n);
}
```

- [ ] **Step 4: Implement `suggest`**

```cpp
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_suggest, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, suggest) {
    zend_string *word;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(word)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    char **slst = nullptr;
    int n = Hunspell_suggest(obj->handle, &slst, ZSTR_VAL(word));
    hunspell_strlist_to_array_and_free(obj->handle, slst, n, return_value);
}
```

- [ ] **Step 5: Wire into the methods table**

```cpp
    PHP_ME(Hunspell_Dictionary, suggest, arginfo_dictionary_suggest, ZEND_ACC_PUBLIC)
```

- [ ] **Step 6: Rebuild and run**

```bash
make -j4
make test TESTS=tests/020-suggest.phpt
```

Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add dictionary.cc tests/020-suggest.phpt
git commit -m "$(cat <<'EOF'
feat(dictionary): implement suggest() with shared list-free helper

Adds hunspell_strlist_to_array_and_free, used by suggest and all
upcoming list-returning methods. Guarantees Hunspell_free_list runs
exactly once per call — eliminates the leak in the legacy code.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 8: Implement `Hunspell\Analysis` class with parser

**Goal:** `Analysis` is registered as a readonly class with `$raw`/`$fields` properties and a constructor that parses the raw analysis string.

**Files:**
- Modify: `php_hunspell.h`, `hunspell.cc`
- Create: `analysis.cc`
- Test: `tests/041-analysis-userland.phpt` (a portion — the second half is added in Task 11)

- [ ] **Step 1: Write the failing test (constructor + getters)**

`tests/041-analysis-userland.phpt`:

```
--TEST--
new Analysis($raw) parses key:value tokens into $fields
--EXTENSIONS--
hunspell
--FILE--
<?php
$a = new Hunspell\Analysis('st:szótár po:noun ts:NOM al:szótárak hy:3');

var_dump($a->getRaw());
var_dump($a->getFields());
var_dump($a->get('po'));
var_dump($a->has('hy'));
var_dump($a->has('zz'));

/* repeated key collects into list */
$b = new Hunspell\Analysis('st:foo st:bar po:noun');
var_dump($b->get('st'));

/* malformed tokens are silently skipped */
$c = new Hunspell\Analysis('justaword po:noun  ');
var_dump($c->getFields());
?>
--EXPECT--
string(45) "st:szótár po:noun ts:NOM al:szótárak hy:3"
array(5) {
  ["st"]=>
  array(1) {
    [0]=>
    string(7) "szótár"
  }
  ["po"]=>
  array(1) {
    [0]=>
    string(4) "noun"
  }
  ["ts"]=>
  array(1) {
    [0]=>
    string(3) "NOM"
  }
  ["al"]=>
  array(1) {
    [0]=>
    string(9) "szótárak"
  }
  ["hy"]=>
  array(1) {
    [0]=>
    string(1) "3"
  }
}
array(1) {
  [0]=>
  string(4) "noun"
}
bool(true)
bool(false)
array(1) {
  [0]=>
  array(2) {
    [0]=>
    string(3) "foo"
    [1]=>
    string(3) "bar"
  }
}
array(1) {
  ["po"]=>
  array(1) {
    [0]=>
    string(4) "noun"
  }
}
```

Note: the multibyte byte counts (`string(45)`, `string(7) "szótár"`, `string(9) "szótárak"`) are correct because each Hungarian accented letter is 2 bytes in UTF-8.

The second-to-last assertion (the repeated `st:`) is wrong as written — `get('st')` returns `list<string>`, so the expected output should be a flat list, not a wrapped list. Fix:

Replace the second-to-last `--EXPECT--` block (the `array(1) { [0]=> array(2) { ... } }`) with:

```
array(2) {
  [0]=>
  string(3) "foo"
  [1]=>
  string(3) "bar"
}
```

- [ ] **Step 2: Run, expect FAIL**

```bash
make test TESTS=tests/041-analysis-userland.phpt
```

Expected: FAIL (`Class "Hunspell\Analysis" not found`).

- [ ] **Step 3: Extend `php_hunspell.h`**

Add before `#endif`:

```cpp
extern zend_class_entry *hunspell_analysis_ce;

void hunspell_register_analysis_class(void);

/* Parser: tokenize raw on whitespace, split each token on first ':'.
 * Repeated keys append. Tokens without ':' are skipped.
 * out_fields is initialized as an associative array by the parser. */
void hunspell_parse_analysis_line(const char *raw, size_t len, zval *out_fields);
```

- [ ] **Step 4: Write `analysis.cc`**

```cpp
#include "php_hunspell.h"

#include <cctype>
#include <cstring>

extern "C" {
#include "Zend/zend_object_handlers.h"
#include "Zend/zend_inheritance.h"
}

zend_class_entry *hunspell_analysis_ce = nullptr;

void hunspell_parse_analysis_line(const char *raw, size_t len, zval *out_fields) {
    array_init(out_fields);
    const char *p = raw;
    const char *end = raw + len;

    while (p < end) {
        while (p < end && std::isspace(static_cast<unsigned char>(*p))) p++;
        if (p >= end) break;

        const char *tok_start = p;
        while (p < end && !std::isspace(static_cast<unsigned char>(*p))) p++;
        const char *tok_end = p;

        const char *colon = static_cast<const char *>(
            std::memchr(tok_start, ':', tok_end - tok_start));
        if (colon == nullptr || colon == tok_start) continue;  /* skip malformed */

        size_t key_len = colon - tok_start;
        size_t val_len = tok_end - (colon + 1);

        zval *bucket = zend_hash_str_find(Z_ARRVAL_P(out_fields), tok_start, key_len);
        if (bucket == nullptr) {
            zval new_list;
            array_init(&new_list);
            bucket = zend_hash_str_add_new(Z_ARRVAL_P(out_fields), tok_start, key_len, &new_list);
        }
        add_next_index_stringl(bucket, colon + 1, val_len);
    }
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_analysis_construct, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, raw, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Analysis, __construct) {
    zend_string *raw;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(raw)
    ZEND_PARSE_PARAMETERS_END();

    zend_update_property_str(hunspell_analysis_ce, Z_OBJ_P(ZEND_THIS),
        "raw", sizeof("raw") - 1, raw);
    zend_string_addref(raw);   /* zend_update_property_str copies, undo if it doesn't */

    zval fields;
    hunspell_parse_analysis_line(ZSTR_VAL(raw), ZSTR_LEN(raw), &fields);
    zend_update_property(hunspell_analysis_ce, Z_OBJ_P(ZEND_THIS),
        "fields", sizeof("fields") - 1, &fields);
    zval_ptr_dtor(&fields);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_analysis_getRaw, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Analysis, getRaw) {
    ZEND_PARSE_PARAMETERS_NONE();
    zval *raw = zend_read_property(hunspell_analysis_ce, Z_OBJ_P(ZEND_THIS),
        "raw", sizeof("raw") - 1, /*silent*/ 0, /*rv*/ nullptr);
    RETURN_COPY(raw);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_analysis_getFields, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Analysis, getFields) {
    ZEND_PARSE_PARAMETERS_NONE();
    zval *fields = zend_read_property(hunspell_analysis_ce, Z_OBJ_P(ZEND_THIS),
        "fields", sizeof("fields") - 1, 0, nullptr);
    RETURN_COPY(fields);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_analysis_get, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Analysis, get) {
    zend_string *key;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(key)
    ZEND_PARSE_PARAMETERS_END();

    zval *fields = zend_read_property(hunspell_analysis_ce, Z_OBJ_P(ZEND_THIS),
        "fields", sizeof("fields") - 1, 0, nullptr);
    if (Z_TYPE_P(fields) != IS_ARRAY) { RETURN_EMPTY_ARRAY(); }

    zval *bucket = zend_hash_find(Z_ARRVAL_P(fields), key);
    if (bucket == nullptr) { RETURN_EMPTY_ARRAY(); }
    RETURN_COPY(bucket);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_analysis_has, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Analysis, has) {
    zend_string *key;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(key)
    ZEND_PARSE_PARAMETERS_END();

    zval *fields = zend_read_property(hunspell_analysis_ce, Z_OBJ_P(ZEND_THIS),
        "fields", sizeof("fields") - 1, 0, nullptr);
    if (Z_TYPE_P(fields) != IS_ARRAY) { RETURN_FALSE; }
    RETURN_BOOL(zend_hash_find(Z_ARRVAL_P(fields), key) != nullptr);
}

static const zend_function_entry hunspell_analysis_methods[] = {
    PHP_ME(Hunspell_Analysis, __construct, arginfo_analysis_construct,
           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(Hunspell_Analysis, getRaw,    arginfo_analysis_getRaw,    ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Analysis, getFields, arginfo_analysis_getFields, ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Analysis, get,       arginfo_analysis_get,       ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Analysis, has,       arginfo_analysis_has,       ZEND_ACC_PUBLIC)
    PHP_FE_END
};

void hunspell_register_analysis_class(void) {
    zend_class_entry ce;
    INIT_NS_CLASS_ENTRY(ce, "Hunspell", "Analysis", hunspell_analysis_methods);
    hunspell_analysis_ce = zend_register_internal_class(&ce);
    hunspell_analysis_ce->ce_flags |= ZEND_ACC_FINAL | ZEND_ACC_READONLY_CLASS;

    /* declare readonly properties */
    zval default_str; ZVAL_EMPTY_STRING(&default_str);
    zend_declare_typed_property(hunspell_analysis_ce,
        zend_string_init_interned("raw", sizeof("raw") - 1, 1),
        &default_str, ZEND_ACC_PUBLIC | ZEND_ACC_READONLY, nullptr,
        (zend_type) ZEND_TYPE_INIT_CODE(IS_STRING, 0, 0));

    zval default_arr; array_init(&default_arr);
    zend_declare_typed_property(hunspell_analysis_ce,
        zend_string_init_interned("fields", sizeof("fields") - 1, 1),
        &default_arr, ZEND_ACC_PUBLIC | ZEND_ACC_READONLY, nullptr,
        (zend_type) ZEND_TYPE_INIT_CODE(IS_ARRAY, 0, 0));
    zval_ptr_dtor(&default_arr);
}
```

Note: `ZEND_ACC_READONLY_CLASS` is the flag for `readonly class` (PHP 8.2+). The typed property declarations include `ZEND_ACC_READONLY` so each property is individually readonly even on engines that don't enforce the class-level flag at property level.

- [ ] **Step 5: Call the registration from `hunspell.cc` MINIT**

```cpp
static PHP_MINIT_FUNCTION(hunspell) {
    hunspell_register_exception_classes();
    hunspell_register_analysis_class();
    hunspell_register_dictionary_class();
    return SUCCESS;
}
```

- [ ] **Step 6: Rebuild and run**

```bash
make -j4
make test TESTS=tests/041-analysis-userland.phpt
```

Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add php_hunspell.h hunspell.cc analysis.cc tests/041-analysis-userland.phpt
git commit -m "$(cat <<'EOF'
feat(analysis): add Hunspell\\Analysis readonly class with parser

readonly class with $raw and $fields properties, parser as a
standalone C++ function (hunspell_parse_analysis_line) shared with
Dictionary::analyze. Userland-constructed Analysis is fully usable
independently of a Dictionary instance.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 9: Implement `Dictionary::analyzeRaw` and 030-analyze-raw.phpt

**Goal:** `analyzeRaw(string): list<string>` returns the raw morph strings as produced by `Hunspell_analyze`.

**Files:**
- Modify: `dictionary.cc`
- Create: `tests/030-analyze-raw.phpt`

- [ ] **Step 1: Write the failing test**

`tests/030-analyze-raw.phpt`:

```
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
```

- [ ] **Step 2: Run, expect FAIL**

```bash
make test TESTS=tests/030-analyze-raw.phpt
```

Expected: FAIL.

- [ ] **Step 3: Implement `analyzeRaw`**

In `dictionary.cc`:

```cpp
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_analyzeRaw, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, analyzeRaw) {
    zend_string *word;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(word)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    char **slst = nullptr;
    int n = Hunspell_analyze(obj->handle, &slst, ZSTR_VAL(word));
    hunspell_strlist_to_array_and_free(obj->handle, slst, n, return_value);
}
```

Methods table:

```cpp
    PHP_ME(Hunspell_Dictionary, analyzeRaw, arginfo_dictionary_analyzeRaw, ZEND_ACC_PUBLIC)
```

- [ ] **Step 4: Rebuild and run**

```bash
make -j4
make test TESTS=tests/030-analyze-raw.phpt
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add dictionary.cc tests/030-analyze-raw.phpt
git commit -m "$(cat <<'EOF'
feat(dictionary): implement analyzeRaw()

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 10: Implement `Dictionary::analyze` and 040-analyze.phpt

**Goal:** `analyze(string): list<Analysis>` returns a list of `Hunspell\Analysis` value objects constructed by direct property writes from the parser output.

**Files:**
- Modify: `dictionary.cc`
- Create: `tests/040-analyze.phpt`

- [ ] **Step 1: Write the failing test**

`tests/040-analyze.phpt`:

```
--TEST--
Dictionary::analyze returns parsed Hunspell\Analysis objects
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

$analyses = $dict->analyze('szótár');
var_dump(is_array($analyses));
var_dump(count($analyses) >= 1);

$a = $analyses[0];
var_dump($a instanceof Hunspell\Analysis);
var_dump($a->get('st'));
var_dump($a->get('po'));
var_dump($a->has('hy'));

/* unknown word: empty */
var_dump($dict->analyze('xyzzy'));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
array(1) {
  [0]=>
  string(7) "szótár"
}
array(1) {
  [0]=>
  string(4) "noun"
}
bool(true)
array(0) {
}
```

- [ ] **Step 2: Run, expect FAIL**

```bash
make test TESTS=tests/040-analyze.phpt
```

- [ ] **Step 3: Implement `analyze`**

In `dictionary.cc`, add a `#include "Zend/zend_object_handlers.h"` if not already present via `php.h`, then:

```cpp
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_analyze, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, analyze) {
    zend_string *word;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(word)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    char **slst = nullptr;
    int n = Hunspell_analyze(obj->handle, &slst, ZSTR_VAL(word));

    array_init_size(return_value, n);
    for (int i = 0; i < n; i++) {
        zval analysis_zv;
        object_init_ex(&analysis_zv, hunspell_analysis_ce);

        zend_object *aobj = Z_OBJ(analysis_zv);
        zend_string *raw = zend_string_init(slst[i], strlen(slst[i]), 0);
        zend_update_property_str(hunspell_analysis_ce, aobj,
            "raw", sizeof("raw") - 1, raw);
        zend_string_release(raw);

        zval fields;
        hunspell_parse_analysis_line(slst[i], strlen(slst[i]), &fields);
        zend_update_property(hunspell_analysis_ce, aobj,
            "fields", sizeof("fields") - 1, &fields);
        zval_ptr_dtor(&fields);

        add_next_index_zval(return_value, &analysis_zv);
    }
    Hunspell_free_list(obj->handle, &slst, n);
}
```

Method table:

```cpp
    PHP_ME(Hunspell_Dictionary, analyze, arginfo_dictionary_analyze, ZEND_ACC_PUBLIC)
```

- [ ] **Step 4: Rebuild and run**

```bash
make -j4
make test TESTS=tests/040-analyze.phpt
```

Expected: PASS.

- [ ] **Step 5: Add a second test that verifies fast-path / slow-path equivalence**

Append to `tests/041-analysis-userland.phpt` a section that compares `$dict->analyze($w)[0]` to `new Analysis($raw_from_analyzeRaw)`:

The test file should look like (full replacement):

```
--TEST--
new Analysis($raw) matches Dictionary::analyze byte-for-byte
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

$raw_list = $dict->analyzeRaw('szótár');
$obj_list = $dict->analyze('szótár');

for ($i = 0, $n = count($raw_list); $i < $n; $i++) {
    $userland = new Hunspell\Analysis($raw_list[$i]);
    $api      = $obj_list[$i];

    if ($userland->getRaw() !== $api->getRaw()) {
        echo "MISMATCH raw\n"; exit;
    }
    if ($userland->getFields() != $api->getFields()) {
        echo "MISMATCH fields\n"; exit;
    }
}
echo "equivalent\n";
?>
--EXPECT--
equivalent
```

Move the parser-only assertions (the original Step 1 from Task 8) to a new file `tests/008-analysis-parser.phpt` to keep concerns separate.

- [ ] **Step 6: Run both tests**

```bash
make test TESTS="tests/040-analyze.phpt tests/041-analysis-userland.phpt tests/008-analysis-parser.phpt"
```

All three PASS.

- [ ] **Step 7: Commit**

```bash
git add dictionary.cc tests/040-analyze.phpt tests/041-analysis-userland.phpt tests/008-analysis-parser.phpt
git commit -m "$(cat <<'EOF'
feat(dictionary): implement analyze() with direct property writes

Constructs Analysis objects via object_init_ex + zend_update_property
to avoid per-result __construct dispatch. The userland and
fast-path produce byte-identical state (verified by 041).

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 11: Implement `Dictionary::stem` and 050-stem.phpt

**Goal:** `stem(string): list<string>` returns stems for the given word.

**Files:**
- Modify: `dictionary.cc`
- Create: `tests/050-stem.phpt`

- [ ] **Step 1: Write the failing test**

`tests/050-stem.phpt`:

```
--TEST--
Dictionary::stem returns stems for inflected words
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

$s = $dict->stem('worlds');
var_dump(in_array('world', $s, true));

var_dump($dict->stem('xyzzy'));
?>
--EXPECT--
bool(true)
array(0) {
}
```

- [ ] **Step 2: Run, expect FAIL**

```bash
make test TESTS=tests/050-stem.phpt
```

- [ ] **Step 3: Implement `stem`**

In `dictionary.cc`:

```cpp
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_stem, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, stem) {
    zend_string *word;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(word)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    char **slst = nullptr;
    int n = Hunspell_stem(obj->handle, &slst, ZSTR_VAL(word));
    hunspell_strlist_to_array_and_free(obj->handle, slst, n, return_value);
}
```

Methods table:

```cpp
    PHP_ME(Hunspell_Dictionary, stem, arginfo_dictionary_stem, ZEND_ACC_PUBLIC)
```

- [ ] **Step 4: Rebuild and run**

```bash
make -j4
make test TESTS=tests/050-stem.phpt
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add dictionary.cc tests/050-stem.phpt
git commit -m "feat(dictionary): implement stem()

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

---

## Task 12: Implement `Dictionary::generate` (both modes) plus 060/061/062 tests

**Goal:** `generate(string $word, string|array $model): array` dispatches on `$model` type — string → `Hunspell_generate`, array → `Hunspell_generate2`. Bad array element raises `TypeError`.

**Files:**
- Modify: `dictionary.cc`
- Create: `tests/060-generate-string.phpt`, `tests/061-generate-array.phpt`, `tests/062-generate-typeerror.phpt`

- [ ] **Step 1: Write all three failing tests**

`tests/060-generate-string.phpt`:

```
--TEST--
Dictionary::generate with a string model uses Hunspell_generate
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

/* generate the form of 'world' that matches the morphology of 'worlds' */
$out = $dict->generate('world', 'worlds');
var_dump(is_array($out));
/* For this dict we cannot guarantee a specific output, only that the call works. */
?>
--EXPECT--
bool(true)
```

`tests/061-generate-array.phpt`:

```
--TEST--
Dictionary::generate with an array model uses Hunspell_generate2
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

$out = $dict->generate('world', ['is:plural']);
var_dump(is_array($out));
?>
--EXPECT--
bool(true)
```

`tests/062-generate-typeerror.phpt`:

```
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
type-error: Argument #2 \(\$model\) item #1 must be of type string, int given.*
```

- [ ] **Step 2: Run all three, expect FAIL**

```bash
make test TESTS="tests/060-generate-string.phpt tests/061-generate-array.phpt tests/062-generate-typeerror.phpt"
```

- [ ] **Step 3: Implement `generate`**

In `dictionary.cc`:

```cpp
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_generate, 0, 2, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
    ZEND_ARG_TYPE_MASK(0, model, MAY_BE_STRING | MAY_BE_ARRAY, nullptr)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, generate) {
    zend_string *word;
    zval *model;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(word)
        Z_PARAM_ZVAL(model)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    char **slst = nullptr;
    int n = 0;

    if (Z_TYPE_P(model) == IS_STRING) {
        n = Hunspell_generate(obj->handle, &slst, ZSTR_VAL(word), Z_STRVAL_P(model));
    } else if (Z_TYPE_P(model) == IS_ARRAY) {
        HashTable *ht = Z_ARRVAL_P(model);
        uint32_t count = zend_hash_num_elements(ht);
        std::vector<const char *> desc(count);
        uint32_t i = 0;
        zval *item;
        ZEND_HASH_FOREACH_VAL(ht, item) {
            if (Z_TYPE_P(item) != IS_STRING) {
                zend_argument_type_error(2,
                    "($model) item #%u must be of type string, %s given",
                    i, zend_zval_value_name(item));
                return;
            }
            desc[i++] = Z_STRVAL_P(item);
        } ZEND_HASH_FOREACH_END();
        n = Hunspell_generate2(obj->handle, &slst, ZSTR_VAL(word),
            const_cast<char **>(desc.data()), static_cast<int>(count));
    } else {
        zend_argument_type_error(2, "must be of type string|array, %s given",
            zend_zval_value_name(model));
        return;
    }

    hunspell_strlist_to_array_and_free(obj->handle, slst, n, return_value);
}
```

Add `#include <vector>` at the top of `dictionary.cc` if not already.

Methods table:

```cpp
    PHP_ME(Hunspell_Dictionary, generate, arginfo_dictionary_generate, ZEND_ACC_PUBLIC)
```

- [ ] **Step 4: Rebuild and run**

```bash
make -j4
make test TESTS="tests/060-generate-string.phpt tests/061-generate-array.phpt tests/062-generate-typeerror.phpt"
```

Expected: All three PASS.

- [ ] **Step 5: Commit**

```bash
git add dictionary.cc tests/060-generate-string.phpt tests/061-generate-array.phpt tests/062-generate-typeerror.phpt
git commit -m "$(cat <<'EOF'
feat(dictionary): implement generate() with polymorphic $model

string model dispatches to Hunspell_generate; array model to
Hunspell_generate2. Non-string array elements raise a clear
TypeError naming the offending index.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 13: Implement `Dictionary::add`, `addWithAffix`, `remove` and 070-add-affix.phpt

**Goal:** Runtime dictionary mutation works. After `addWithAffix('foobar', 'world')`, `spell('foobar')` is true and `spell('foobars')` is true via the inherited affix.

**Files:**
- Modify: `dictionary.cc`
- Create: `tests/070-add-affix.phpt`

- [ ] **Step 1: Write the failing test**

`tests/070-add-affix.phpt`:

```
--TEST--
Dictionary::add, addWithAffix, remove update the in-memory handle
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

var_dump($dict->spell('foobar'));      /* false */

$dict->add('foobar');
var_dump($dict->spell('foobar'));      /* true */

$dict->remove('foobar');
var_dump($dict->spell('foobar'));      /* false */

/* addWithAffix: inherit "world"'s affix group A → "newword" + "newwords" both spell */
$dict->addWithAffix('newword', 'world');
var_dump($dict->spell('newword'));     /* true */
var_dump($dict->spell('newwords'));    /* true via SFX A */
?>
--EXPECT--
bool(false)
bool(true)
bool(false)
bool(true)
bool(true)
```

- [ ] **Step 2: Run, expect FAIL**

```bash
make test TESTS=tests/070-add-affix.phpt
```

- [ ] **Step 3: Implement `add`, `addWithAffix`, `remove`**

In `dictionary.cc`:

```cpp
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_add, 0, 1, IS_VOID, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, add) {
    zend_string *word;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(word)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    (void) Hunspell_add(obj->handle, ZSTR_VAL(word));
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_addWithAffix, 0, 2, IS_VOID, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, example, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, addWithAffix) {
    zend_string *word, *example;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(word)
        Z_PARAM_STR(example)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    (void) Hunspell_add_with_affix(obj->handle, ZSTR_VAL(word), ZSTR_VAL(example));
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_remove, 0, 1, IS_VOID, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, remove) {
    zend_string *word;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(word)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    (void) Hunspell_remove(obj->handle, ZSTR_VAL(word));
}
```

Methods table additions:

```cpp
    PHP_ME(Hunspell_Dictionary, add,          arginfo_dictionary_add,          ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Dictionary, addWithAffix, arginfo_dictionary_addWithAffix, ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Dictionary, remove,       arginfo_dictionary_remove,       ZEND_ACC_PUBLIC)
```

- [ ] **Step 4: Rebuild and run**

```bash
make -j4
make test TESTS=tests/070-add-affix.phpt
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add dictionary.cc tests/070-add-affix.phpt
git commit -m "$(cat <<'EOF'
feat(dictionary): implement add/addWithAffix/remove

Runtime mutation of the in-memory Hunspell handle. addWithAffix
takes an example word so inflections of the new entry are accepted
by the affix engine without authoring an .aff entry.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 14: Implement `Dictionary::addDictionary` and 080-add-dic.phpt

**Goal:** `addDictionary(string): void` stacks an additional `.dic` onto the handle; throws `DictionaryLoadException` on failure.

**Files:**
- Modify: `dictionary.cc`
- Create: `tests/080-add-dic.phpt`, `tests/data/extra.dic`

- [ ] **Step 1: Create the extra dictionary**

`tests/data/extra.dic`:

```
2
extraword po:custom
another po:custom
```

- [ ] **Step 2: Write the failing test**

`tests/080-add-dic.phpt`:

```
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
```

- [ ] **Step 3: Run, expect FAIL**

```bash
make test TESTS=tests/080-add-dic.phpt
```

- [ ] **Step 4: Implement `addDictionary`**

In `dictionary.cc`:

```cpp
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_addDictionary, 0, 1, IS_VOID, 0)
    ZEND_ARG_TYPE_INFO(0, dicPath, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, addDictionary) {
    zend_string *dic_path;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(dic_path)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    int rc = Hunspell_add_dic(obj->handle, ZSTR_VAL(dic_path));
    if (rc != 0) {
        zend_throw_exception_ex(hunspell_dictionary_load_exception_ce, 0,
            "Failed to add dictionary '%s' to handle", ZSTR_VAL(dic_path));
        return;
    }
}
```

Methods table:

```cpp
    PHP_ME(Hunspell_Dictionary, addDictionary, arginfo_dictionary_addDictionary, ZEND_ACC_PUBLIC)
```

- [ ] **Step 5: Rebuild and run**

```bash
make -j4
make test TESTS=tests/080-add-dic.phpt
```

Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add dictionary.cc tests/080-add-dic.phpt tests/data/extra.dic
git commit -m "feat(dictionary): implement addDictionary() with load-failure exception

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

---

## Task 15: Implement `Dictionary::getEncoding`, `getVersion`, `getWordChars` and 090-meta.phpt

**Goal:** All three metadata methods return non-empty strings. `getWordChars` uses the C++ class method via `reinterpret_cast`.

**Files:**
- Modify: `dictionary.cc`
- Create: `tests/090-meta.phpt`

- [ ] **Step 1: Write the failing test**

`tests/090-meta.phpt`:

```
--TEST--
Dictionary::getEncoding / getWordChars / getVersion return strings
--EXTENSIONS--
hunspell
--FILE--
<?php
$base = __DIR__ . '/data/test';
$dict = new Hunspell\Dictionary("$base.aff", "$base.dic");

$enc = $dict->getEncoding();
var_dump(is_string($enc));
var_dump(strlen($enc) > 0);
var_dump(stripos($enc, 'utf') !== false);

$wc = $dict->getWordChars();
var_dump(is_string($wc));

$ver = $dict->getVersion();
var_dump(is_string($ver));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
```

- [ ] **Step 2: Run, expect FAIL**

```bash
make test TESTS=tests/090-meta.phpt
```

- [ ] **Step 3: Implement the three methods**

For `getWordChars` we need access to the C++ `Hunspell` class. Add at the top of `dictionary.cc`:

```cpp
#include <hunspell/hunspell.hxx>
```

This header ships with libhunspell-dev. It exposes the `Hunspell` class and the `get_wordchars_cpp()` member function. `Hunhandle*` is the same address as the `Hunspell*` (the C API is a thin wrapper).

Then add the three methods:

```cpp
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_getEncoding, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, getEncoding) {
    ZEND_PARSE_PARAMETERS_NONE();
    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    const char *enc = Hunspell_get_dic_encoding(obj->handle);
    if (enc == nullptr) RETURN_EMPTY_STRING();
    RETURN_STRING(enc);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_getVersion, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, getVersion) {
    ZEND_PARSE_PARAMETERS_NONE();
    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    const char *ver = Hunspell_get_version(obj->handle);
    if (ver == nullptr) RETURN_EMPTY_STRING();
    RETURN_STRING(ver);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_getWordChars, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, getWordChars) {
    ZEND_PARSE_PARAMETERS_NONE();
    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    Hunspell *cpp = reinterpret_cast<Hunspell *>(obj->handle);
    const std::string &wc = cpp->get_wordchars_cpp();
    RETURN_STRINGL(wc.data(), static_cast<size_t>(wc.size()));
}
```

Methods table additions:

```cpp
    PHP_ME(Hunspell_Dictionary, getEncoding,  arginfo_dictionary_getEncoding,  ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Dictionary, getVersion,   arginfo_dictionary_getVersion,   ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Dictionary, getWordChars, arginfo_dictionary_getWordChars, ZEND_ACC_PUBLIC)
```

- [ ] **Step 4: Rebuild and run**

```bash
make -j4
make test TESTS=tests/090-meta.phpt
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add dictionary.cc tests/090-meta.phpt
git commit -m "$(cat <<'EOF'
feat(dictionary): implement getEncoding / getVersion / getWordChars

getWordChars uses the C++ class via reinterpret_cast<Hunspell*>(
Hunhandle*) because the C API only exposes the UTF-16 form.
Pinned by the libhunspell >= 1.6.0 minimum.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 16: clone-forbidden test and key-rejected test

**Goal:** `clone $dict` raises `Error`. Constructor with non-null `$key` raises `\InvalidArgumentException` (already implemented in Task 5, but covered explicitly by its own test).

**Files:**
- Create: `tests/100-clone-forbidden.phpt`, `tests/101-key-rejected.phpt`

- [ ] **Step 1: Write both tests**

`tests/100-clone-forbidden.phpt`:

```
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
```

`tests/101-key-rejected.phpt`:

```
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
```

- [ ] **Step 2: Run both — should PASS already**

```bash
make test TESTS="tests/100-clone-forbidden.phpt tests/101-key-rejected.phpt"
```

Both PASS because the underlying behavior was implemented in Tasks 4 and 5. If 101 fails, double-check Task 5 Step 3.

- [ ] **Step 3: Commit**

```bash
git add tests/100-clone-forbidden.phpt tests/101-key-rejected.phpt
git commit -m "test: clone-forbidden and key-rejected behaviour coverage

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

---

## Task 17: Update MINFO to print the actual libhunspell version

**Goal:** `phpinfo()` reports the libhunspell version pkg-config provided at compile time.

**Files:**
- Modify: `config.m4`, `hunspell.cc`

- [ ] **Step 1: Inject the version into compile flags via `config.m4`**

Modify `config.m4` — after the `PKG_CHECK_MODULES` line, capture the version:

```m4
  AC_DEFINE_UNQUOTED([HUNSPELL_LIB_VERSION], ["$(pkg-config --modversion hunspell)"],
    [Detected libhunspell version])
```

- [ ] **Step 2: Update `MINFO` in `hunspell.cc`**

```cpp
static PHP_MINFO_FUNCTION(hunspell) {
    php_info_print_table_start();
    php_info_print_table_header(2, "hunspell support", "enabled");
    php_info_print_table_row(2, "module version", PHP_HUNSPELL_VERSION);
    php_info_print_table_row(2, "libhunspell version", HUNSPELL_LIB_VERSION);
    php_info_print_table_end();
}
```

- [ ] **Step 3: Regenerate the build (re-run phpize because config.m4 changed)**

```bash
phpize --clean
phpize
./configure --enable-hunspell
make -j4
php -d extension="$(pwd)/modules/hunspell.so" -r 'phpinfo();' | grep -i hunspell
```

Expected: output contains both module version (`0.1.0`) and a real libhunspell version (e.g. `1.7.2`).

- [ ] **Step 4: Commit**

```bash
git add config.m4 hunspell.cc
git commit -m "feat(info): print detected libhunspell version in phpinfo()

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

---

## Task 18: Add `composer.json` (PIE day-0)

**Goal:** Root `composer.json` declares `type: php-ext` with PIE metadata.

**Files:**
- Create: `composer.json`

- [ ] **Step 1: Write `composer.json`**

```json
{
    "name": "codeconjure/php-hunspell",
    "description": "PHP binding for the Hunspell spell-checker and morphological analyzer",
    "type": "php-ext",
    "license": "PHP-3.01",
    "require": { "php": ">=8.2" },
    "php-ext": {
        "extension-name": "hunspell",
        "support-zts": true,
        "support-nts": true,
        "configure-options": []
    }
}
```

- [ ] **Step 2: Validate via composer**

```bash
composer validate --no-check-publish
```

Expected: `./composer.json is valid` (warnings about Packagist publish are fine).

- [ ] **Step 3: Try a local PIE install (if PIE is installed)**

```bash
pie install ./
php -m | grep -i hunspell
```

Expected: extension loaded. If `pie` is not installed locally, skip; CI will verify.

- [ ] **Step 4: Commit**

```bash
git add composer.json
git commit -m "build: add composer.json with PIE php-ext metadata

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

---

## Task 19: Add the GitHub Actions workflow

**Goal:** `.github/workflows/ci.yml` runs five jobs: `linux-pie` (PHP 8.2/8.3/8.4/8.5), `linux-phpize`, `linux-zts`, `macos`, `asan`.

**Files:**
- Create: `.github/workflows/ci.yml`

- [ ] **Step 1: Write the workflow**

`.github/workflows/ci.yml`:

```yaml
name: CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  linux-pie:
    runs-on: ubuntu-latest
    strategy:
      fail-fast: false
      matrix: { php: ['8.2', '8.3', '8.4', '8.5'] }
    steps:
      - uses: actions/checkout@v4
      - uses: shivammathur/setup-php@v2
        with:
          php-version: ${{ matrix.php }}
          tools: pie
          coverage: none
      - run: sudo apt-get update && sudo apt-get install -y libhunspell-dev pkg-config
      - run: pie install ./
      - run: php -m | grep -i hunspell
      - run: php run-tests.php -q tests/

  linux-phpize:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: shivammathur/setup-php@v2
        with: { php-version: '8.3', coverage: none }
      - run: sudo apt-get update && sudo apt-get install -y libhunspell-dev pkg-config
      - run: phpize
      - run: ./configure --enable-hunspell
      - run: make -j4
      - run: NO_INTERACTION=1 make test

  linux-zts:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: shivammathur/setup-php@v2
        with:
          php-version: '8.3'
          coverage: none
          extensions: ''
        env:
          phpts: zts
      - run: sudo apt-get update && sudo apt-get install -y libhunspell-dev pkg-config
      - run: phpize
      - run: ./configure --enable-hunspell
      - run: make -j4
      - run: NO_INTERACTION=1 make test

  macos:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v4
      - uses: shivammathur/setup-php@v2
        with:
          php-version: '8.3'
          tools: pie
          coverage: none
      - run: brew install hunspell pkg-config
      - run: pie install ./
      - run: php -m | grep -i hunspell
      - run: php run-tests.php -q tests/

  asan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: shivammathur/setup-php@v2
        with:
          php-version: '8.3'
          coverage: none
        env:
          phpts: nts
      - run: sudo apt-get update && sudo apt-get install -y libhunspell-dev pkg-config
      - run: phpize
      - run: ./configure --enable-hunspell CFLAGS='-fsanitize=address -g -O0' CXXFLAGS='-fsanitize=address -g -O0' LDFLAGS='-fsanitize=address'
      - run: make -j4
      - run: NO_INTERACTION=1 USE_ZEND_ALLOC=0 ASAN_OPTIONS=detect_leaks=1 make test
```

- [ ] **Step 2: Push and verify**

```bash
git add .github/workflows/ci.yml
git commit -m "$(cat <<'EOF'
ci: add GitHub Actions matrix (linux-pie / linux-phpize / linux-zts / macos / asan)

Five jobs ensure the extension builds and tests pass on Ubuntu under
PIE for PHP 8.2/8.3/8.4/8.5, on Ubuntu under classic phpize, on Ubuntu ZTS,
on macOS via brew + PIE, and on Ubuntu with AddressSanitizer enabled.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
git push
```

Watch the Actions tab until all five jobs go green. If a job fails on something CI-specific (e.g. setup-php tool name, brew package), patch in a follow-up commit. The local test suite is the source of truth; CI plumbing is the secondary surface.

---

## Task 20: Write `README.md`

**Goal:** Build/install instructions, minimal usage example, CI badge.

**Files:**
- Create: `README.md`

- [ ] **Step 1: Write README.md**

```markdown
# php-hunspell

[![CI](https://github.com/codeconjure/php-hunspell/actions/workflows/ci.yml/badge.svg)](https://github.com/codeconjure/php-hunspell/actions/workflows/ci.yml)

PHP 8.2+ binding for the [Hunspell](https://hunspell.github.io/) spell-checker and morphological analyzer. Exposes spell-check, suggestions, morphological analysis (`analyze`, `stem`, `generate`), runtime dictionary mutation (`add`, `addWithAffix`, `remove`, `addDictionary`), and dictionary metadata.

## Requirements

- PHP 8.2 or newer (NTS or ZTS)
- libhunspell 1.6.0 or newer + its development headers
- pkg-config
- A C++17-capable compiler

## Install via PIE (recommended)

```sh
pie install codeconjure/php-hunspell
```

## Install via phpize

```sh
git clone https://github.com/codeconjure/php-hunspell.git
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
$a->get('st');                            // stems
$a->has('hy');                            // bool
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
```

- [ ] **Step 2: Commit**

```bash
git add README.md
git commit -m "docs: add README with install instructions and usage example

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

---

## Task 21: Rewrite `CLAUDE.md` for the 0.1 architecture

**Goal:** The existing CLAUDE.md describes the legacy PHP 5 code that no longer exists. Replace it with documentation matching the new architecture so future Claude instances have an accurate map.

**Files:**
- Modify: `CLAUDE.md`

- [ ] **Step 1: Overwrite `CLAUDE.md`**

Replace the entire contents with:

```markdown
# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

PHP 8.2+ C++17 extension that binds libhunspell (≥ 1.6.0) to PHP. Exposes two classes — `Hunspell\Dictionary` (the binding) and `Hunspell\Analysis` (readonly value object for parsed morphological analyses) — plus a `Hunspell\Exception\Exception` marker interface and the concrete `DictionaryLoadException`.

The extension was rewritten in 0.1; the prior PHP 5 implementation is in git history before the `rewrite:` commits.

## Build & test

The build uses `pkg-config` exclusively to locate libhunspell — there is no filesystem-search fallback. `libhunspell.pc` must be visible to `pkg-config`.

```sh
phpize
./configure --enable-hunspell
make -j4
NO_INTERACTION=1 make test
```

PIE install (PHP Installer for Extensions):

```sh
pie install ./
```

Single phpt test:

```sh
make test TESTS=tests/040-analyze.phpt
```

## Architecture

Source split:

- `hunspell.cc` — module entry only (MINIT/MSHUTDOWN/MINFO), no method bodies.
- `dictionary.cc` — every `Hunspell\Dictionary` method + the shared list-free helper.
- `analysis.cc` — `Hunspell\Analysis` methods + `hunspell_parse_analysis_line()`, which both `Analysis::__construct` and `Dictionary::analyze` call.
- `exceptions.cc` — registration of `Hunspell\Exception\Exception` and `DictionaryLoadException`.

### Memory ownership — non-negotiable rules

1. **Never `efree` a ZPP-parsed string.** `Z_PARAM_STR(word)` returns a borrowed pointer into a PHP-owned `zend_string`. We only read `ZSTR_VAL(word)`. The legacy PHP 5 code's `efree(word)` was heap corruption.
2. **Never use libc `free()` on a libhunspell-allocated `char**` list.** All list-returning Hunspell calls funnel through the file-static helper `hunspell_strlist_to_array_and_free` in `dictionary.cc`, which always calls `Hunspell_free_list`. If you add another list-returning method, route it through that helper.

### Dictionary object layout

```c
typedef struct {
    Hunhandle *handle;     // libhunspell-owned, freed in free_obj
    zend_object std;       // MUST BE LAST (PHP 7/8 idiom)
} php_hunspell_object;
```

`offset = XtOffsetOf(php_hunspell_object, std)` in the custom handlers. `clone_obj = nullptr` forbids cloning at engine level.

### Analysis state model

`Hunspell\Analysis` is declared `final readonly class` with two typed properties (`string $raw`, `array $fields`). Both `Analysis::__construct` (called from userland) and `Dictionary::analyze` (called from C) populate those properties via `zend_update_property*`. The fast path bypasses the PHP-level constructor; the userland and fast paths must produce byte-identical state — verified by `tests/041-analysis-userland.phpt`.

### Parser semantics — public contract

`hunspell_parse_analysis_line(const char *raw, size_t len, zval *out)`:

- Tokenizes on whitespace.
- For each token, splits on the first `:`. Substring before is the key; after is the value.
- Repeated keys append to a list.
- Tokens without `:` are silently skipped.
- Empty input → empty associative array.

Downstream modules (planned `MagyarIspell\` in 0.2) build on these semantics — do not change them without bumping the major version.

### `getWordChars` couples to the C++ ABI

The C API only exposes UTF-16 word chars. To return UTF-8, `Dictionary::getWordChars` casts `Hunhandle*` → `Hunspell*` and calls `get_wordchars_cpp()`. This is deliberate and pinned by the libhunspell ≥ 1.6.0 minimum. Do not "fix" it by trying to use the C API.

## CI

`.github/workflows/ci.yml` runs five jobs:
- `linux-pie` × PHP {8.2, 8.3, 8.4, 8.5}: PIE install + phpt
- `linux-phpize`: classic phpize build + phpt
- `linux-zts`: ZTS build + phpt (no shared-Dictionary test; concurrent use is not contracted)
- `macos`: macOS + brew + PIE
- `asan`: AddressSanitizer build to catch any regression of the memory-bug class the legacy code suffered from

If you add a new method that returns a list, the ASAN job must stay green.

## Roadmap

- 0.2 — `MagyarIspell\Analyzer` (`partition`, `syllabify`, `rawTag`, `hyphenate`). Separate brainstorming session pending.
- 0.3 — `Hyphen\Dictionary` (libhyphen wrapper). Separate brainstorming session pending.
- 0.4+ — `MyThes`, `DictionaryInfo`, `PersonalDictionary`, `TextChecker`. Low priority.

## Out of scope for 0.1 (do not add without an explicit task)

- Hungarian-specific helpers on `Analysis` (`stems()`, `partOfSpeech()`, `hyphenationHints()`). These belong in `MagyarIspell\` in 0.2.
- Procedural function layer (`hunspell_open`, etc.). Modern OOP only.
- Encrypted-dictionary key support (constructor argument exists but throws).
- `Hunspell_spell_with_info` detailed-result bits.
```

- [ ] **Step 2: Commit**

```bash
git add CLAUDE.md
git commit -m "$(cat <<'EOF'
docs: rewrite CLAUDE.md for the 0.1 C++17 architecture

Replaces the legacy PHP 5 documentation. Captures the new file
layout, the two memory-ownership invariants, the Analysis state
model, the parser contract, the deliberate C++ coupling for
getWordChars, and the explicit non-goals.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 22: Final integration verification

**Goal:** Every phpt is green locally on a fresh build. CI is green on master. The extension prints its version and the libhunspell version. The 0.1 tag is created.

- [ ] **Step 1: Fresh clean build**

```bash
phpize --clean
phpize
./configure --enable-hunspell
make -j4
```

- [ ] **Step 2: Run the entire test suite**

```bash
NO_INTERACTION=1 make test
```

Expected: every test reports `PASS`. Zero `FAIL`, zero `BORK`, zero `LEAK`.

- [ ] **Step 3: Verify phpinfo output**

```bash
php -d extension="$(pwd)/modules/hunspell.so" -r 'phpinfo();' | grep -i hunspell
```

Expected output includes:
- `hunspell support => enabled`
- `module version => 0.1.0`
- `libhunspell version => <actual version>`

- [ ] **Step 4: Confirm CI is green**

```bash
git push
gh run watch
```

Expected: all five jobs green.

- [ ] **Step 5: Tag the release**

```bash
git tag -a 0.1.0 -m "php-hunspell 0.1.0 — modern C++17 rewrite"
git push origin 0.1.0
```

- [ ] **Step 6: Confirm done state per spec §9**

Walk through the definition-of-done checklist in `docs/superpowers/specs/2026-05-20-php-hunspell-0.1-design.md` §9. Every box must be checked before the 0.1 tag is considered shipped.

---

## Self-review

**Spec coverage** — every section of the spec maps to at least one task:

| Spec section | Task(s) |
|---|---|
| §3 Architecture — config.m4 | 1, 17 |
| §3 Architecture — file layout | 1 |
| §4 Components — Dictionary | 4 |
| §4 Components — Analysis | 8 |
| §4 Components — Exceptions | 3 |
| §4 Components — module entry | 1, 3, 4, 8, 17 |
| §5 API — `__construct`, key rejection | 5 |
| §5 API — `spell` | 6 |
| §5 API — `suggest` | 7 |
| §5 API — `analyzeRaw` | 9 |
| §5 API — `analyze` | 10 |
| §5 API — `stem` | 11 |
| §5 API — `generate` (both modes) | 12 |
| §5 API — `add`/`addWithAffix`/`remove` | 13 |
| §5 API — `addDictionary` | 14 |
| §5 API — `getEncoding`/`getWordChars`/`getVersion` | 15 |
| §5 API — Analysis class | 8, 10 |
| §5 API — clone forbidden | 4 (handler), 16 (test) |
| §6 Data flow — helper | 7 |
| §6 Data flow — `analyze` direct property writes | 10 |
| §6 Data flow — parser contract | 8 |
| §7 Error handling — DictionaryLoadException | 3 (class), 5/14 (sites) |
| §7 Error handling — TypeError on bad array element | 12 |
| §7 Error handling — InvalidArgumentException on $key | 5, 16 (test) |
| §7 Error handling — clone Error | 4 (handler), 16 (test) |
| §8 Testing — phpt suite | every TDD task |
| §8 Testing — PIE composer.json | 18 |
| §8 Testing — GitHub Actions matrix | 19 |
| §9 Roadmap — README | 20 |
| §9 Roadmap — CLAUDE.md update | 21 |
| §9 Roadmap — final verification + tag | 22 |

No gaps.

**Type consistency** — Method names match the spec (`__construct`, `spell`, `suggest`, `analyzeRaw`, `analyze`, `stem`, `generate`, `add`, `addWithAffix`, `remove`, `addDictionary`, `getEncoding`, `getWordChars`, `getVersion`; `getRaw`, `getFields`, `get`, `has`). File names match (`dictionary.cc`, `analysis.cc`, `exceptions.cc`, `hunspell.cc`, `php_hunspell.h`). Helper function name is consistent across tasks: `hunspell_strlist_to_array_and_free` (used in Tasks 7, 9, 11, 12, 14).
