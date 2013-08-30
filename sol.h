
#ifndef SOL_H
#define	SOL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "solutils.h"

#define STRUCT_EXTEND(super_name, name, body) struct name ## _raw { body }; typedef struct name { super_name super; body } name

typedef enum obj_type {
    TYPE_SOL_OBJ,
    TYPE_SOL_LIST,
    TYPE_SOL_FUNC,
    TYPE_SOL_DATATYPE,
    TYPE_SOL_TOKEN,
    TYPE_SOL_OBJ_NATIVE
} obj_type;

struct token_pool_entry;
struct sol_event_listener;
enum token_binding_metadata_type;

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

struct sol_obj_native;
typedef struct sol_obj_native* SolObjectNative;
typedef void (*SolObjectNativeDestructor)(SolObjectNative obj);
STRUCT_EXTEND(sol_obj, sol_obj_native,
    void* value;
    SolObjectNativeDestructor dealloc;
);


/**
 * Creates a new "raw" object with no parent and only the get and set methods.
 * @return object
 */
SolObject sol_obj_create_raw();

/**
 * Creates and returns a "native" object, or one that provides a facility for
 * extension with arbitrary data. A deallocation function must be provided.
 */
SolObjectNative sol_obj_clone_native(SolObject parent, void* value, SolObjectNativeDestructor dealloc);

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
 * Copies the properties/prototype values from the patch object onto the parent object. The parent object
 * is modified, but the patch object is left unchanged. Note that while the prototype values are copied,
 * prototype values from the patch's parents are not copied.
 * @param parent
 * @param patch
 */
void sol_obj_patch(SolObject parent, SolObject patch);

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
bool sol_obj_equals(SolObject obj_a, SolObject obj_b);

/**
 * Provides a human-readable representation of an object.
 * @param obj
 * @return
 */
char* sol_obj_to_string(SolObject obj);

/**
 * Provides a human-readable internal representation of an object.
 * @param obj
 * @return 
 */
char* sol_obj_inspect(SolObject obj);

/**
 * Creates a string literal that represents a type.
 * Use sol_datatype_string for datatypes instead.
 * @param type
 * @return 
 */
char* sol_type_string(obj_type type);

/**
 * Creates a string literal that represents an object's raw type.
 * @param obj
 * @return 
 */
char* sol_obj_type_string(SolObject obj);

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
 * Gets a metadata value on a property.
 * @param obj
 * @param token
 * @param type
 * @return value
 */
void* sol_obj_get_prop_metadata(SolObject obj, char* token, enum token_binding_metadata_type type);

/**
 * Sets a metadata value on a property.
 * @param obj
 * @param token
 * @param type
 * @param value
 */
void sol_obj_set_prop_metadata(SolObject obj, char* token, enum token_binding_metadata_type type, void* value);

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

/**
 * Gets a metadata value on a property on the prototype.
 * @param obj
 * @param token
 * @param type
 * @return value
 */
void* sol_obj_get_proto_metadata(SolObject obj, char* token, enum token_binding_metadata_type type);

/**
 * Sets a metadata value on a property on the prototype.
 * @param obj
 * @param token
 * @param type
 * @param value
 */
void sol_obj_set_proto_metadata(SolObject obj, char* token, enum token_binding_metadata_type type, void* value);

#endif	/* SOL_H */

