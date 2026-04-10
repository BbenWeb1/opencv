/* ============================================================
   OpenCV PHP Overload System - Score Based Strict
   Messenger v2 + Vector Support (bidirectional) - FINAL
   PHP 8.1 safe | Zero leaks
   ============================================================ */
#ifndef OCV_OVERLOAD_H
#define OCV_OVERLOAD_H

#include "ext/standard/php_smart_str.h"

#define OCV_MAX_ARGS 16

/* ============================================================
   Types
   ============================================================ */

typedef enum {
    A_NONE        = 0,
    A_MAT         = 1,
    A_SCALAR      = 2,
    A_POINT       = 3,
    A_SIZE        = 4,
    A_RECT        = 5,
    A_ROTATEDRECT = 6,
    A_RANGE       = 7,
    A_INT         = 8,
    A_DOUBLE      = 9,
    A_BOOL        = 10,
    A_STRING      = 11,
    A_ARRAY       = 12,
    A_LONG        = 13,

    A_VEC_POINT   = 20,
    A_VEC_POINT2F = 21,
    A_VEC_MAT     = 22,
    A_VEC_RECT    = 23,
    A_VEC_SCALAR  = 24,
    A_VEC_VEC4I   = 25,
    A_VEC_INT     = 26,
    A_VEC_DOUBLE  = 27,
    A_VEC_FLOAT   = 28,
    A_VEC_STRING  = 29,
} arg_kind_t;

/* ============================================================
   ocv_vec_t
   ============================================================ */
typedef struct {
    void    *data;
    uint8_t  vec_kind;
} ocv_vec_t;

static inline void ocv_vec_free(ocv_vec_t *w)
{
    if (!w || !w->data) return;
    switch (w->vec_kind) {
        case A_VEC_POINT:   delete reinterpret_cast<std::vector<cv::Point>*>  (w->data); break;
        case A_VEC_POINT2F: delete reinterpret_cast<std::vector<cv::Point2f>*>(w->data); break;
        case A_VEC_MAT:     delete reinterpret_cast<std::vector<cv::Mat>*>    (w->data); break;
        case A_VEC_RECT:    delete reinterpret_cast<std::vector<cv::Rect>*>   (w->data); break;
        case A_VEC_SCALAR:  delete reinterpret_cast<std::vector<cv::Scalar>*> (w->data); break;
        case A_VEC_VEC4I:   delete reinterpret_cast<std::vector<cv::Vec4i>*>  (w->data); break;
        case A_VEC_INT:     delete reinterpret_cast<std::vector<int>*>        (w->data); break;
        case A_VEC_DOUBLE:  delete reinterpret_cast<std::vector<double>*>     (w->data); break;
        case A_VEC_FLOAT:   delete reinterpret_cast<std::vector<float>*>      (w->data); break;
        case A_VEC_STRING:  delete reinterpret_cast<std::vector<std::string>*>(w->data); break;
        default: break;
    }
    efree(w);
}

/* ============================================================
   Value + Slot + Result + Hint
   ============================================================ */

typedef union {
    void      *ptr;
    zend_long  i_val;
    double     d_val;
} ocv_value_t;

typedef struct {
    ocv_value_t  val;
    zval        *zv;
    uint32_t     kind;
    unsigned int present      : 1;
    unsigned int is_converted : 1;
    unsigned int need_release : 1;
} ocv_arg_slot_t;

typedef struct {
    ocv_arg_slot_t slots[OCV_MAX_ARGS];
    int count;
    int score;
    int max_score;
} ocv_arg_result_t;

typedef struct {
    const arg_kind_t *kinds;
    int        count;
    int        score;
    int        failed_at;
    arg_kind_t expected;
    uint8_t    got_ztype;
} ocv_hint_t;

/* ============================================================
   Helpers
   ============================================================ */

static inline const char *ocv_current_function_name(void)
{
    zend_execute_data *ex = EG(current_execute_data);
    return (ex && ex->func && ex->func->common.function_name)
        ? ZSTR_VAL(ex->func->common.function_name)
        : "unknown_function";
}

static inline int ocv_sig_count(const arg_kind_t *kinds)
{
    int n = 0;
    while (n < OCV_MAX_ARGS && kinds[n] != A_NONE) n++;
    return n;
}

static inline const char *ocv_kind_name(arg_kind_t k)
{
    switch (k) {
        case A_MAT:         return "Mat";
        case A_SCALAR:      return "Scalar";
        case A_POINT:       return "Point";
        case A_SIZE:        return "Size";
        case A_RECT:        return "Rect";
        case A_ROTATEDRECT: return "RotatedRect";
        case A_RANGE:       return "Range";
        case A_INT:         return "int";
        case A_LONG:        return "long";
        case A_DOUBLE:      return "double";
        case A_BOOL:        return "bool";
        case A_STRING:      return "string";
        case A_ARRAY:       return "array";
        case A_VEC_POINT:   return "Point[]";
        case A_VEC_POINT2F: return "Point2f[]";
        case A_VEC_MAT:     return "Mat[]";
        case A_VEC_RECT:    return "Rect[]";
        case A_VEC_SCALAR:  return "Scalar[]";
        case A_VEC_VEC4I:   return "Vec4i[]";
        case A_VEC_INT:     return "int[]";
        case A_VEC_DOUBLE:  return "double[]";
        case A_VEC_FLOAT:   return "float[]";
        case A_VEC_STRING:  return "string[]";
        default:            return "?";
    }
}

