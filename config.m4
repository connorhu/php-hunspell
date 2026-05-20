dnl config.m4 for the hunspell PHP extension

PHP_ARG_ENABLE([hunspell],
  [whether to enable hunspell support],
  [AS_HELP_STRING([--enable-hunspell], [Enable hunspell support])])

if test "$PHP_HUNSPELL" != "no"; then
  PHP_REQUIRE_CXX()
  PKG_CHECK_MODULES([HUNSPELL], [hunspell >= 1.6.0])
  AC_DEFINE_UNQUOTED([HUNSPELL_LIB_VERSION], ["$(pkg-config --modversion hunspell)"],
    [Detected libhunspell version])
  PHP_EVAL_INCLINE($HUNSPELL_CFLAGS)
  dnl hunspell.pc places /hunspell at the end of the include path, which only
  dnl works for `#include <hunspell.h>`. We use `#include <hunspell/hunspell.h>`
  dnl everywhere, so also add the parent includedir explicitly. Linux finds it
  dnl via the implicit /usr/include search path; macOS Homebrew (especially
  dnl arm64 /opt/homebrew) does not search there by default.
  HUNSPELL_INCLUDEDIR=$(pkg-config --variable=includedir hunspell 2>/dev/null)
  if test -n "$HUNSPELL_INCLUDEDIR"; then
    PHP_ADD_INCLUDE([$HUNSPELL_INCLUDEDIR])
  fi
  PHP_EVAL_LIBLINE($HUNSPELL_LIBS, HUNSPELL_SHARED_LIBADD)
  PHP_SUBST(HUNSPELL_SHARED_LIBADD)
  PHP_NEW_EXTENSION([hunspell],
    [hunspell.cc dictionary.cc analysis.cc exceptions.cc],
    [$ext_shared],,
    [-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -std=c++17])
fi
