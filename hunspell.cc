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
    nullptr,
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
