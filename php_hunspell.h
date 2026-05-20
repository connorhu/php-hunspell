#ifndef PHP_HUNSPELL_H
#define PHP_HUNSPELL_H

extern "C" {
#include "php.h"
}

#define PHP_HUNSPELL_VERSION "0.1.0"

extern zend_module_entry hunspell_module_entry;
#define phpext_hunspell_ptr &hunspell_module_entry

#endif /* PHP_HUNSPELL_H */