#define _OCV_VEC_STORE(slot_ptr, vec_ptr, kind_val)              \
    do {                                                          \
        ocv_vec_t *_w = (ocv_vec_t*)emalloc(sizeof(ocv_vec_t)); \
        _w->data           = (vec_ptr);                           \
        _w->vec_kind       = (uint8_t)(kind_val);                 \
        (slot_ptr)->val.ptr     = _w;                             \
        (slot_ptr)->need_release = 1;                             \
    } while (0)

#define _OCV_FOREACH_ARRAY(vec_ptr, ht_ptr, body)       \
    do {                                                 \
        zval *item;                                      \
        ZEND_HASH_FOREACH_VAL((ht_ptr), item) {          \
            ZVAL_DEREF(item);                            \
            body                                         \
        } ZEND_HASH_FOREACH_END();                       \
    } while (0)

/* ============================================================
   Reset
   ============================================================ */

static inline void ocv_result_reset(ocv_arg_result_t *res)
{
    if (!res) return;
    for (int i = 0; i < res->count; i++) {
        if (!res->slots[i].need_release || !res->slots[i].val.ptr) continue;
        uint32_t k = res->slots[i].kind;
        if (k >= (uint32_t)A_VEC_POINT && k <= (uint32_t)A_VEC_STRING) {
            ocv_vec_free((ocv_vec_t*)res->slots[i].val.ptr);
            continue;
        }
        switch (k) {
            case A_STRING:
                zend_string_release((zend_string*)res->slots[i].val.ptr); break;
            case A_POINT:
            case A_SIZE:
            case A_RECT:
                efree(res->slots[i].val.ptr); break;
        }
    }
    memset(res, 0, sizeof(*res));
}

/* ============================================================
   kind_extract  (بدون تغيير)
   ============================================================ */

