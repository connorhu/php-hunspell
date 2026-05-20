# php-hunspell 0.1 — design

**Status:** approved, ready for implementation plan
**Date:** 2026-05-20
**Scope:** version 0.1 of the rewritten extension. Follow-up brainstorming sessions will produce separate designs for 0.2 (`MagyarIspell\Analyzer`) and 0.3 (`Hyphen\Dictionary`).

---

## 1. Background

The current repository contains a PHP 5–era C extension (`hunspell.c`, `php_hunspell.h`, `config.m4`, `tests/001.phpt`) that wraps a small slice of libhunspell (`spell`, `suggest`, plus open/close/get_encoding). It targets the Zend Engine 2 ABI (TSRMLS macros, `zend_object_store_get_object`, the old 2-arg `RETURN_STRING`) and will not build on PHP 7+ without a port that is effectively a rewrite. Independent of the ABI, the existing code carries several correctness bugs (`efree` on ZPP-owned strings, libc `free()` on libhunspell-allocated string lists, no double-free guard on `close()`), and only exposes ~15% of the libhunspell API surface — the morphological calls (`analyze`, `stem`, `generate`) that motivate Hungarian use cases are entirely absent.

This document specifies the 0.1 release: a clean replacement that targets PHP 8.2+, exposes the full Hunspell binding surface needed for downstream Hungarian-language modules, and ships with day-0 PIE support and a CI matrix that exercises NTS, ZTS, macOS and AddressSanitizer builds.

The 0.1 release is the foundation; the 0.2 and 0.3 releases (`MagyarIspell\*` and `Hyphen\*`) depend on it but are designed and shipped separately.

## 2. Scope and non-goals

### In scope for 0.1

- A modern PHP 8.2+ C++17 extension with namespaced classes under `Hunspell\`.
- Two PHP classes: `Hunspell\Dictionary` (the libhunspell binding) and `Hunspell\Analysis` (a readonly value object representing one morphological analysis line).
- One exception type plus a marker interface: `Hunspell\Exception\DictionaryLoadException`, `Hunspell\Exception\Exception`.
- Full P0 surface (lifecycle, `spell`, `suggest`, `analyzeRaw`, `analyze`, `stem`, `generate`, `getEncoding`, `getWordChars`) and full P1 surface (`add`, `addWithAffix`, `remove`, `addDictionary`, `getVersion`).
- pkg-config–based libhunspell detection in `config.m4` with a minimum version of 1.6.0.
- Day-0 PIE distribution metadata (`composer.json` with `"type": "php-ext"`).
- Day-0 GitHub Actions workflow covering Ubuntu × {PHP 8.2, 8.3, 8.4}, an Ubuntu ZTS job, a macOS job, and an AddressSanitizer job.
- A test suite of phpt files (≈13–15 files at completion) plus a tiny `.aff`/`.dic` test dictionary.

### Explicit non-goals for 0.1

- Encrypted-dictionary key support (`?string $key` is accepted in the signature but rejected at construct time; full support is P2).
- The Hunspell XML API (`Hunspell_spell_with_info` and the detailed result bits: compound, forbidden, warn) — P2.
- `Hunspell\DictionaryInfo` (static .aff/.dic parser/inspector) — deferred to 0.4 or later.
- `Hunspell\PersonalDictionary`, `Hunspell\TextChecker` — deferred to 0.4 or later.
- Hungarian-specific helpers on `Analysis` (`stems()`, `partOfSpeech()`, `hyphenationHints()`) — these belong in the `MagyarIspell\` namespace shipped in 0.2 and must not leak into the generic Hunspell binding.
- A procedural function layer (`hunspell_open`, etc.) — modern OOP only. No backwards-compatibility shim with the PHP 5 API.
- Documented thread safety for sharing a `Dictionary` across threads in ZTS builds. ZTS is *built and tested* but a single `Dictionary` is not guaranteed to be safe for concurrent use across threads.
- A bundled `hu_HU` dictionary. Dictionary distribution is the responsibility of the user or the distro packager.

## 3. Architecture

A single PHP extension registered as `hunspell`, implemented in C++17, built with `phpize` / `config.m4`. The repository’s existing PHP 5 source files (`hunspell.c`, `php_hunspell.h`, `config.m4`, `tests/001.phpt`) are removed in the first rewrite commit.

**Build detection.** `config.m4` uses `PKG_CHECK_MODULES([HUNSPELL], [hunspell >= 1.6.0])`. There is no fallback search of `/usr`, `/usr/local`, or Homebrew prefixes; if `hunspell.pc` is not on the pkg-config search path, `./configure` fails with a clear error. This matches the requirement that the build relies on standard, declarative dependency metadata rather than ad-hoc filesystem probing.

**Minimum libhunspell version: 1.6.0.** This is the version at which the C API surface used in 0.1 (`Hunspell_get_version`, `Hunspell_generate2`, `Hunspell_free_list`, `Hunspell_add_with_affix`) is reliably present across distributions.

**Source file layout.**

```
config.m4              pkg-config detection + PHP_REQUIRE_CXX + PHP_NEW_EXTENSION (multi-file)
php_hunspell.h         shared header — module entry, struct definitions, class entries, parser proto
hunspell.cc            module entry: MINIT/MSHUTDOWN/MINFO, registers all classes
dictionary.cc          Hunspell\Dictionary methods
analysis.cc            Hunspell\Analysis methods and the line parser
exceptions.cc          Hunspell\Exception\* class registration
tests/
  data/
    test.aff           SET UTF-8, TRY rules, 1-2 affix groups
    test.dic           ~5 words with morphological fields
    hu-tiny.aff/.dic   optional: minimal Hungarian inflection group for addWithAffix tests
  001-load.phpt … N    one phpt file per behavior
