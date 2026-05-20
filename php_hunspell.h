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

extern "C" {
#include <hunspell/hunspell.h>
}

typedef struct {
    Hunhandle *handle;
    zend_object std;        /* must be last */
} php_hunspell_object;

extern zend_class_entry *hunspell_dictionary_ce;

inline php_hunspell_object *php_hunspell_from_obj(zend_object *obj) {
    return reinterpret_cast<php_hunspell_object *>(
        reinterpret_cast<char *>(obj) - XtOffsetOf(php_hunspell_object, std));
}

void hunspell_register_dictionary_class(void);

#endif /* PHP_HUNSPELL_H */