static inline int kind_extract(zval *zv, arg_kind_t kind,
                                ocv_arg_slot_t *slot, int *score)
{
    slot->zv           = zv;
    slot->kind         = kind;
    slot->present      = (zv && Z_TYPE_P(zv) != IS_NULL);
    slot->need_release = 0;
    slot->is_converted = 0;
    slot->val.ptr      = NULL;

    if (!slot->present) return 1;

    switch (kind) {
    case A_MAT:
        if (Z_TYPE_P(zv) != IS_OBJECT ||
            !instanceof_function(Z_OBJCE_P(zv), opencv_mat_ce)) return 0;
        slot->val.ptr = Z_PHP_MAT_OBJ_P(zv)->mat_ptr;
        *score += 3; return 1;

    case A_SCALAR:
        if (Z_TYPE_P(zv) != IS_OBJECT ||
            !instanceof_function(Z_OBJCE_P(zv), opencv_scalar_ce)) return 0;
        slot->val.ptr = Z_PHP_SCALAR_OBJ_P(zv)->ptr;
        *score += 3; return 1;

    case A_POINT: {
        if (Z_TYPE_P(zv) != IS_OBJECT ||
            !instanceof_function(Z_OBJCE_P(zv), opencv_point_ce)) return 0;
        opencv_point_object *obj = Z_PHP_POINT_OBJ_P(zv);
        cv::Point pt;
        if      (obj->type == POINT_TYPE_2I && obj->ptr.pi)
            { pt = *obj->ptr.pi; *score += 3; }
        else if (obj->type == POINT_TYPE_2D && obj->ptr.pd) {
            pt = cv::Point(cvRound(obj->ptr.pd->x), cvRound(obj->ptr.pd->y));
            slot->is_converted = 1; *score += 1;
        } else return 0;
        slot->val.ptr = emalloc(sizeof(cv::Point));
        *(cv::Point*)slot->val.ptr = pt;
        slot->need_release = 1;
        return 1;
    }

    case A_SIZE: {
        if (Z_TYPE_P(zv) != IS_OBJECT ||
            !instanceof_function(Z_OBJCE_P(zv), opencv_size_ce)) return 0;
        opencv_size_object *obj = Z_PHP_SIZE_OBJ_P(zv);
        cv::Size sz;
        if      (obj->type == SIZE_TYPE_2I && obj->ptr.si)
            { sz = *obj->ptr.si; *score += 3; }
        else if (obj->type == SIZE_TYPE_2D && obj->ptr.sd) {
            sz = cv::Size(cvRound(obj->ptr.sd->width), cvRound(obj->ptr.sd->height));
            slot->is_converted = 1; *score += 1;
        } else return 0;
        slot->val.ptr = emalloc(sizeof(cv::Size));
        *(cv::Size*)slot->val.ptr = sz;
        slot->need_release = 1;
        return 1;
    }

    case A_RECT: {
        if (Z_TYPE_P(zv) != IS_OBJECT ||
            !instanceof_function(Z_OBJCE_P(zv), opencv_rect_ce)) return 0;
        opencv_rect_object *obj = Z_PHP_RECT_OBJ_P(zv);
        if (obj->type == RECT_TYPE_2I && obj->ptr.ri) {
            slot->val.ptr = obj->ptr.ri; *score += 3; return 1;
        }
        if (obj->type == RECT_TYPE_2D && obj->ptr.rd) {
            cv::Rect *r = (cv::Rect*)emalloc(sizeof(cv::Rect));
            *r = cv::Rect(cvRound(obj->ptr.rd->x),    cvRound(obj->ptr.rd->y),
                          cvRound(obj->ptr.rd->width), cvRound(obj->ptr.rd->height));
            slot->val.ptr = r;
            slot->need_release = slot->is_converted = 1;
            *score += 1; return 1;
        }
        return 0;
    }

    case A_ROTATEDRECT:
    case A_RANGE:
        if (Z_TYPE_P(zv) != IS_OBJECT) return 0;
        slot->val.ptr = (kind == A_ROTATEDRECT)
            ? (void*)Z_PHP_ROTATEDRECT_OBJ_P(zv)->ptr
            : (void*)Z_PHP_RANGE_OBJ_P(zv)->ptr;
        *score += 3; return 1;

    case A_INT:
    case A_LONG:
        if (Z_TYPE_P(zv) == IS_LONG)
            { slot->val.i_val = Z_LVAL_P(zv); *score += 3; return 1; }
        if (Z_TYPE_P(zv) == IS_DOUBLE)
            { slot->val.i_val = (zend_long)Z_DVAL_P(zv);
              slot->is_converted = 1; *score += 1; return 1; }
        return 0;

    case A_DOUBLE:
        if (Z_TYPE_P(zv) == IS_DOUBLE)
            { slot->val.d_val = Z_DVAL_P(zv); *score += 3; return 1; }
        if (Z_TYPE_P(zv) == IS_LONG)
            { slot->val.d_val = (double)Z_LVAL_P(zv);
              slot->is_converted = 1; *score += 1; return 1; }
        return 0;

    case A_BOOL:
        if (Z_TYPE_P(zv) == IS_TRUE || Z_TYPE_P(zv) == IS_FALSE) {
            slot->val.i_val = (Z_TYPE_P(zv) == IS_TRUE);
            *score += 3; return 1;
        }
        return 0;

    case A_STRING:
        if (Z_TYPE_P(zv) != IS_STRING) return 0;
        zend_string_addref(Z_STR_P(zv));
        slot->val.ptr      = Z_STR_P(zv);
        slot->need_release = 1;
        *score += 3; return 1;

    case A_ARRAY:
        if (Z_TYPE_P(zv) != IS_ARRAY) return 0;
        slot->val.ptr = Z_ARRVAL_P(zv);
        *score += 3; return 1;

    case A_VEC_POINT: {
        auto *vec = new std::vector<cv::Point>();
        if (Z_TYPE_P(zv) == IS_ARRAY) {
            bool converted = false;
            _OCV_FOREACH_ARRAY(vec, Z_ARRVAL_P(zv), {
                if (Z_TYPE_P(item) != IS_OBJECT ||
                    !instanceof_function(Z_OBJCE_P(item), opencv_point_ce))
                    { delete vec; return 0; }
                opencv_point_object *p = Z_PHP_POINT_OBJ_P(item);
                if (p->type == POINT_TYPE_2I && p->ptr.pi)
                    vec->push_back(*p->ptr.pi);
                else if (p->type == POINT_TYPE_2D && p->ptr.pd) {
                    vec->push_back(cv::Point(cvRound(p->ptr.pd->x), cvRound(p->ptr.pd->y)));
                    converted = true;
                } else { delete vec; return 0; }
            });
            _OCV_VEC_STORE(slot, vec, A_VEC_POINT);
            if (converted) slot->is_converted = 1;
            *score += (converted ? 1 : 2); return 1;
        }
        if (Z_TYPE_P(zv) == IS_OBJECT &&
            instanceof_function(Z_OBJCE_P(zv), opencv_mat_ce)) {
            cv::Mat *m = Z_PHP_MAT_OBJ_P(zv)->mat_ptr;
            if (!m || m->empty()) { delete vec; return 0; }
            if (m->type() == CV_32SC2) {
                vec->reserve((size_t)m->rows);
                for (int r = 0; r < m->rows; r++) {
                    auto v = m->at<cv::Vec2i>(r, 0);
                    vec->emplace_back(v[0], v[1]);
                }
                _OCV_VEC_STORE(slot, vec, A_VEC_POINT);
                *score += 2; return 1;
            }
            if (m->type() == CV_32FC2) {
                vec->reserve((size_t)m->rows);
                for (int r = 0; r < m->rows; r++) {
                    auto v = m->at<cv::Vec2f>(r, 0);
                    vec->emplace_back(cvRound(v[0]), cvRound(v[1]));
                }
                _OCV_VEC_STORE(slot, vec, A_VEC_POINT);
                slot->is_converted = 1; *score += 1; return 1;
            }
        }
        delete vec; return 0;
    }

    case A_VEC_POINT2F: {
        auto *vec = new std::vector<cv::Point2f>();
        if (Z_TYPE_P(zv) == IS_ARRAY) {
            bool converted = false;
            _OCV_FOREACH_ARRAY(vec, Z_ARRVAL_P(zv), {
                if (Z_TYPE_P(item) != IS_OBJECT ||
                    !instanceof_function(Z_OBJCE_P(item), opencv_point_ce))
                    { delete vec; return 0; }
                opencv_point_object *p = Z_PHP_POINT_OBJ_P(item);
                if (p->type == POINT_TYPE_2D && p->ptr.pd)
                    vec->emplace_back((float)p->ptr.pd->x, (float)p->ptr.pd->y);
                else if (p->type == POINT_TYPE_2I && p->ptr.pi) {
                    vec->emplace_back((float)p->ptr.pi->x, (float)p->ptr.pi->y);
                    converted = true;
                } else { delete vec; return 0; }
            });
            _OCV_VEC_STORE(slot, vec, A_VEC_POINT2F);
            if (converted) slot->is_converted = 1;
            *score += (converted ? 1 : 2); return 1;
        }
        if (Z_TYPE_P(zv) == IS_OBJECT &&
            instanceof_function(Z_OBJCE_P(zv), opencv_mat_ce)) {
            cv::Mat *m = Z_PHP_MAT_OBJ_P(zv)->mat_ptr;
            if (!m || m->empty()) { delete vec; return 0; }
            if (m->type() == CV_32FC2) {
                vec->reserve((size_t)m->rows);
                for (int r = 0; r < m->rows; r++) {
                    auto v = m->at<cv::Vec2f>(r, 0);
                    vec->emplace_back(v[0], v[1]);
                }
                _OCV_VEC_STORE(slot, vec, A_VEC_POINT2F);
                *score += 2; return 1;
            }
            if (m->type() == CV_32SC2) {
                vec->reserve((size_t)m->rows);
                for (int r = 0; r < m->rows; r++) {
                    auto v = m->at<cv::Vec2i>(r, 0);
                    vec->emplace_back((float)v[0], (float)v[1]);
                }
                _OCV_VEC_STORE(slot, vec, A_VEC_POINT2F);
                slot->is_converted = 1; *score += 1; return 1;
            }
        }
        delete vec; return 0;
    }

    case A_VEC_MAT: {
        if (Z_TYPE_P(zv) != IS_ARRAY) return 0;
        auto *vec = new std::vector<cv::Mat>();
        _OCV_FOREACH_ARRAY(vec, Z_ARRVAL_P(zv), {
            if (Z_TYPE_P(item) != IS_OBJECT ||
                !instanceof_function(Z_OBJCE_P(item), opencv_mat_ce))
                { delete vec; return 0; }
            vec->push_back(*Z_PHP_MAT_OBJ_P(item)->mat_ptr);
        });
        _OCV_VEC_STORE(slot, vec, A_VEC_MAT);
        *score += 2; return 1;
    }

    case A_VEC_RECT: {
        if (Z_TYPE_P(zv) != IS_ARRAY) return 0;
        auto *vec = new std::vector<cv::Rect>();
        bool converted = false;
        _OCV_FOREACH_ARRAY(vec, Z_ARRVAL_P(zv), {
            if (Z_TYPE_P(item) != IS_OBJECT ||
                !instanceof_function(Z_OBJCE_P(item), opencv_rect_ce))
                { delete vec; return 0; }
            opencv_rect_object *r = Z_PHP_RECT_OBJ_P(item);
            if (r->type == RECT_TYPE_2I && r->ptr.ri)
                vec->push_back(*r->ptr.ri);
            else if (r->type == RECT_TYPE_2D && r->ptr.rd) {
                vec->emplace_back(cvRound(r->ptr.rd->x), cvRound(r->ptr.rd->y),
                                  cvRound(r->ptr.rd->width), cvRound(r->ptr.rd->height));
                converted = true;
            } else { delete vec; return 0; }
        });
        _OCV_VEC_STORE(slot, vec, A_VEC_RECT);
        if (converted) slot->is_converted = 1;
        *score += (converted ? 1 : 2); return 1;
    }

    case A_VEC_SCALAR: {
        if (Z_TYPE_P(zv) != IS_ARRAY) return 0;
        auto *vec = new std::vector<cv::Scalar>();
        _OCV_FOREACH_ARRAY(vec, Z_ARRVAL_P(zv), {
            if (Z_TYPE_P(item) != IS_OBJECT ||
                !instanceof_function(Z_OBJCE_P(item), opencv_scalar_ce))
                { delete vec; return 0; }
            vec->push_back(*Z_PHP_SCALAR_OBJ_P(item)->ptr);
        });
        _OCV_VEC_STORE(slot, vec, A_VEC_SCALAR);
        *score += 2; return 1;
    }

    case A_VEC_VEC4I: {
        auto *vec = new std::vector<cv::Vec4i>();
        if (Z_TYPE_P(zv) == IS_ARRAY) {
            _OCV_FOREACH_ARRAY(vec, Z_ARRVAL_P(zv), {
                if (Z_TYPE_P(item) != IS_ARRAY ||
                    zend_array_count(Z_ARRVAL_P(item)) != 4)
                    { delete vec; return 0; }
                cv::Vec4i v; int idx = 0; zval *elem;
                ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(item), elem) {
                    ZVAL_DEREF(elem);
                    if      (Z_TYPE_P(elem) == IS_LONG)   v[idx++] = (int)Z_LVAL_P(elem);
                    else if (Z_TYPE_P(elem) == IS_DOUBLE)  v[idx++] = (int)Z_DVAL_P(elem);
                    else { delete vec; return 0; }
                } ZEND_HASH_FOREACH_END();
                if (idx != 4) { delete vec; return 0; }
                vec->push_back(v);
            });
            _OCV_VEC_STORE(slot, vec, A_VEC_VEC4I);
            *score += 2; return 1;
        }
        if (Z_TYPE_P(zv) == IS_OBJECT &&
            instanceof_function(Z_OBJCE_P(zv), opencv_mat_ce)) {
            cv::Mat *m = Z_PHP_MAT_OBJ_P(zv)->mat_ptr;
            if (!m || m->empty() || m->type() != CV_32SC4) { delete vec; return 0; }
            vec->reserve((size_t)m->rows);
            for (int r = 0; r < m->rows; r++) vec->push_back(m->at<cv::Vec4i>(r, 0));
            _OCV_VEC_STORE(slot, vec, A_VEC_VEC4I);
            *score += 2; return 1;
        }
        delete vec; return 0;
    }

    case A_VEC_INT: {
        if (Z_TYPE_P(zv) != IS_ARRAY) return 0;
        auto *vec = new std::vector<int>();
        bool converted = false;
        _OCV_FOREACH_ARRAY(vec, Z_ARRVAL_P(zv), {
            if      (Z_TYPE_P(item) == IS_LONG)   vec->push_back((int)Z_LVAL_P(item));
            else if (Z_TYPE_P(item) == IS_DOUBLE) { vec->push_back((int)Z_DVAL_P(item)); converted = true; }
            else { delete vec; return 0; }
        });
        _OCV_VEC_STORE(slot, vec, A_VEC_INT);
        if (converted) slot->is_converted = 1;
        *score += (converted ? 1 : 2); return 1;
    }

    case A_VEC_DOUBLE: {
        if (Z_TYPE_P(zv) != IS_ARRAY) return 0;
        auto *vec = new std::vector<double>();
        bool converted = false;
        _OCV_FOREACH_ARRAY(vec, Z_ARRVAL_P(zv), {
            if      (Z_TYPE_P(item) == IS_DOUBLE) vec->push_back(Z_DVAL_P(item));
            else if (Z_TYPE_P(item) == IS_LONG)   { vec->push_back((double)Z_LVAL_P(item)); converted = true; }
            else { delete vec; return 0; }
        });
        _OCV_VEC_STORE(slot, vec, A_VEC_DOUBLE);
        if (converted) slot->is_converted = 1;
        *score += (converted ? 1 : 2); return 1;
    }

    case A_VEC_FLOAT: {
        if (Z_TYPE_P(zv) != IS_ARRAY) return 0;
        auto *vec = new std::vector<float>();
        bool converted = false;
        _OCV_FOREACH_ARRAY(vec, Z_ARRVAL_P(zv), {
            if      (Z_TYPE_P(item) == IS_DOUBLE) vec->push_back((float)Z_DVAL_P(item));
            else if (Z_TYPE_P(item) == IS_LONG)   { vec->push_back((float)Z_LVAL_P(item)); converted = true; }
            else { delete vec; return 0; }
        });
        _OCV_VEC_STORE(slot, vec, A_VEC_FLOAT);
        if (converted) slot->is_converted = 1;
        *score += (converted ? 1 : 2); return 1;
    }

    case A_VEC_STRING: {
        if (Z_TYPE_P(zv) != IS_ARRAY) return 0;
        auto *vec = new std::vector<std::string>();
        _OCV_FOREACH_ARRAY(vec, Z_ARRVAL_P(zv), {
            if (Z_TYPE_P(item) != IS_STRING) { delete vec; return 0; }
            vec->emplace_back(ZSTR_VAL(Z_STR_P(item)), ZSTR_LEN(Z_STR_P(item)));
        });
        _OCV_VEC_STORE(slot, vec, A_VEC_STRING);
        *score += 2; return 1;
    }

    default: return 0;
    }
}

