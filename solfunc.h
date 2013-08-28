
#ifndef SOLFUNC_H
#define	SOLFUNC_H

#include "sollist.h"
#include "soltoken.h"

STRUCT_EXTEND(sol_obj, sol_func,
    bool is_operator;
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

/**
 * Executes a function.
 * @param func
 * @param arguments
 * @return function return value
 */
SolObject sol_func_execute(SolFunction func, SolList arguments, SolObject self);

#endif	/* SOLFUNC_H */

