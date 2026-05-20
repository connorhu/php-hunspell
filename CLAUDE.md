# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

PHP 8.2+ C++17 extension that binds libhunspell (≥ 1.6.0) to PHP. Exposes two classes — `Hunspell\Dictionary` (the binding) and `Hunspell\Analysis` (readonly value object for parsed morphological analyses) — plus a `Hunspell\Exception\Exception` marker interface and the concrete `DictionaryLoadException`.

The extension was rewritten in 0.1. The prior PHP 5 implementation lives in git history before the `rewrite:` commit on `worktree-rewrite-0.1`.

## Build & test

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

After modifying `config.m4`, `phpize --clean && phpize && ./configure --enable-hunspell` is required to pick up the change — `make` alone won't regenerate `config.h`.

## Architecture

Source split:

- `hunspell.cc` — module entry only (`MINIT` registers all classes, `MINFO` prints versions). The `#include "config.h"` guard at the top picks up `HUNSPELL_LIB_VERSION` from `config.m4`'s `AC_DEFINE_UNQUOTED`.
- `dictionary.cc` — every `Hunspell\Dictionary` method, the shared `hunspell_strlist_to_array_and_free` helper, custom object handlers.
- `analysis.cc` — `Hunspell\Analysis` methods and `hunspell_parse_analysis_line()`, which both `Analysis::__construct` and `Dictionary::analyze` call.
- `exceptions.cc` — registration of `Hunspell\Exception\Exception` (interface) and `DictionaryLoadException` (extends `\RuntimeException`, implements the interface, `ZEND_ACC_FINAL`).

### Memory ownership — non-negotiable rules

1. **Never `efree` a ZPP-parsed string.** `Z_PARAM_STR(word)` returns a borrowed pointer into a PHP-owned `zend_string`. We only read `ZSTR_VAL(word)`. The legacy PHP 5 code's `efree(word)` was heap corruption.
2. **Never use libc `free()` on a libhunspell-allocated `char**` list.** All list-returning Hunspell calls funnel through the file-static helper `hunspell_strlist_to_array_and_free` in `dictionary.cc`, which always calls `Hunspell_free_list`. If you add another list-returning method, route it through that helper.
3. **Never manually `zend_string_addref` after `zend_update_property*`.** The engine's `write_property` handler (called inside `zend_update_property_str`) already invokes `Z_TRY_ADDREF_P` on the value. A manual addref leaks one reference per call. See commit `6d4c60a` for the original leak fix in `Analysis::__construct`.

### Dictionary object layout

```c
typedef struct {
    Hunhandle *handle;     // libhunspell-owned, freed in free_obj
    zend_object std;       // MUST BE LAST (PHP 7/8 idiom)
} php_hunspell_object;
```

`offset = XtOffsetOf(php_hunspell_object, std)` in the custom handlers. `clone_obj = nullptr` forbids cloning at engine level (the engine throws `Error: Trying to clone an uncloneable object of class Hunspell\Dictionary`).

### Analysis state model

`Hunspell\Analysis` is a `final readonly class` with two typed properties (`string $raw`, `array $fields`). Both `Analysis::__construct` (called from userland) and `Dictionary::analyze` (called from C with direct property writes via `zend_update_property_*`) populate those properties. The two paths must produce byte-identical state — verified by `tests/041-analysis-userland.phpt`.

### Parser semantics — public contract

`hunspell_parse_analysis_line(const char *raw, size_t len, zval *out)`:

- Tokenizes on whitespace.
- For each token, splits on the first `:`. Substring before is the key; after is the value.
- Repeated keys append to a list (`array<string, list<string>>`).
- Tokens without `:` are silently skipped.
- Empty input → empty associative array.

