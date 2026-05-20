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
