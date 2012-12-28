/* 
 * File:   sol.h
 * Author: Jake
 *
 * Created on November 18, 2012, 8:54 PM
 */

#ifndef SOL_H
#define	SOL_H

#include <stdlib.h>

typedef enum {
    TYPE_SOL_OBJ,
    TYPE_SOL_LIST,
    TYPE_SOL_FUNC,
    TYPE_SOL_DATATYPE,
    TYPE_SOL_OPERATOR,
    TYPE_SOL_TOKEN
} obj_type;

struct token_pool_entry;

typedef struct sol_obj {
    obj_type type_id;
    struct sol_obj* parent;
    struct token_pool_entry* prototype;
    struct token_pool_entry* properties;
} sol_obj;

extern const sol_obj DEFAULT_OBJECT;

typedef sol_obj* SolObject;

extern SolObject Object;
extern SolObject nil;

/**
 * Clones an object, returning a new object with its parent prototype
 * set to the cloned object's.
 * @param obj
 * @return new object
 */
SolObject sol_obj_clone(SolObject obj);

/**
 * Evaluates an object.
 * @param obj
 * @return object
 */
SolObject sol_obj_evaluate(SolObject obj);

/**
 * Tests to see if two objects are considered equal.
 * @param obj_a
 * @param obj_b
 * @return 
 */
int sol_obj_equals(SolObject obj_a, SolObject obj_b);

/**
 * Provides a human-readable representation of an object.
 * @param obj
 * @return 
 */
char* sol_obj_to_string(SolObject obj);

/**
 * Gets a property on an object. This function searches ivars and all
 * associated prototypes.
 * @param obj
 * @param token
 * @return object
 */
SolObject sol_obj_get_prop(SolObject obj, char* token);

/**
 * Sets an ivar property on an object.
 * @param obj
 * @param token
 * @param value
 */
void sol_obj_set_prop(SolObject obj, char* token, SolObject value);

/**
 * Gets a prototype property for an object.
 * @param obj
 * @param token
 * @return object
 */
SolObject sol_obj_get_proto(SolObject obj, char* token);

/**
 * Sets a prototype property for an object.
 * @param obj
 * @param token
 * @param value
 */
void sol_obj_set_proto(SolObject obj, char* token, SolObject value);

#endif	/* SOL_H */