composer.json          PIE metadata
.github/workflows/ci.yml   CI matrix
README.md              build/install + minimal usage example
CLAUDE.md              updated to match the 0.1 architecture
```

The multi-file split is deliberate: `Dictionary`, `Analysis`/parser, and exceptions are three distinct responsibilities, and forcing them into a single 1500-line `hunspell.cc` would obscure ownership.

**ZTS posture.** The extension declares `support-zts: true` in `composer.json` and a ZTS job runs in CI, but no global state crosses threads — each `Dictionary` owns its own `Hunhandle*` and contains no module globals. Concurrent use of the *same* `Dictionary` from multiple threads is not tested and is not part of the contract.

## 4. Components and responsibilities

### `Hunspell\Dictionary` — `dictionary.cc`

Owns the libhunspell handle. Uses the PHP 7/8 idiomatic embedding convention with `zend_object std` as the **last** field of the custom object struct:

```c
typedef struct {
    Hunhandle *handle;
    zend_object std;        // must be last
} php_hunspell_object;
```

Custom `zend_object_handlers` set `offset = XtOffsetOf(php_hunspell_object, std)`. The `free_obj` handler calls `Hunspell_destroy(obj->handle)` if non-null, then `zend_object_std_dtor(&obj->std)`. The `clone_obj` handler is set to `NULL` so that `clone $dict` causes the engine to throw an `Error` ("Trying to clone an uncloneable object of class Hunspell\\Dictionary"). All Hunspell C API calls live in this translation unit.

The list-returning methods (`suggest`, `analyzeRaw`, `stem`, both `generate` variants) all pass through a single file-static helper `hunspell_strlist_to_array_and_free` (see §6) so the `Hunspell_free_list` call cannot be forgotten.

### `Hunspell\Analysis` — `analysis.cc`

A `readonly class` (PHP 8.2+) with two public properties: `string $raw` and `array $fields`. The class has a public constructor that accepts the raw analysis string and parses it. The parser itself (`parse_analysis_line`) is a standalone C++ function exposed via `php_hunspell.h`, so both `Analysis::__construct` (called from userland) and `Dictionary::analyze` (called from C) share the same logic.

When `Dictionary::analyze` constructs Analysis objects, it uses `object_init_ex` plus direct property writes (bypassing the userland `__construct` for performance). The userland constructor exists primarily to keep the value object independently testable and to allow downstream consumers to parse a raw Hunspell analysis string offline. Both code paths must produce bit-identical state — verified by test 041-analysis-userland.phpt (see §8).

### `Hunspell\Exception\*` — `exceptions.cc`

Registers the marker interface `Hunspell\Exception\Exception` and the concrete `Hunspell\Exception\DictionaryLoadException extends \RuntimeException implements Exception`. No logic, just class entry registration.

### `hunspell.cc` — module entry

MINIT calls three registration functions (`register_dictionary_class`, `register_analysis_class`, `register_exception_classes`). MINFO prints, via `php_info_print_table_*`, the libhunspell version (compiled in from pkg-config’s `HUNSPELL_CFLAGS` as a preprocessor define) and the module version (`0.1.0`). There is no MSHUTDOWN logic, no INI entries, and no module globals.

### `php_hunspell.h`

Declares `php_hunspell_object` and `php_hunspell_analysis_object` (the latter contains only the `zend_object std`, since all Analysis state is stored as PHP properties). Declares the four extern `zend_class_entry*` (Dictionary, Analysis, Exception interface, DictionaryLoadException) and the `parse_analysis_line` prototype.

### `config.m4`

```m4
PHP_ARG_ENABLE(hunspell, whether to enable hunspell support,
[  --enable-hunspell        Enable hunspell support])

