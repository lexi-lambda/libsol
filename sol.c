
#include <stdio.h>

#include "sol.h"
#include "uthash.h"
#include "soltoken.h"
#include "solfunc.h"
#include "solop.h"
#include "soltypes.h"

const sol_obj DEFAULT_OBJECT = { TYPE_SOL_OBJ, 1, NULL, NULL, NULL };

void* sol_obj_create_global(SolObject parent, obj_type type, void* default_data,  size_t size, char* token) {
    SolObject new_obj = malloc(size);
    memcpy(new_obj, &DEFAULT_OBJECT, sizeof(*new_obj));
    memcpy(new_obj + 1, default_data, size - sizeof(sol_obj));
    new_obj->parent = sol_obj_retain(parent);
    new_obj->type_id = type;
    sol_token_register(token, new_obj);
    return new_obj;
}

SolObject sol_obj_retain(SolObject obj) {
    obj->retain_count++;
    return obj;
}

void sol_obj_release(SolObject obj) {
    obj->retain_count--;
    if (obj->retain_count <= 0) {
        // TODO: handle dealloc of objects' properties/prototypes
        free(obj);
        if (obj->parent != NULL) {
            sol_obj_release(obj->parent);
        }
    }
}

SolObject sol_obj_clone(SolObject obj) {
    SolObject new_obj = malloc(sizeof(*new_obj));
    memcpy(new_obj, &DEFAULT_OBJECT, sizeof(*new_obj));
    new_obj->parent = sol_obj_retain(obj);
    return new_obj;
}

void* sol_obj_clone_type(SolObject obj, void* default_data, size_t size) {
    SolObject new_obj = malloc(size);
    memcpy(new_obj, &DEFAULT_OBJECT, sizeof(*new_obj));
    memcpy(new_obj + 1, default_data, size - sizeof(sol_obj));
    new_obj->type_id = obj->type_id;
    new_obj->parent = sol_obj_retain(obj);
    return new_obj;
}

SolObject sol_obj_evaluate(SolObject obj) {
    // get the object type
    // (keep in mind that SolObjects' first item is an int, so
    //  we can interpret this as an int pointer)
    obj_type type = *((int*) obj);
    switch (type) {
        case TYPE_SOL_OBJ:
        case TYPE_SOL_FUNC:
        case TYPE_SOL_DATATYPE:
            return obj;
        case TYPE_SOL_LIST: {
            SolList list = (SolList) obj;
            if (list->freezeCount < 0) {
                SolObject self = list->object_mode ? sol_obj_evaluate(list->first->value) : nil;
                SolObject first_object = list->object_mode ? sol_obj_get_prop(self, ((SolToken) list->first->next->value)->identifier) : sol_obj_evaluate(list->first->value);
                obj_type first_type = first_object->type_id;
                switch (first_type) {
                    case TYPE_SOL_FUNC: {
                        SolFunction func = (SolFunction) first_object;
                        return sol_func_execute(func, sol_list_get_sublist_s(list, 1), self);
                    }
                    default:
                        fprintf(stderr, "ERROR: Attempted to execute non-executable object.\n");
                        return nil;
                }
            } else {
                return (SolObject) list;
            }
        }
        case TYPE_SOL_TOKEN:
            return sol_token_resolve(((SolToken) obj)->identifier);
        default:
            fprintf(stderr, "WARNING: Encountered unknown obj_type.\n");
            return obj;
    }
}

int sol_obj_equals(SolObject obj_a, SolObject obj_b) {
    if (obj_a->type_id != obj_b->type_id) {
        return 0;
    }
    switch (obj_a->type_id) {
        case TYPE_SOL_DATATYPE:
            if (((SolDatatype) obj_a)->type_id != ((SolDatatype) obj_b)->type_id) return 0;
            switch (((SolDatatype) obj_a)->type_id) {
                case DATA_TYPE_BOOL:
                    return !((SolBoolean) obj_a)->value == !((SolBoolean) obj_b)->value;
                case DATA_TYPE_NUM:
                    return !((SolNumber) obj_a)->value == !((SolNumber) obj_b)->value;
                case DATA_TYPE_STR:
                    return !strcmp(((SolString) obj_a)->value, ((SolString) obj_b)->value);
            }
        case TYPE_SOL_TOKEN:
            return !strcmp(((SolToken) obj_a)->identifier, ((SolToken) obj_b)->identifier);
        default:
            return obj_a == obj_b;
    }
}

char* sol_obj_to_string(SolObject obj) {
    switch (obj->type_id) {
        case TYPE_SOL_OBJ:
            return strdup("Object");
        case TYPE_SOL_FUNC:
            return strdup("lambda");
        case TYPE_SOL_DATATYPE: {;
            SolDatatype datatype = (SolDatatype) obj;
            switch (datatype->type_id) {
                case DATA_TYPE_NUM: {;
                    char buff[32];
                    snprintf(buff, 32, "%lf", ((SolNumber) datatype)->value);
                    return strdup(buff);
                }
                case DATA_TYPE_STR:
                    return strdup(((SolString) datatype)->value);
                case DATA_TYPE_BOOL:
                    return strdup(((SolBoolean) datatype)->value ? "true" : "false");
            }
        }
        case TYPE_SOL_LIST:
            return strdup("List");
        case TYPE_SOL_TOKEN:
            return strdup(((SolToken) obj)->identifier);
    }
}

SolObject sol_obj_get_prop(SolObject obj, char* token) {
    // first check properties for token
    TokenPoolEntry resolved_token;
    HASH_FIND_STR(obj->properties, token, resolved_token);
    if (resolved_token != NULL) {
        return *resolved_token->value;
    }
    
    // loop through prototypes to find the token
    SolObject current_obj = obj;
    do {
        TokenPoolEntry resolved_token;
        HASH_FIND_STR(current_obj->prototype, token, resolved_token);
        if (resolved_token != NULL) {
            return *resolved_token->value;
        }
    } while ((current_obj = current_obj->parent) != NULL);
    
    // if it cannot be found, return NULL
    return NULL;
}

void sol_obj_set_prop(SolObject obj, char* token, SolObject value) {
    TokenPoolEntry new_token;
    // check if entry already exists
    HASH_FIND_STR(obj->properties, token, new_token);
    if (new_token == NULL) {
        new_token = malloc(sizeof(sol_token));
        new_token->identifier = token;
        HASH_ADD_KEYPTR(hh, obj->properties, new_token->identifier, strlen(new_token->identifier), new_token);
    }
    new_token->value = &value;
}

SolObject sol_obj_get_proto(SolObject obj, char* token) {
    TokenPoolEntry resolved_token;
    HASH_FIND_STR(obj->prototype, token, resolved_token);
    if (resolved_token != NULL) {
        return *resolved_token->value;
    }
    
    // if it cannot be found, return NULL
    return NULL;
}

void sol_obj_set_proto(SolObject obj, char* token, SolObject value) {
    TokenPoolEntry new_token;
    // check if entry already exists
    HASH_FIND_STR(obj->prototype, token, new_token);
    if (new_token == NULL) {
        new_token = malloc(sizeof(sol_token));
        new_token->identifier = token;
        HASH_ADD_KEYPTR(hh, obj->properties, new_token->identifier, strlen(new_token->identifier), new_token);
    }
    new_token->value = &value;
}
