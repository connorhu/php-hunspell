#ifndef PHP_HUNSPELL_H
#define PHP_HUNSPELL_H

extern "C" {
#include "php.h"
}

#define PHP_HUNSPELL_VERSION "0.1.0"

extern zend_module_entry hunspell_module_entry;
#define phpext_hunspell_ptr &hunspell_module_entry

extern zend_class_entry *hunspell_exception_ce;             /* interface */
extern zend_class_entry *hunspell_dictionary_load_exception_ce;

void hunspell_register_exception_classes(void);

#endif /* PHP_HUNSPELL_H */
