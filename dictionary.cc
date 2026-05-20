#include "php_hunspell.h"

extern "C" {
#include "zend_exceptions.h"
#include "ext/spl/spl_exceptions.h"
}

#include <unistd.h>
#include <vector>

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

static void hunspell_strlist_to_array_and_free(
        Hunhandle *handle, char **slst, int n, zval *return_value) {
    array_init_size(return_value, n);
    for (int i = 0; i < n; i++) {
        add_next_index_string(return_value, slst[i]);
    }
    Hunspell_free_list(handle, &slst, n);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_spell, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_suggest, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_analyzeRaw, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_analyze, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_stem, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dictionary_generate, 0, 2, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, word, IS_STRING, 0)
    ZEND_ARG_TYPE_MASK(0, model, MAY_BE_STRING | MAY_BE_ARRAY, nullptr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_dictionary_construct, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, affPath, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, dicPath, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, key, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Dictionary, __construct) {
    zend_string *aff_path, *dic_path;
    zend_string *key = nullptr;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(aff_path)
        Z_PARAM_STR(dic_path)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(key)
    ZEND_PARSE_PARAMETERS_END();

    if (key != nullptr) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Encrypted dictionary keys are not supported in this release", 0);
        return;
    }

    /* Pre-check that both files are readable before calling Hunspell_create,
     * because libhunspell 1.7.x never returns NULL — it prints errors to
     * stderr and returns a broken-but-non-null handle on failure. */
    if (access(ZSTR_VAL(aff_path), R_OK) != 0 ||
        access(ZSTR_VAL(dic_path), R_OK) != 0) {
        zend_throw_exception_ex(hunspell_dictionary_load_exception_ce, 0,
            "Failed to load Hunspell dictionary (aff='%s', dic='%s')",
            ZSTR_VAL(aff_path), ZSTR_VAL(dic_path));
        return;
    }

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    obj->handle = Hunspell_create(ZSTR_VAL(aff_path), ZSTR_VAL(dic_path));
    if (obj->handle == nullptr) {
        zend_throw_exception_ex(hunspell_dictionary_load_exception_ce, 0,
            "Failed to load Hunspell dictionary (aff='%s', dic='%s')",
            ZSTR_VAL(aff_path), ZSTR_VAL(dic_path));
        return;
    }
}

PHP_METHOD(Hunspell_Dictionary, spell) {
    zend_string *word;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(word)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    int ok = Hunspell_spell(obj->handle, ZSTR_VAL(word));
    RETURN_BOOL(ok != 0);
}

PHP_METHOD(Hunspell_Dictionary, suggest) {
    zend_string *word;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(word)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    if (Hunspell_spell(obj->handle, ZSTR_VAL(word))) {
        array_init(return_value);
        return;
    }
    char **slst = nullptr;
    int n = Hunspell_suggest(obj->handle, &slst, ZSTR_VAL(word));
    hunspell_strlist_to_array_and_free(obj->handle, slst, n, return_value);
}

PHP_METHOD(Hunspell_Dictionary, analyzeRaw) {
    zend_string *word;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(word)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    char **slst = nullptr;
    int n = Hunspell_analyze(obj->handle, &slst, ZSTR_VAL(word));
    hunspell_strlist_to_array_and_free(obj->handle, slst, n, return_value);
}

PHP_METHOD(Hunspell_Dictionary, analyze) {
    zend_string *word;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(word)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    char **slst = nullptr;
    int n = Hunspell_analyze(obj->handle, &slst, ZSTR_VAL(word));

    array_init_size(return_value, n);
    for (int i = 0; i < n; i++) {
        zval analysis_zv;
        object_init_ex(&analysis_zv, hunspell_analysis_ce);
        zend_object *aobj = Z_OBJ(analysis_zv);

        size_t raw_len = strlen(slst[i]);

        zend_string *raw_zs = zend_string_init(slst[i], raw_len, 0);
        zend_update_property_str(hunspell_analysis_ce, aobj,
            "raw", sizeof("raw") - 1, raw_zs);
        zend_string_release(raw_zs);

        zval fields;
        hunspell_parse_analysis_line(slst[i], raw_len, &fields);
        zend_update_property(hunspell_analysis_ce, aobj,
            "fields", sizeof("fields") - 1, &fields);
        zval_ptr_dtor(&fields);

        add_next_index_zval(return_value, &analysis_zv);
    }
    Hunspell_free_list(obj->handle, &slst, n);
}

PHP_METHOD(Hunspell_Dictionary, stem) {
    zend_string *word;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(word)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    char **slst = nullptr;
    int n = Hunspell_stem(obj->handle, &slst, ZSTR_VAL(word));
    hunspell_strlist_to_array_and_free(obj->handle, slst, n, return_value);
}

PHP_METHOD(Hunspell_Dictionary, generate) {
    zend_string *word;
    zval *model;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(word)
        Z_PARAM_ZVAL(model)
    ZEND_PARSE_PARAMETERS_END();

    php_hunspell_object *obj = php_hunspell_from_obj(Z_OBJ_P(ZEND_THIS));
    char **slst = nullptr;
    int n = 0;

    if (Z_TYPE_P(model) == IS_STRING) {
        n = Hunspell_generate(obj->handle, &slst, ZSTR_VAL(word), Z_STRVAL_P(model));
    } else if (Z_TYPE_P(model) == IS_ARRAY) {
        HashTable *ht = Z_ARRVAL_P(model);
        uint32_t count = zend_hash_num_elements(ht);
        std::vector<const char *> desc(count);
        uint32_t i = 0;
        zval *item;
        ZEND_HASH_FOREACH_VAL(ht, item) {
            if (Z_TYPE_P(item) != IS_STRING) {
                zend_argument_type_error(2,
                    "item #%u must be of type string, %s given",
                    i, zend_zval_value_name(item));
                return;
            }
            desc[i++] = Z_STRVAL_P(item);
        } ZEND_HASH_FOREACH_END();
        n = Hunspell_generate2(obj->handle, &slst, ZSTR_VAL(word),
            const_cast<char **>(desc.data()), static_cast<int>(count));
    } else {
        zend_argument_type_error(2, "must be of type string|array, %s given",
            zend_zval_value_name(model));
        return;
    }

    hunspell_strlist_to_array_and_free(obj->handle, slst, n, return_value);
}

static const zend_function_entry hunspell_dictionary_methods[] = {
    PHP_ME(Hunspell_Dictionary, __construct, arginfo_dictionary_construct,
           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(Hunspell_Dictionary, spell, arginfo_dictionary_spell, ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Dictionary, suggest, arginfo_dictionary_suggest, ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Dictionary, analyzeRaw, arginfo_dictionary_analyzeRaw, ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Dictionary, analyze, arginfo_dictionary_analyze, ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Dictionary, stem, arginfo_dictionary_stem, ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Dictionary, generate, arginfo_dictionary_generate, ZEND_ACC_PUBLIC)
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
