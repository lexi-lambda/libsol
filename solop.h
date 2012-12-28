/* 
 * File:   solop.h
 * Author: Jake
 *
 * Created on November 19, 2012, 8:04 PM
 */

#ifndef SOLOP_H
#define	SOLOP_H

#include "sol.h"
#include "sollist.h"

typedef SolObject (*SolOperatorRef)(SolList arguments);

typedef struct sol_operator {
    sol_obj super; // extend sol_obj
    SolOperatorRef operation_ref;
} sol_operator;
typedef sol_operator* SolOperator;


// MATH OPERATORS
extern const sol_operator OP_ADD;
extern const sol_operator OP_SUBTRACT;
extern const sol_operator OP_MULTIPLY;
extern const sol_operator OP_DIVIDE;
extern const sol_operator OP_MOD;
// CORE OPERATORS
extern const sol_operator OP_BIND;
extern const sol_operator OP_SET;
extern const sol_operator OP_EVALUATE;
extern const sol_operator OP_LAMBDA;
// I/O OPERATORS
extern const sol_operator OP_PRINT;
// LOGICAL OPERATORS
extern const sol_operator OP_NOT;
extern const sol_operator OP_AND;
extern const sol_operator OP_OR;
// COMPARISON OPERATORS
extern const sol_operator OP_EQUALITY;
extern const sol_operator OP_LESS_THAN;
extern const sol_operator OP_GREATER_THAN;
extern const sol_operator OP_LESS_THAN_EQUALITY;
extern const sol_operator OP_GREATER_THAN_EQUALITY;
// CONTROL OPERATORS
extern const sol_operator OP_IF;
extern const sol_operator OP_LOOP;

/**
 * Generates an object pointer from a default operator.
 * @param operation
 * @return operation
 */
SolOperator sol_operator_create(sol_operator operation);

#endif	/* SOLOP_H */

