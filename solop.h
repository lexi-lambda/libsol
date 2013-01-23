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

typedef SolObject (*SolOperatorRef)(SolList arguments, SolObject self);

STRUCT_EXTEND(sol_obj, sol_operator,
    bool is_operator;
    SolOperatorRef operator_ref;
);
typedef sol_operator* SolOperator;


/* CORE FUNCTIONS */
// MATH
extern const SolOperatorRef OP_ADD;
extern const SolOperatorRef OP_SUBTRACT;
extern const SolOperatorRef OP_MULTIPLY;
extern const SolOperatorRef OP_DIVIDE;
extern const SolOperatorRef OP_MOD;
// CORE
extern const SolOperatorRef OP_REQUIRE;
extern const SolOperatorRef OP_BIND;
extern const SolOperatorRef OP_SET;
extern const SolOperatorRef OP_EVALUATE;
extern const SolOperatorRef OP_LAMBDA;
// I/O
extern const SolOperatorRef OP_PRINT;
// LOGIC
extern const SolOperatorRef OP_NOT;
extern const SolOperatorRef OP_AND;
extern const SolOperatorRef OP_OR;
// COMPARISON
extern const SolOperatorRef OP_EQUALITY;
extern const SolOperatorRef OP_LESS_THAN;
extern const SolOperatorRef OP_GREATER_THAN;
extern const SolOperatorRef OP_LESS_THAN_EQUALITY;
extern const SolOperatorRef OP_GREATER_THAN_EQUALITY;
// CONTROL
extern const SolOperatorRef OP_IF;
extern const SolOperatorRef OP_LOOP;

/* OBJECT FUNCTIONS */
// OBJECT
extern const SolOperatorRef OP_OBJECT_GET;
extern const SolOperatorRef OP_OBJECT_SET;
extern const SolOperatorRef OP_OBJECT_CLONE;
// NUMBER
// STRING
// extern const SolOperatorRef OP_STRING_CONCATENATE;


/**
 * Creates a sol operator object from a compliant function pointer.
 * A SolOperator is a special sol object that masquerades as a SolFunction.
 * It can be used with the standard sol_func_* functions in place of
 * SolFunction parameters. However, SolOperators are defined at compile time
 * in native C code, not at runtime in sol.
 * @param operator_ref a valid function pointer
 * @return a new operator object
 */
SolOperator sol_operator_create(SolOperatorRef operator_ref);

#endif	/* SOLOP_H */