/* ============================================================
   C++ → PHP
   ============================================================ */

static inline void ocv_vec_point_to_php(zval *zv, const std::vector<cv::Point> &vec)
{
    array_init_size(zv, (uint32_t)vec.size());
    for (const auto &pt : vec) {
        zval obj; object_init_ex(&obj, opencv_point_ce);
        opencv_point_object *p = Z_PHP_POINT_OBJ_P(&obj);
        p->ptr.pi = new cv::Point(pt); p->type = POINT_TYPE_2I;
        add_next_index_zval(zv, &obj);
    }
}

static inline void ocv_vec_point2f_to_php(zval *zv, const std::vector<cv::Point2f> &vec)
{
    array_init_size(zv, (uint32_t)vec.size());
    for (const auto &pt : vec) {
        zval obj; object_init_ex(&obj, opencv_point_ce);
        opencv_point_object *p = Z_PHP_POINT_OBJ_P(&obj);
        p->ptr.pd = new cv::Point2d((double)pt.x, (double)pt.y); p->type = POINT_TYPE_2D;
        add_next_index_zval(zv, &obj);
    }
}

static inline void ocv_vec_mat_to_php(zval *zv, const std::vector<cv::Mat> &vec)
{
    array_init_size(zv, (uint32_t)vec.size());
    for (const auto &m : vec) {
        zval obj; object_init_ex(&obj, opencv_mat_ce);
        Z_PHP_MAT_OBJ_P(&obj)->mat_ptr = new cv::Mat(m);
        add_next_index_zval(zv, &obj);
    }
}

