#include "php_hunspell.h"

#include <cctype>
#include <cstring>

extern "C" {
#include "Zend/zend_object_handlers.h"
#include "Zend/zend_inheritance.h"
}

zend_class_entry *hunspell_analysis_ce = nullptr;

void hunspell_parse_analysis_line(const char *raw, size_t len, zval *out_fields) {
    array_init(out_fields);
    const char *p = raw;
    const char *end = raw + len;

    while (p < end) {
        while (p < end && std::isspace(static_cast<unsigned char>(*p))) p++;
        if (p >= end) break;

        const char *tok_start = p;
        while (p < end && !std::isspace(static_cast<unsigned char>(*p))) p++;
        const char *tok_end = p;

        const char *colon = static_cast<const char *>(
            std::memchr(tok_start, ':', tok_end - tok_start));
        if (colon == nullptr || colon == tok_start) continue;  /* skip malformed */

        size_t key_len = colon - tok_start;
        size_t val_len = tok_end - (colon + 1);

        zval *bucket = zend_hash_str_find(Z_ARRVAL_P(out_fields), tok_start, key_len);
        if (bucket == nullptr) {
            zval new_list;
            array_init(&new_list);
            bucket = zend_hash_str_add_new(Z_ARRVAL_P(out_fields), tok_start, key_len, &new_list);
        }
        add_next_index_stringl(bucket, colon + 1, val_len);
    }
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_analysis_construct, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, raw, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Analysis, __construct) {
    zend_string *raw;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(raw)
    ZEND_PARSE_PARAMETERS_END();

    zend_update_property_str(hunspell_analysis_ce, Z_OBJ_P(ZEND_THIS),
        "raw", sizeof("raw") - 1, raw);
    zend_string_addref(raw);

    zval fields;
    hunspell_parse_analysis_line(ZSTR_VAL(raw), ZSTR_LEN(raw), &fields);
    zend_update_property(hunspell_analysis_ce, Z_OBJ_P(ZEND_THIS),
        "fields", sizeof("fields") - 1, &fields);
    zval_ptr_dtor(&fields);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_analysis_getRaw, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Analysis, getRaw) {
    ZEND_PARSE_PARAMETERS_NONE();
    zval *raw = zend_read_property(hunspell_analysis_ce, Z_OBJ_P(ZEND_THIS),
        "raw", sizeof("raw") - 1, /*silent*/ 0, /*rv*/ nullptr);
    RETURN_COPY(raw);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_analysis_getFields, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Analysis, getFields) {
    ZEND_PARSE_PARAMETERS_NONE();
    zval *fields = zend_read_property(hunspell_analysis_ce, Z_OBJ_P(ZEND_THIS),
        "fields", sizeof("fields") - 1, 0, nullptr);
    RETURN_COPY(fields);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_analysis_get, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Analysis, get) {
    zend_string *key;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(key)
    ZEND_PARSE_PARAMETERS_END();

    zval *fields = zend_read_property(hunspell_analysis_ce, Z_OBJ_P(ZEND_THIS),
        "fields", sizeof("fields") - 1, 0, nullptr);
    if (Z_TYPE_P(fields) != IS_ARRAY) { RETURN_EMPTY_ARRAY(); }

    zval *bucket = zend_hash_find(Z_ARRVAL_P(fields), key);
    if (bucket == nullptr) { RETURN_EMPTY_ARRAY(); }
    RETURN_COPY(bucket);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_analysis_has, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(Hunspell_Analysis, has) {
    zend_string *key;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(key)
    ZEND_PARSE_PARAMETERS_END();

    zval *fields = zend_read_property(hunspell_analysis_ce, Z_OBJ_P(ZEND_THIS),
        "fields", sizeof("fields") - 1, 0, nullptr);
    if (Z_TYPE_P(fields) != IS_ARRAY) { RETURN_FALSE; }
    RETURN_BOOL(zend_hash_find(Z_ARRVAL_P(fields), key) != nullptr);
}

static const zend_function_entry hunspell_analysis_methods[] = {
    PHP_ME(Hunspell_Analysis, __construct, arginfo_analysis_construct,
           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(Hunspell_Analysis, getRaw,    arginfo_analysis_getRaw,    ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Analysis, getFields, arginfo_analysis_getFields, ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Analysis, get,       arginfo_analysis_get,       ZEND_ACC_PUBLIC)
    PHP_ME(Hunspell_Analysis, has,       arginfo_analysis_has,       ZEND_ACC_PUBLIC)
    PHP_FE_END
};

void hunspell_register_analysis_class(void) {
    zend_class_entry ce;
    INIT_NS_CLASS_ENTRY(ce, "Hunspell", "Analysis", hunspell_analysis_methods);
    hunspell_analysis_ce = zend_register_internal_class_with_flags(
        &ce, nullptr, ZEND_ACC_FINAL | ZEND_ACC_READONLY_CLASS | ZEND_ACC_NO_DYNAMIC_PROPERTIES);

    zval default_undef;
    ZVAL_UNDEF(&default_undef);

    zend_string *prop_raw = zend_string_init_interned("raw", sizeof("raw") - 1, 1);
    zend_declare_typed_property(hunspell_analysis_ce,
        prop_raw, &default_undef, ZEND_ACC_PUBLIC | ZEND_ACC_READONLY, nullptr,
        (zend_type) ZEND_TYPE_INIT_CODE(IS_STRING, 0, 0));
    zend_string_release(prop_raw);

    zend_string *prop_fields = zend_string_init_interned("fields", sizeof("fields") - 1, 1);
    zend_declare_typed_property(hunspell_analysis_ce,
        prop_fields, &default_undef, ZEND_ACC_PUBLIC | ZEND_ACC_READONLY, nullptr,
        (zend_type) ZEND_TYPE_INIT_CODE(IS_ARRAY, 0, 0));
    zend_string_release(prop_fields);
}
