/* 
 * File:   solfunc.h
 * Author: Jake
 *
 * Created on November 19, 2012, 11:43 AM
 */

#ifndef SOLFUNC_H
#define	SOLFUNC_H

#include "sollist.h"
#include "soltoken.h"


typedef struct sol_func {
    sol_obj super; // extend sol_obj
    SolList parameters;
    SolList statements;
    TokenMap closure_scope;
} sol_func;

extern const sol_func DEFAULT_FUNC;

typedef sol_func* SolFunction;


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
SolObject sol_func_execute(SolFunction func, SolList arguments);

#endif	/* SOLFUNC_H */

