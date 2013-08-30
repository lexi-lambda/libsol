
#ifndef SOLFUNC_H
#define	SOLFUNC_H

#include "sollist.h"
#include "soltoken.h"

typedef enum func_type {
    FUNC_TYPE_FUNCTION,
    FUNC_TYPE_MACRO,
    FUNC_TYPE_OPERATOR
} func_type;

STRUCT_EXTEND(sol_obj, sol_func,
    func_type type_id;
    SolList parameters;
    SolList statements;
    TokenMap closure_scope;
);
typedef sol_func* SolFunction;
extern SolFunction Function;


/**
 * Creates a new function object.
 * @param parameters
 * @param statements
 * @return function object
 */
SolFunction sol_func_create(SolList parameters, SolList statements);

SolFunction sol_macro_create(SolList parameters, SolList statements);

/**
 * Executes a function.
 * @param func
 * @param arguments
 * @return function return value
 */
SolObject sol_func_execute(SolFunction func, SolList arguments, SolObject self);

#endif	/* SOLFUNC_H */