Downstream modules (planned `MagyarIspell\` in 0.2) build on these semantics — do not change them without bumping the major version.

## libhunspell behavior notes (non-obvious, code-relevant)

These are observed quirks of libhunspell 1.7.x. They drive non-obvious code decisions:

1. **`Hunspell_create` does NOT return NULL on missing/invalid dictionary files.** It writes diagnostics to stderr and returns a non-null but broken handle. The constructor in `dictionary.cc` pre-checks both `affPath` and `dicPath` with `access(path, R_OK)` before calling `Hunspell_create`. Same pattern applies to `addDictionary` (`Hunspell_add_dic` also silently accepts missing files).

2. **`Hunspell_suggest` returns the input word for correctly-spelled words.** `suggest()` therefore guards with `Hunspell_spell` first and short-circuits with an empty array if the word is already correct. This matches conventional libhunspell usage — upstream documents `Hunspell_suggest` as intended for misspelled words only.

3. **`Hunspell_get_version` and `Hunspell_get_wordchars_*` are declared in `hunspell.h` but NOT exported** from `libhunspell-1.7.dylib` on Homebrew macOS (verified with `nm -gU`). The binding reaches into the C++ `Hunspell` class via `<hunspell/hunspell.hxx>` for these two methods. The naive `reinterpret_cast<Hunspell *>(Hunhandle*)` would segfault — `Hunspell_create` returns a `HunspellImpl*`, not a `Hunspell*`. The working pattern in `getVersion`/`getWordChars` is:

   ```cpp
   struct { void *m_Impl; } facade{ obj->handle };
   Hunspell *cpp = reinterpret_cast<Hunspell *>(&facade);
   const std::string &s = cpp->get_wordchars_cpp();  // or get_version_cpp()
   ```

   This works because `class Hunspell` has no virtual methods and only one data member (`HunspellImpl* m_Impl` at offset 0). A vtable or extra member added by future libhunspell would silently break this — guard with `static_assert(sizeof(Hunspell) == sizeof(void *))` if extending this pattern.

## Tests

`tests/*.phpt` — 17 tests at 0.1 completion. Naming convention:

- `001-load.phpt` — constructor success / load-failure / key-rejection
- `002-exceptions-registered.phpt` — exception class registration sanity
- `003-dictionary-class.phpt` — Dictionary class reflection sanity
- `010-spell.phpt`, `020-suggest.phpt`, `030-analyze-raw.phpt`, `040-analyze.phpt`, `041-analysis-userland.phpt`, `050-stem.phpt`, `060-generate-string.phpt`, `061-generate-array.phpt`, `062-generate-typeerror.phpt`, `070-add-affix.phpt`, `080-add-dic.phpt`, `090-meta.phpt`, `100-clone-forbidden.phpt`, `101-key-rejected.phpt`

The minimal test dictionary lives in `tests/data/test.aff` + `tests/data/test.dic` (5 words, including `world/A` for SFX affix testing and `szótár` for UTF-8 + morph-field testing). The extra dictionary for `addDictionary` testing is `tests/data/extra.dic`.

## CI

`.github/workflows/ci.yml` runs five jobs on push and pull request to `main`:

- `linux-pie` × PHP {8.2, 8.3, 8.4, 8.5}: `pie install ./` + phpt
- `linux-phpize`: classic phpize build + phpt
- `linux-zts`: ZTS build + phpt (concurrent use of a single Dictionary across threads is not contracted)
- `macos`: macOS + brew + PIE
- `asan`: AddressSanitizer build to catch the class of memory bug the legacy code had

If you add a list-returning method, the ASAN job must stay green.

## Roadmap

- 0.2 — `MagyarIspell\Analyzer` (`partition`, `syllabify`, `rawTag`, `hyphenate`). Separate brainstorming session pending.
- 0.3 — `Hyphen\Dictionary` (libhyphen wrapper). Separate brainstorming session pending.
- 0.4+ — `MyThes`, `DictionaryInfo`, `PersonalDictionary`, `TextChecker`. Low priority.

## Out of scope for 0.1 (do not add without an explicit task)

- Hungarian-specific helpers on `Analysis` (`stems()`, `partOfSpeech()`, `hyphenationHints()`). These belong in `MagyarIspell\` in 0.2.
- Procedural function layer (`hunspell_open`, etc.). Modern OOP only.
- Encrypted-dictionary key support (constructor accepts `?string $key` but throws `InvalidArgumentException` if non-null).
- `Hunspell_spell_with_info` detailed-result bits.
- Bundled dictionaries — distribution is the user's / packager's responsibility.

## Design and plan documents

- Design spec: `docs/superpowers/specs/2026-05-20-php-hunspell-0.1-design.md`
- Implementation plan (executed): `docs/superpowers/plans/2026-05-20-php-hunspell-0.1.md`

The design spec is the authoritative source for the API contract. Where the implementation diverged from the spec's original assumptions (libhunspell behavior, refcount handling), the divergence is captured in this CLAUDE.md and in git history. The plan reflects the as-executed sequence of TDD tasks.