static inline void ocv_vec_rect_to_php(zval *zv, const std::vector<cv::Rect> &vec)
{
    array_init_size(zv, (uint32_t)vec.size());
    for (const auto &r : vec) {
        zval obj; object_init_ex(&obj, opencv_rect_ce);
        opencv_rect_object *ro = Z_PHP_RECT_OBJ_P(&obj);
        ro->ptr.ri = new cv::Rect(r); ro->type = RECT_TYPE_2I;
        add_next_index_zval(zv, &obj);
    }
}

static inline void ocv_vec_scalar_to_php(zval *zv, const std::vector<cv::Scalar> &vec)
{
    array_init_size(zv, (uint32_t)vec.size());
    for (const auto &s : vec) {
        zval obj; object_init_ex(&obj, opencv_scalar_ce);
        Z_PHP_SCALAR_OBJ_P(&obj)->ptr = new cv::Scalar(s);
        add_next_index_zval(zv, &obj);
    }
}

static inline void ocv_vec_vec4i_to_php(zval *zv, const std::vector<cv::Vec4i> &vec)
{
    array_init_size(zv, (uint32_t)vec.size());
    for (const auto &v : vec) {
        zval sub; array_init_size(&sub, 4);
        for (int i = 0; i < 4; i++) add_next_index_long(&sub, (zend_long)v[i]);
        add_next_index_zval(zv, &sub);
    }
}