if test "$PHP_HUNSPELL" != "no"; then
  PHP_REQUIRE_CXX()
  PKG_CHECK_MODULES([HUNSPELL], [hunspell >= 1.6.0])
  PHP_EVAL_INCLINE($HUNSPELL_CFLAGS)
  PHP_EVAL_LIBLINE($HUNSPELL_LIBS, HUNSPELL_SHARED_LIBADD)
  PHP_SUBST(HUNSPELL_SHARED_LIBADD)
  PHP_NEW_EXTENSION(hunspell,
    hunspell.cc dictionary.cc analysis.cc exceptions.cc,
    $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -std=c++17)
fi
```

## 5. Complete PHP API surface (0.1)

```php
<?php
namespace Hunspell;

final class Dictionary
{
    // — lifecycle —
    public function __construct(
        string $affPath,
        string $dicPath,
        ?string $key = null,   // P2: encrypted-dic key. 0.1 throws InvalidArgumentException if non-null.
    );
    // __destruct: Hunspell_destroy, GC-driven
    // clone $dict → Error (clone_obj handler is NULL)

    // — spell-check (P0) —
    public function spell(string $word): bool;
    /** @return list<string> */
    public function suggest(string $word): array;

    // — morphology (P0) —
    /** @return list<string> raw "st:x po:y …" lines */
    public function analyzeRaw(string $word): array;
    /** @return list<Analysis> parsed analyses */
    public function analyze(string $word): array;
    /** @return list<string> */
    public function stem(string $word): array;
    /** @return list<string> */
    public function generate(string $word, string|array $model): array;
    // model string → Hunspell_generate(word, model)
    // model array  → Hunspell_generate2(word, model[], n)

    // — runtime dictionary mutation (P1) —
    public function add(string $word): void;
    public function addWithAffix(string $word, string $example): void;
    public function remove(string $word): void;
    public function addDictionary(string $dicPath): void;   // may throw DictionaryLoadException

    // — metadata (P0/P1) —
    public function getEncoding(): string;       // P0: Hunspell_get_dic_encoding
    public function getWordChars(): string;      // P0: Hunspell::get_wordchars_cpp() (C++ class method)
    public function getVersion(): string;        // P1: the .aff VERSION string (not libhunspell version)
}

