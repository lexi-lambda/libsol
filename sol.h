/* 
 * File:   sol.h
 * Author: Jake
 *
 * Created on November 18, 2012, 8:54 PM
 */

#ifndef SOL_H
#define	SOL_H

#include <stdlib.h>
#include <stdbool.h>

#define STRUCT_EXTEND(super_name, name, body) struct name ## _raw { body }; typedef struct name { super_name super; body } name

typedef enum {
    TYPE_SOL_OBJ,
    TYPE_SOL_LIST,
    TYPE_SOL_FUNC,
    TYPE_SOL_DATATYPE,
    TYPE_SOL_TOKEN
} obj_type;

struct token_pool_entry;
struct sol_event_listener;

typedef struct sol_obj {
    obj_type type_id;
    int retain_count;
    struct sol_obj* parent;
    struct token_pool_entry* prototype;
    struct token_pool_entry* properties;
    struct sol_event_listener* listeners;
} sol_obj;

extern const sol_obj DEFAULT_OBJECT;

typedef sol_obj* SolObject;

extern SolObject Object;
extern SolObject RawObject;
extern SolObject nil;

/**
 * Creates a new "raw" object with no parent and only the get and set methods.
 * @return object
 */
SolObject sol_obj_create_raw();

/**
 * Creates a new object from the given parent with the given type.
 * Should only be used by runtime internals for initializing new object types.
 * @param parent the object to clone from
 * @param type the type of object to create
 * @param default_data a pointer to a piece of data to copy to the new object (after default Object properties)
 * @param size the size of the resulting object
 * @param token the token to bind the object to
 * @return the new object
 */
void* sol_obj_create_global(SolObject parent, obj_type type, void* default_data, size_t size, char* token);

/**
 * Retains an object, incrementing its retain count and preventing deallocation.
 * @param obj
 * @return retained object
 */
SolObject sol_obj_retain(SolObject obj);

/**
 * Releases an object, decrementing its retain count and deallocating if necessary.
 * @param obj
 */
void sol_obj_release(SolObject obj);

/**
 * Clones an object, returning a new object with its parent prototype
 * set to the cloned object's.
 * @param obj
 * @return new object
 */
SolObject sol_obj_clone(SolObject obj);

/**
 * Clones an object to a given size. Meant to be used only for creating clones
 * of sol types, such as Number, String, Token, etc.
 * @param obj
 * @param default_data a pointer to a piece of data to copy to the new object (after default Object properties)
 * @param size
 * @return new object
 */
void* sol_obj_clone_type(SolObject obj, void* default_data, size_t size);

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

