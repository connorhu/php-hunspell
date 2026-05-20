#include "php_hunspell.h"

extern "C" {
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "ext/spl/spl_exceptions.h"
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
        zend_register_internal_class_ex(&ce_dle, spl_ce_RuntimeException);
    zend_class_implements(hunspell_dictionary_load_exception_ce, 1, hunspell_exception_ce);
    hunspell_dictionary_load_exception_ce->ce_flags |= ZEND_ACC_FINAL;
}