final readonly class Analysis
{
    public string $raw;
    /** @var array<string, list<string>> */
    public array $fields;

    public function __construct(string $raw);    // parses $raw into $fields

    public function getRaw(): string;
    /** @return array<string, list<string>> */
    public function getFields(): array;
    /** @return list<string> values for a single key, empty array if absent */
    public function get(string $key): array;
    public function has(string $key): bool;
}

namespace Hunspell\Exception;

interface Exception {}                          // marker — every Hunspell-layer exception implements this

final class DictionaryLoadException
    extends \RuntimeException
    implements Exception {}
```

**Deliberate decisions captured in the surface:**

1. `suggest` / `analyze` / `analyzeRaw` / `stem` / `generate` return an empty array when there is no result. They never return `false`. The PHP 5 code’s mixed `bool|array` return is dropped.
2. `generate` is polymorphic on `$model`: strings dispatch to `Hunspell_generate`, arrays dispatch to `Hunspell_generate2`. PHP 8.2 union types enforce the boundary; non-string array elements raise a `TypeError` (see §7).
3. `getWordChars` returns UTF-8. Since libhunspell’s C API only exposes UTF-16 word chars, the C++ method `Hunspell::get_wordchars_cpp()` is called via `reinterpret_cast<Hunspell*>(handle)`. This is a deliberate, documented coupling to the C++ ABI permitted by the C++17 build mode and pinned by the minimum libhunspell version.
4. `getVersion` returns the `VERSION` value declared by the loaded `.aff` file, not the libhunspell library version. The PHP-doc on the method must spell this out.
5. `__clone` is forbidden at the engine level (the `clone_obj` handler is `NULL`); the user sees a fatal `Error` from the engine.
6. `Dictionary` has no `close()` / `open()` / reopen surface. Lifecycle is constructor-to-destructor. A different dictionary means a new object.
7. `Analysis` has a public constructor — additive to the original prose proposal, motivated by parser-testability and offline parsing of pre-existing analysis strings.

## 6. Data flow and memory semantics

### Two hard invariants

1. **Never `efree` a ZPP-owned string.** `Z_PARAM_STR(word)` returns a pointer into a PHP-owned `zend_string` whose lifetime is the parameter zval. We only read `ZSTR_VAL(word)`. The PHP 5 code’s `efree(word)` in `spell()` and `suggest()` is heap corruption.
2. **Never use libc `free()` on a libhunspell-allocated string list.** Every `Hunspell_*` call that populates a `char ***` parameter must be paired with `Hunspell_free_list(handle, &slst, n)`. The PHP 5 code’s `free(slist)` in `suggest()` leaks the individual suggestion strings.

Both invariants are enforced through a single file-static helper in `dictionary.cc`:

```cpp
static void hunspell_strlist_to_array_and_free(
    Hunhandle *h, char **slst, int n, zval *return_value)
{
    array_init_size(return_value, n);
    for (int i = 0; i < n; i++) {
        add_next_index_string(return_value, slst[i]);   // PHP 7+ copies
    }
    Hunspell_free_list(h, &slst, n);                    // always runs
}
```

Every list-returning method funnels through this helper, making leaks / double-frees / wrong-frees impossible to introduce in one place rather than each method individually.

### Per-method data flow

| Method | C API call | Memory |
|---|---|---|
| `__construct` | `Hunspell_create(aff, dic)` | Stores `Hunhandle*`; throws `DictionaryLoadException` on NULL |
| `spell` | `Hunspell_spell` | No allocation |
| `suggest` | `Hunspell_suggest` → `char**` | `hunspell_strlist_to_array_and_free` |
| `analyzeRaw` | `Hunspell_analyze` → `char**` | `hunspell_strlist_to_array_and_free` |
| `stem` | `Hunspell_stem` → `char**` | `hunspell_strlist_to_array_and_free` |
| `generate` (string model) | `Hunspell_generate(word, model)` | `hunspell_strlist_to_array_and_free` |
| `generate` (array model) | `Hunspell_generate2(word, char*[], n)` (stack array of `ZSTR_VAL` pointers) | `hunspell_strlist_to_array_and_free` |
| `analyze` | `Hunspell_analyze` → raw, then construct Analysis objects (below) | single `Hunspell_free_list` after the construction loop |
| `add` / `addWithAffix` / `remove` | `Hunspell_add*` | None; return value ignored |
| `addDictionary` | `Hunspell_add_dic` | Nonzero → `DictionaryLoadException` |
| `getEncoding` / `getVersion` | `Hunspell_get_dic_encoding` / `Hunspell_get_version` | `RETURN_STRING` copies; returned pointer is libhunspell-owned |
| `getWordChars` | `reinterpret_cast<Hunspell*>(h)->get_wordchars_cpp()` | Internal `std::string&`; `RETURN_STRINGL(.data(), .size())` |
| `__destruct` (`free_obj`) | `Hunspell_destroy` if non-null | Plus `zend_object_std_dtor` |

### The `analyze()` special path

Analysis objects are constructed by writing properties directly rather than calling `Analysis::__construct` from C, to avoid a PHP-level method invocation per result:

```cpp
char **slst = nullptr;
int n = Hunspell_analyze(handle, &slst, ZSTR_VAL(word));
array_init_size(return_value, n);
for (int i = 0; i < n; i++) {
    zval analysis_zv, fields_zv;
    object_init_ex(&analysis_zv, hunspell_analysis_ce);

    // Set the readonly $raw and $fields properties directly. The engine
    // permits the first write per readonly property; subsequent userland
    // writes are still rejected. zend_update_property* is the supported
    // entry point during initialization.
    zend_update_property_string(/* raw */ ..., slst[i]);
    parse_analysis_line(slst[i], strlen(slst[i]), &fields_zv);
    zend_update_property(/* fields */ ..., &fields_zv);
    zval_ptr_dtor(&fields_zv);

    add_next_index_zval(return_value, &analysis_zv);
}
Hunspell_free_list(handle, &slst, n);
```

The fast path (direct property writes) and the slow path (`new Analysis($raw)` from userland) must produce bit-identical state — see test 041-analysis-userland.phpt.

### Parser semantics

`parse_analysis_line(const char *raw, size_t len, zval *out_fields)`:

- Tokenizes the input on whitespace.
- For each token, the substring before the first `:` is the key; the substring after is the value.
- Repeated keys append to the value list (`array<string, list<string>>`).
- Tokens without a `:` are silently skipped. Empty input produces an empty fields array.
- This is documented as the public contract; downstream parsers (Magyar Ispell layer in 0.2) build on these semantics.

## 7. Error handling

The exception surface is intentionally minimal: one marker interface and one concrete class. PHP engine–level exceptions (`TypeError`, `ArgumentCountError`, `Error`) cover most input mistakes.

### Where each exception is thrown

| Situation | Exception | Message |
|---|---|---|
| `__construct` — `Hunspell_create` returns NULL | `Hunspell\Exception\DictionaryLoadException` | `"Failed to load Hunspell dictionary (aff='{aff}', dic='{dic}')"` |
| `__construct` — `$key !== null` | `\InvalidArgumentException` | `"Encrypted dictionary keys are not supported in this release"` |
| `addDictionary` — `Hunspell_add_dic` returns nonzero | `Hunspell\Exception\DictionaryLoadException` | `"Failed to add dictionary '{path}' to handle"` |
| `generate(word, array $model)` — element is not a string | `\TypeError` (via `zend_argument_type_error`) | `"Argument #2 ($model) item #{i} must be of type string, {given} given"` |
| `new Analysis(<wrong type>)` | `\TypeError` | engine-provided ZPP message |
| `clone $dict` | `\Error` | `"Trying to clone an uncloneable object of class Hunspell\Dictionary"` (engine, because `clone_obj` is NULL) |
| ZPP wrong type / wrong arg count | `\TypeError` / `\ArgumentCountError` | engine-provided |

### Explicitly NOT errors

- An empty result is never an error. `suggest` / `analyze` / `analyzeRaw` / `stem` / `generate` return `[]` when there is no match.
- A malformed analysis token (no `:`, empty) is silently skipped by the parser.
- `Hunspell_add` / `_addWithAffix` / `_remove` return values are ignored. The methods are `void` and never throw.
- No pre-check for `.aff` / `.dic` existence before `Hunspell_create`. libhunspell’s NULL return is sufficient; the paths in the exception message let the caller diagnose.

### Catching

```php
try {
    $dict = new Hunspell\Dictionary($aff, $dic);
} catch (Hunspell\Exception\Exception $e) {
    // Any Hunspell-layer failure
}
```

The marker interface looks thin in 0.1 (only one concrete class implements it), but the 0.2 (`MagyarIspell\*`) and 0.3 (`Hyphen\*`) modules will add their own exception types that implement the same interface, so the catch-all pattern remains a one-liner.

## 8. Testing, PIE distribution, and GitHub Actions

### Test suite

13–15 phpt files at completion. Each file tests one behavior. Initial set (the “day-0 four” are required green from the first commit):

```
tests/data/test.aff             SET UTF-8, TRY rules, 1-2 affix groups
tests/data/test.dic             ~5 words with morphological fields
tests/data/hu-tiny.aff|.dic     optional: minimal Hungarian inflection group