static inline void ocv_vec_int_to_php(zval *zv, const std::vector<int> &vec)
{
    array_init_size(zv, (uint32_t)vec.size());
    for (int v : vec) add_next_index_long(zv, (zend_long)v);
}

static inline void ocv_vec_double_to_php(zval *zv, const std::vector<double> &vec)
{
    array_init_size(zv, (uint32_t)vec.size());
    for (double v : vec) add_next_index_double(zv, v);
}

static inline void ocv_vec_float_to_php(zval *zv, const std::vector<float> &vec)
{
    array_init_size(zv, (uint32_t)vec.size());
    for (float v : vec) add_next_index_double(zv, (double)v);
}

static inline void ocv_vec_string_to_php(zval *zv, const std::vector<std::string> &vec)
{
    array_init_size(zv, (uint32_t)vec.size());
    for (const auto &s : vec) add_next_index_stringl(zv, s.c_str(), s.size());
}

static inline void _ocv_vec_to_php_dispatch(zval *zv, void *vec_ptr, arg_kind_t vec_kind)
{
    switch (vec_kind) {
        case A_VEC_POINT:   ocv_vec_point_to_php  (zv, *reinterpret_cast<std::vector<cv::Point>*>  (vec_ptr)); break;
        case A_VEC_POINT2F: ocv_vec_point2f_to_php(zv, *reinterpret_cast<std::vector<cv::Point2f>*>(vec_ptr)); break;
        case A_VEC_MAT:     ocv_vec_mat_to_php    (zv, *reinterpret_cast<std::vector<cv::Mat>*>    (vec_ptr)); break;
        case A_VEC_RECT:    ocv_vec_rect_to_php   (zv, *reinterpret_cast<std::vector<cv::Rect>*>   (vec_ptr)); break;
        case A_VEC_SCALAR:  ocv_vec_scalar_to_php (zv, *reinterpret_cast<std::vector<cv::Scalar>*> (vec_ptr)); break;
        case A_VEC_VEC4I:   ocv_vec_vec4i_to_php  (zv, *reinterpret_cast<std::vector<cv::Vec4i>*>  (vec_ptr)); break;
        case A_VEC_INT:     ocv_vec_int_to_php    (zv, *reinterpret_cast<std::vector<int>*>        (vec_ptr)); break;
        case A_VEC_DOUBLE:  ocv_vec_double_to_php (zv, *reinterpret_cast<std::vector<double>*>     (vec_ptr)); break;
        case A_VEC_FLOAT:   ocv_vec_float_to_php  (zv, *reinterpret_cast<std::vector<float>*>      (vec_ptr)); break;
        case A_VEC_STRING:  ocv_vec_string_to_php (zv, *reinterpret_cast<std::vector<std::string>*>(vec_ptr)); break;
        default: array_init(zv); break;
    }
}

/* ============================================================
   Parser  –  منطق النظام القديم + كفاءة النظام الجديد
   ============================================================ */

static inline int ocv_parse_minmax(
    zval *args, int argc,
    ocv_arg_result_t *res,
    int min_args, int max_args,
    const arg_kind_t *kinds, int nkinds,
    int *out_failed_at, arg_kind_t *out_expected, uint8_t *out_ztype)
{
    ocv_result_reset(res);

    *out_failed_at = -1;
    *out_expected  = A_NONE;
    *out_ztype     = IS_NULL;

    /* كفاءة: نرفض مبكراً */
    if (argc < min_args || argc > max_args) return 0;

    for (int i = 0; i < nkinds && kinds[i] != A_NONE; i++) {
        if (i < argc) {
            /*
             * منطق النظام القديم:
             * max_score يُضاف فقط للـ slots الحاضرة
             * → optional args لا تؤثر على score
             */
            res->max_score += 3;

            if (!kind_extract(&args[i], kinds[i], &res->slots[i], &res->score)) {
                *out_failed_at = i;
                *out_expected  = kinds[i];
                *out_ztype     = (uint8_t)Z_TYPE(args[i]);
                ocv_result_reset(res);
                return 0;
            }
        } else {
            /* optional: غائب → لا يُضاف لـ max_score */
            res->slots[i].present = 0;
            res->slots[i].kind    = kinds[i];
        }
        res->count++;
    }

    return (res->score == res->max_score);
}

/* ============================================================
   Messenger
   ============================================================ */

