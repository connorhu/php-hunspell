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
    hunspell_dictionary_handlers.clone_obj = nullptr;
}
