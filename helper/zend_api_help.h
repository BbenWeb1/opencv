#ifndef ZEND_API_HELP_H
#define ZEND_API_HELP_H

#define Z_PARAM_OBJECT_OF_CLASS_REF(dest, ce)                  \
    Z_PARAM_PROLOGUE(0, 0);                                    \
    zval *_val = Z_ISREF_P(_arg) ? Z_REFVAL_P(_arg) : _arg;    \
    if (Z_TYPE_P(_val) != IS_OBJECT || !instanceof_function(Z_OBJCE_P(_val), (ce))) { \
        zend_argument_type_error(_i + 1, "must be object of type %s", ZSTR_VAL((ce)->name)); \
        _error_code = ZPP_ERROR_WRONG_ARG;                    \
        break;                                                \
    }                                                          \
    dest = _val;

#define Z_PARAM_OBJECT_OF_CLASSES(dest, ce1, ce2, ce3)            \
    Z_PARAM_PROLOGUE(0, 0);                                       \
    if (Z_TYPE_P(_arg) != IS_OBJECT ||                            \
        (!instanceof_function(Z_OBJCE_P(_arg), (ce1)) &&          \
         (!(ce2) || !instanceof_function(Z_OBJCE_P(_arg), (ce2))) && \
         (!(ce3) || !instanceof_function(Z_OBJCE_P(_arg), (ce3))))) { \
                                                                    \
        zend_argument_type_error(                                 \
            _i + 1,                                               \
            "must be object of type %s",                          \
            ZSTR_VAL((ce1)->name));                               \
        _error_code = ZPP_ERROR_WRONG_ARG;                        \
        break;                                                    \
    }                                                             \
    dest = _arg;


#endif //ZEND_API_HELP_H