static inline void ocv_append_zval_desc(smart_str *msg, zval *zv)
{
    switch (Z_TYPE_P(zv)) {
    case IS_OBJECT: smart_str_append(msg, Z_OBJCE_P(zv)->name); break;
    case IS_LONG:
        smart_str_appends(msg, "int(");
        smart_str_append_long(msg, Z_LVAL_P(zv));
        smart_str_appends(msg, ")"); break;
    case IS_DOUBLE: {
        char buf[32]; snprintf(buf, sizeof(buf), "double(%.4g)", Z_DVAL_P(zv));
        smart_str_appends(msg, buf); break;
    }
    case IS_TRUE:  smart_str_appends(msg, "bool(true)");  break;
    case IS_FALSE: smart_str_appends(msg, "bool(false)"); break;
    case IS_STRING: {
        smart_str_appends(msg, "string(\"");
        size_t len = MIN(ZSTR_LEN(Z_STR_P(zv)), 16);
        smart_str_appendl(msg, ZSTR_VAL(Z_STR_P(zv)), (zend_long)len);
        if (ZSTR_LEN(Z_STR_P(zv)) > 16) smart_str_appends(msg, "...");
        smart_str_appends(msg, "\")"); break;
    }
    case IS_ARRAY: {
        char buf[48]; snprintf(buf, sizeof(buf), "array(%d)", (int)zend_array_count(Z_ARRVAL_P(zv)));
        smart_str_appends(msg, buf); break;
    }
    case IS_NULL: smart_str_appends(msg, "null"); break;
    default: smart_str_appends(msg, zend_get_type_by_const(Z_TYPE_P(zv)));
    }
}

static inline void ocv_throw_overload_error(int argc, zval *args, const ocv_hint_t *hint)
{
    smart_str msg = {0};
    const char *fn = ocv_current_function_name();

    smart_str_appends(&msg, "No matching overload for CV\\");
    smart_str_appends(&msg, fn);
    smart_str_appends(&msg, "()\nCalled with: (");
    for (int i = 0; i < argc; i++) {
        if (i) smart_str_appends(&msg, ", ");
        ocv_append_zval_desc(&msg, &args[i]);
    }
    smart_str_appends(&msg, ")");

    if (hint->kinds && hint->count >= 0) {
        smart_str_appends(&msg, "\n\nClosest match:\n    CV\\");
        smart_str_appends(&msg, fn);
        smart_str_appends(&msg, "(");
        for (int i = 0; i < hint->count; i++) {
            if (i) smart_str_appends(&msg, ", ");
            smart_str_appends(&msg, ocv_kind_name(hint->kinds[i]));
        }
        smart_str_appends(&msg, ")");
        if (hint->failed_at >= 0) {
            int offset = (int)strlen(fn) + 1;
            for (int i = 0; i < hint->failed_at; i++)
                offset += (int)strlen(ocv_kind_name(hint->kinds[i])) + 2;
            smart_str_appends(&msg, "\n    ");
            for (int i = 0; i < offset; i++) smart_str_appends(&msg, " ");
            smart_str_appends(&msg, "^");
            smart_str_appends(&msg, "\n    --> arg #");
            smart_str_append_long(&msg, (zend_long)(hint->failed_at + 1));
            smart_str_appends(&msg, ": expected ");
            smart_str_appends(&msg, ocv_kind_name(hint->expected));
            smart_str_appends(&msg, ", got ");
            smart_str_appends(&msg, zend_get_type_by_const(hint->got_ztype));
        }
    }

    smart_str_0(&msg);
    zend_throw_exception_ex(opencv_exception_ce, 0, "%s", ZSTR_VAL(msg.s));
    smart_str_free(&msg);
}

/* ============================================================
   MACROS
   ============================================================ */

#define OCV_OVERLOAD_START()                   \
    ocv_arg_result_t ocv = {{{0}}};            \
    int ocv_matched = 0;                        \
    ocv_hint_t _hint = {                        \
        .kinds      = NULL,                     \
        .count      = 0,                        \
        .score      = -1,                       \
        .failed_at  = -1,                       \
        .expected   = A_NONE,                   \
        .got_ztype  = IS_NULL                   \
    };

#define _OCV_TRY_IMPL(MIN, MAX, ...)                                       \
    do {                                                                    \
        static const arg_kind_t _sig[] =                                   \
            { __VA_ARGS__ __VA_OPT__(,) A_NONE };                          \
        const int _nsig = ocv_sig_count(_sig);                             \
        int _fail_at  = -1;                                                 \
        arg_kind_t _fail_ex = A_NONE;                                      \
        uint8_t    _fail_zt = IS_NULL;                                      \
        if (!ocv_matched &&                                                 \
            ocv_parse_minmax(args, argc, &ocv, (MIN), (MAX),               \
                             _sig, _nsig,                                   \
                             &_fail_at, &_fail_ex, &_fail_zt))             \
        {                                                                   \
            ocv_matched = 1;                                                \
        } else if (!ocv_matched && ocv.score > _hint.score) {              \
            _hint.kinds     = _sig;     _hint.count     = _nsig;           \
            _hint.score     = ocv.score;                                    \
            _hint.failed_at = _fail_at;                                     \
            _hint.expected  = _fail_ex; _hint.got_ztype = _fail_zt;        \
        }                                                                   \
    } while (0);                                                            \
    if (ocv_matched) do

#define OCV_TRY(...)                                                        \
    _OCV_TRY_IMPL(                                                          \
        (int)(sizeof((arg_kind_t[]){ __VA_ARGS__ __VA_OPT__(,) A_NONE })   \
              / sizeof(arg_kind_t) - 1),                                    \
        (int)(sizeof((arg_kind_t[]){ __VA_ARGS__ __VA_OPT__(,) A_NONE })   \
              / sizeof(arg_kind_t) - 1),                                    \
        __VA_ARGS__)

#define OCV_TRY_OPT(MIN, MAX, ...)  _OCV_TRY_IMPL(MIN, MAX, __VA_ARGS__)

#define OCV_END_TRY()  while (0)

#define OCV_OVERLOAD_END()                              \
    if (!ocv_matched) {                                 \
        ocv_throw_overload_error(argc, args, &_hint);  \
        ocv_result_reset(&ocv);                        \
        RETURN_NULL();                                  \
    }                                                   \
    ocv_result_reset(&ocv);

