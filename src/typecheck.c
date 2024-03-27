#include "include/parser.h"
#include "include/console.h"
#include "include/misc.h"
#include "include/map.h"
#include "include/eval.h"

extern type_t* builtin_i64;
extern type_t* builtin_i32;
extern type_t* builtin_i16;
extern type_t* builtin_i8;
extern type_t* builtin_u64;
extern type_t* builtin_u32;
extern type_t* builtin_u16;
extern type_t* builtin_u8;
extern type_t* builtin_bool;

typedef struct {
    scope_t* global_scope;
    scope_t* cur_scope;
} checker_t;

checker_t checker;

type_t* resolve_type(str_t ident, scope_t* s)
{
    do {
        symbol_t* r = map_gets(&s->syms, ident);
        if (r && r->kind == SYMBOL_TYPE) return &r->_type;
    } while ((s = s->parent) != null);
    return null;
}

// returns true if the expr can be evaluated at compile time
bool typecheck_expr(expr_t* e, scope_t* s, type_t* expected)
{
    log_fatal("TYPECHECK_EXPR is not implemented yet!");
    return false;
}

void infer_type(type_ref_t* r)
{
    log_fatal("INFER_TYPE is not implemented yet!");
    r->resolved = false;
    return;
}

void finalize_type_ref(type_ref_t* r)
{
    if (r->resolved) return;
    // r is not resolved
    if (r->inferred) {
        infer_type(r);
        return;
    }
    if (r->is_ptr) {
        finalize_type_ref(r->rhs);
        r->resolved = r->rhs->resolved;
        return;
    }
    if (r->is_array) {
        finalize_type_ref(r->rhs);
        r->resolved = r->rhs->resolved;
        bool can_eval = typecheck_expr(r->array_len, checker.cur_scope, builtin_u32);
        if (!can_eval) {
            // TODO: rewrite expr parsing
            make_error("Length of array has to be known at compile-time", r->array_len->loc);
            r->resolved = false; // signal that an error has happened
            return;
        }
        value_t v = eval_expr(r->array_len);
        if (v.kind == VAL_NONE) {
            return;
        } else if (v.kind != VAL_U) {
            make_error("Length of array must be of type u32", r->array_len->loc);
        }
        r->array_len = (void*)v.as.uval;
        return;
    }

    type_t* t = map_gets(&checker.cur_scope->syms, r->ident);
    if (t == null) {
        make_error("Unknown type", r->loc);
        return;
    }
    r->resolved = true; r->resolved_type = t;
    return;
}

u32 align_up(u32* off, u8 align)
{
    if (*off == 0) return 0; 
    *off += align - (*off & (align-1));
    return *off;
}

u32 type_get_align(type_ref_t* t)
{
    if (t->is_ptr) return 8;
    if (t->is_array) {
        // at this point array_len is no longer a pointer but the result of the array_len expr
        return t->resolved_type->size;
    }
    // basic types are initialized with element_size == 0
    if (t->resolved_type->fields.element_size == 0) return t->resolved_type->size;
    else {
        type_member_t* f = array_get(&t->resolved_type->fields, 0);
        finalize_type_ref(&f->type); 
        if (!f->type.resolved) return 0;
        return type_get_align(&f->type);
    }
}

// aligns structs according to the first element
// TODO: test this and implement multiple passes for finalizing structs
void finalize_struct(type_t* t)
{
    u32 cur_off = 0;
    u32 biggest_align = 0;
    u32 _count;
    for_array(&t->fields, type_member_t)
        finalize_type_ref(&e->type);
        if (!e->type.resolved) return;
        u32 align = type_get_align(&e->type);
        biggest_align = align > biggest_align ? align : biggest_align;
        if (align == 0) { t->size = 0; return; }
        cur_off += align_up(&cur_off, align);
    }
    // end align
    align_up(&cur_off, biggest_align);
    t->size = cur_off;
}

void finalize_union(union_t* u)
{
    log_fatal("Finalizing union is not implemented yet!");
}

void finalize_syms(file_t* ast)
{
    map_t* c = map_get_at(&checker.cur_scope->syms, 0);
    do {
        symbol_t* s = c->value;
        if (s->kind == SYMBOL_TYPE) {
            finalize_struct(&s->_type);
        } else if (s->kind == SYMBOL_UNION) {
            finalize_union(&s->_union);
        }
    } while ((c = map_next(c)));
}

void typecheck_ast(file_t* ast)
{
    checker.global_scope = ast->global_scope;
    checker.cur_scope = checker.global_scope;

    // STEP 1: Resolve structures
    finalize_syms(ast);
}