001-load.phpt                   ✔ day-0  successful + failing __construct
010-spell.phpt                  ✔ day-0  positive / negative spell
020-suggest.phpt                ✔ day-0  suggest on correct word returns [], not false
030-analyze-raw.phpt                     analyzeRaw returns the morph strings
040-analyze.phpt                ✔ day-0  analyze returns parsed Analysis objects (parser validation)
041-analysis-userland.phpt               new Analysis($raw) matches Dictionary::analyze output
050-stem.phpt                            stem
060-generate-string.phpt                 generate with string model
061-generate-array.phpt                  generate with array model
062-generate-typeerror.phpt              array element non-string → TypeError
070-add-affix.phpt                       addWithAffix then spell / suggest
080-add-dic.phpt                         addDictionary success + failure (DictionaryLoadException)
090-meta.phpt                            getEncoding / getWordChars / getVersion
100-clone-forbidden.phpt                 clone → Error
101-key-rejected.phpt                    non-null $key → InvalidArgumentException
```

The test dictionary is a synthetic minimal `.aff`/`.dic` that exercises every API surface. Real Hungarian content is intentionally not used (downstream 0.2 tests handle that).

### PIE distribution

`composer.json` at the repository root from day-0:

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

`"type": "php-ext"` tells PIE this is a buildable extension. `pie install ./` (local) or `pie install codeconjure/php-hunspell` (after release to Packagist) runs `phpize && ./configure && make && make install` and adds the `extension=hunspell` line to the active `php.ini`. The declared ZTS support is backed by a real ZTS job in CI (below), not just a flag.

### GitHub Actions matrix

`.github/workflows/ci.yml` runs five jobs:

| Job | OS | PHP | Build | Tests |
|---|---|---|---|---|
| `linux-pie` | ubuntu-latest | 8.2 / 8.3 / 8.4 (matrix) | `pie install ./` | `make test` |
| `linux-phpize` | ubuntu-latest | 8.3 | `phpize && ./configure && make` | `make test` |
| `linux-zts` | ubuntu-latest | 8.3 (ZTS) | phpize | `make test` |
| `macos` | macos-latest | 8.3 | `pie install ./` (libhunspell via brew) | `make test` |
| `asan` | ubuntu-latest | 8.3-debug (`--enable-debug` + ASAN flags) | phpize | `make test` under ASAN |

The `asan` job exists specifically to catch the class of memory bug present in the PHP 5 code (`efree` on ZPP strings, libc `free` on libhunspell lists). If a regression is introduced, ASAN turns it red.

`shivammathur/setup-php@v2` installs PHP and PIE (PIE is supported in the `tools:` list). libhunspell is installed via `apt-get install libhunspell-dev pkg-config` on Ubuntu and via `brew install hunspell pkg-config` on macOS.

Sketch of the `linux-pie` job:

```yaml
strategy:
  matrix: { php: ['8.2', '8.3', '8.4'] }