/* ============================================================
   READ MACROS
   ============================================================ */

#define OCV_MAT(i)         ((cv::Mat*)ocv.slots[i].val.ptr)
#define OCV_POINT(i)       (ocv.slots[i].present ? (cv::Point*)ocv.slots[i].val.ptr      : nullptr)
#define OCV_SIZE(i)        (ocv.slots[i].present ? (cv::Size*)ocv.slots[i].val.ptr       : nullptr)
#define OCV_RECT(i)        (ocv.slots[i].present ? (cv::Rect*)ocv.slots[i].val.ptr       : nullptr)
#define OCV_SCALAR(i)      (ocv.slots[i].present ? (cv::Scalar*)ocv.slots[i].val.ptr     : nullptr)
#define OCV_ROTATEDRECT(i) (ocv.slots[i].present ? (cv::RotatedRect*)ocv.slots[i].val.ptr: nullptr)
#define OCV_RANGE(i)       (ocv.slots[i].present ? (cv::Range*)ocv.slots[i].val.ptr      : nullptr)
#define OCV_INT(i, def)    (ocv.slots[i].present ? (int)ocv.slots[i].val.i_val           : (def))
#define OCV_LONG(i, def)   (ocv.slots[i].present ? (zend_long)ocv.slots[i].val.i_val     : (def))
#define OCV_DOUBLE(i, def) (ocv.slots[i].present ? ocv.slots[i].val.d_val                : (def))
#define OCV_BOOL(i, def)   (ocv.slots[i].present ? (zend_bool)ocv.slots[i].val.i_val     : (def))
#define OCV_STRING(i)      (ocv.slots[i].present ? ZSTR_VAL((zend_string*)ocv.slots[i].val.ptr) : nullptr)
#define OCV_ARRAY(i)       (ocv.slots[i].present ? (HashTable*)ocv.slots[i].val.ptr      : nullptr)

#define _OCV_VEC_GET(i, T) \
    (*reinterpret_cast<std::vector<T>*>(((ocv_vec_t*)ocv.slots[i].val.ptr)->data))

#define OCV_VEC_POINT(i)   _OCV_VEC_GET(i, cv::Point)
#define OCV_VEC_POINT2F(i) _OCV_VEC_GET(i, cv::Point2f)
#define OCV_VEC_MAT(i)     _OCV_VEC_GET(i, cv::Mat)
#define OCV_VEC_RECT(i)    _OCV_VEC_GET(i, cv::Rect)
#define OCV_VEC_SCALAR(i)  _OCV_VEC_GET(i, cv::Scalar)
#define OCV_VEC_VEC4I(i)   _OCV_VEC_GET(i, cv::Vec4i)
#define OCV_VEC_INT(i)     _OCV_VEC_GET(i, int)
#define OCV_VEC_DOUBLE(i)  _OCV_VEC_GET(i, double)
#define OCV_VEC_FLOAT(i)   _OCV_VEC_GET(i, float)
#define OCV_VEC_STRING(i)  _OCV_VEC_GET(i, std::string)

#define OCV_VEC_TO_PHP(zv_ptr, vec, kind) \
    _ocv_vec_to_php_dispatch((zv_ptr), (void*)&(vec), (arg_kind_t)(kind))

#define OCV_RETURN_VEC(vec, kind)               \
    do {                                         \
        OCV_VEC_TO_PHP(return_value, vec, kind); \
        return;                                  \
    } while (0)

#define OCV_ARRAY_ADD_VEC(arr_zv, key, vec, kind)  \
    do {                                             \
        zval _tmp_vec;                               \
        OCV_VEC_TO_PHP(&_tmp_vec, (vec), (kind));    \
        add_assoc_zval((arr_zv), (key), &_tmp_vec);  \
    } while (0)

#define OCV_ARRAY_APPEND_VEC(arr_zv, vec, kind)    \
    do {                                             \
        zval _tmp_vec;                               \
        OCV_VEC_TO_PHP(&_tmp_vec, (vec), (kind));    \
        add_next_index_zval((arr_zv), &_tmp_vec);    \
    } while (0)

#define OCV_RETURN_VEC_POINT(v)   OCV_RETURN_VEC((v), A_VEC_POINT)
#define OCV_RETURN_VEC_POINT2F(v) OCV_RETURN_VEC((v), A_VEC_POINT2F)
#define OCV_RETURN_VEC_MAT(v)     OCV_RETURN_VEC((v), A_VEC_MAT)
#define OCV_RETURN_VEC_RECT(v)    OCV_RETURN_VEC((v), A_VEC_RECT)
#define OCV_RETURN_VEC_SCALAR(v)  OCV_RETURN_VEC((v), A_VEC_SCALAR)
#define OCV_RETURN_VEC_VEC4I(v)   OCV_RETURN_VEC((v), A_VEC_VEC4I)
#define OCV_RETURN_VEC_INT(v)     OCV_RETURN_VEC((v), A_VEC_INT)
#define OCV_RETURN_VEC_DOUBLE(v)  OCV_RETURN_VEC((v), A_VEC_DOUBLE)
#define OCV_RETURN_VEC_FLOAT(v)   OCV_RETURN_VEC((v), A_VEC_FLOAT)
#define OCV_RETURN_VEC_STRING(v)  OCV_RETURN_VEC((v), A_VEC_STRING)

#endif /* OCV_OVERLOAD_H */