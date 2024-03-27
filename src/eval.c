#include "include/eval.h"
#include "include/console.h"

value_t eval_post(post_expr_t* p)
{
    value_t result; result.kind = VAL_NONE;
    if (p->val_kind == POST_TRUE) {
        result.kind = VAL_TRUE;
    } else if (p->val_kind == POST_FALSE) {
        result.kind = VAL_FALSE;
    } else if (p->val_kind == POST_NULL) {
        result.kind = VAL_NULL;
    } else if (p->val_kind == POST_INT) {
        result.kind = VAL_I;
        result.as.ival = p->value.int_value;
    } else if (p->val_kind == POST_FLOAT) {
        result.kind = VAL_D;
        result.as.dval = p->value.double_value;
    } else if (p->val_kind == POST_STR) {
        result.kind = VAL_S;
        result.as.sval = p->value.string_value;
    } else if (p->val_kind == POST_IDENT) {
        log_fatal("Can't const evaluate variable (yet)!");
        return result;
    }

    switch (p->op_kind) {
        case POST_MEMBER_ACCESS:
        case POST_ARRAY_ACCESS:
        case POST_FN_CALL:
        case POST_INC:
        case POST_DEC: {
            log_fatal("Not implemented yet!");
        }
    }
    return result;
}

value_t eval_expr(expr_t* e)
{
    value_t result;
    result.kind = VAL_NONE;  
    switch (e->kind) {
        case EXPR_POST: {
            post_expr_t* p = &e->post;
            return eval_post(p);
        } break;     
        default: {
            log_fatal("Not implemented yet");
        }
    }
    
}