steps:
  - uses: actions/checkout@v4
  - uses: shivammathur/setup-php@v2
    with: { php-version: ${{ matrix.php }}, tools: pie, coverage: none }
  - run: sudo apt-get update && sudo apt-get install -y libhunspell-dev pkg-config
  - run: pie install ./
  - run: php -m | grep -i hunspell
  - run: php run-tests.php -q tests/
```

## 9. Roadmap and deliverables

### Definition of done for 0.1

All of the following must hold before tagging 0.1:

- The complete PHP API in §5 is implemented (Dictionary 14 methods + Analysis 5 methods + lifecycle handlers).
- All phpt tests are green on every CI job (Linux × {8.2, 8.3, 8.4} + ZTS + macOS + ASAN).
- The four day-0 phpt tests (001, 010, 020, 040) are green from the first commit.
- `pie install ./` succeeds on both Ubuntu and macOS GitHub runners.
- `composer.json` declares `"type": "php-ext"`.
- `README.md` covers build, install, and a minimal usage example, plus a CI badge.
- The ASAN job completes one full test cycle with no leak, no use-after-free, no invalid free.
- `php -d extension=hunspell.so -r 'phpinfo();'` prints the libhunspell version and the module version (`0.1.0`).

### Repository deliverables (day-0 commit onward)

“Day-0” means the first commit of the 0.1 rewrite. Files marked ✔ are committed in that first commit; the rest land iteratively but the CI stays green for the already-implemented surface.

```
.github/workflows/ci.yml          ✔ day-0
composer.json                     ✔ day-0
config.m4                         ✔ day-0
php_hunspell.h                    ✔ day-0
hunspell.cc                       ✔ day-0  (module init only)
dictionary.cc                     iterative (P0 methods first)
analysis.cc                       ✔ day-0  (parser + Analysis class)
exceptions.cc                     ✔ day-0
tests/data/test.aff               ✔ day-0
tests/data/test.dic               ✔ day-0
tests/001-load.phpt               ✔ day-0
tests/010-spell.phpt              ✔ day-0
tests/020-suggest.phpt            ✔ day-0
tests/040-analyze.phpt            ✔ day-0
tests/...                         iterative, one per implemented method
README.md                         ✔ day-0
CLAUDE.md                         updated to match the 0.1 architecture
```

### What follows 0.1

- **0.2** — `MagyarIspell\Analyzer` with `partition()`, `syllabify()`, `rawTag()`, `hyphenate()`. Separate brainstorming session; depends on the public API of 0.1.
- **0.3** — `Hyphen\Dictionary` wrapping libhyphen. Separate brainstorming session; independent of 0.1 in code, linked only by the namespace convention.
- **0.4+** — `MyThes\Thesaurus`, `Hunspell\DictionaryInfo`, `Hunspell\PersonalDictionary`, `Hunspell\TextChecker`. Low priority; gated on real-world demand from 0.1–0.3 users.

## 10. Open questions

None blocking implementation. Items deliberately deferred:

- The exact PIE `tools:` integration path with `setup-php` may evolve; the workflow may need a one-off `composer global require php/pie` step if PIE is not yet first-class in the action. This is a CI plumbing detail, not a design decision.
- The shape of the `Analysis` field accessors may grow convenience methods in 0.2 (the Magyar Ispell layer), but those live in `MagyarIspell\` and do not affect the 0.1 API.
