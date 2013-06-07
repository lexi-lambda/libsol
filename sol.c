
#include <stdio.h>
#include <stdarg.h>

#include "sol.h"
#include "uthash.h"
#include "soltoken.h"
#include "solfunc.h"
#include "solop.h"
#include "soltypes.h"

const sol_obj DEFAULT_OBJECT = { TYPE_SOL_OBJ, 0, NULL, NULL, NULL, NULL };

SolObject sol_obj_create_raw() {
    return sol_obj_clone(RawObject);
}

void* sol_obj_create_global(SolObject parent, obj_type type, void* default_data, size_t size, char* token) {
    SolObject new_obj = malloc(size);
    memcpy(new_obj, &DEFAULT_OBJECT, sizeof(*new_obj));
    memcpy(new_obj + 1, default_data, size - sizeof(sol_obj));
    new_obj->parent = sol_obj_retain(parent);
    new_obj->type_id = type;
    sol_token_register(token, new_obj);
    return new_obj;
}

SolObject sol_obj_retain(SolObject obj) {
    if (obj != NULL)
        obj->retain_count++;
    return obj;
}

void sol_obj_release(SolObject obj) {
    if (obj == NULL || obj == nil) return;
    obj->retain_count--;
    if (obj->retain_count <= 0) {
        // release all properties
        TokenPoolEntry current_token, tmp;
        HASH_ITER(hh, obj->properties, current_token, tmp) {
            sol_obj_release(current_token->binding->value);
            free(current_token->identifier);
            free(current_token->binding);
            HASH_DEL(obj->properties, current_token);
            free(current_token);
        }
        // release all prototypes
        HASH_ITER(hh, obj->prototype, current_token, tmp) {
            free(current_token->identifier);
            sol_obj_release(current_token->binding->value);
            HASH_DEL(obj->prototype, current_token);
            free(current_token);
        }
        // handle type-specific memory mangement
        switch (obj->type_id) {
            case TYPE_SOL_DATATYPE: {
                SolDatatype datatype = (SolDatatype) obj;
                switch (datatype->type_id) {
                    case DATA_TYPE_STR:
                        free(((SolString) datatype)->value);
                        break;
                    default:
                        break;
                }
                break;
            }
            case TYPE_SOL_FUNC: {
                SolFunction func = (SolFunction) obj;
                if (func->is_operator) break;
                TokenPoolEntry current_token, tmp;
                HASH_ITER(hh, func->closure_scope, current_token, tmp) {
                    sol_obj_release(current_token->binding->value);
                    free(current_token->identifier);
                    if (--current_token->binding->retain_count <= 0) free(current_token->binding);
                    HASH_DEL(func->closure_scope, current_token);
                    free(current_token);
                }
                sol_obj_release((SolObject) func->parameters);
                sol_obj_release((SolObject) func->statements);
                break;
            }
            case TYPE_SOL_LIST: {
                SolList list = (SolList) obj;
                list->current = list->first;
                while (list->current != NULL) {
                    sol_list_node* current = list->current;
                    list->current = list->current->next;
                    sol_obj_release(current->value);
                    free(current);
                }
                break;
            }
            case TYPE_SOL_TOKEN: {
                SolToken token = (SolToken) obj;
                free(token->identifier);
                break;
            }
            case TYPE_SOL_OBJ_FROZEN:
                sol_obj_release(((SolObjectFrozen) obj)->value);
                break;
            case TYPE_SOL_OBJ:
                break;
        }
        // release parent
        if (obj->parent != NULL) {
            sol_obj_release(obj->parent);
        }
        // free object
        free(obj);
    }
}

void sol_obj_patch(SolObject parent, SolObject patch) {
    TokenPoolEntry current_token, tmp;
    HASH_ITER(hh, patch->properties, current_token, tmp) {
        sol_obj_set_prop(parent, current_token->identifier, current_token->binding->value);
    }
    HASH_ITER(hh, patch->prototype, current_token, tmp) {
        sol_obj_set_proto(parent, current_token->identifier, current_token->binding->value);
    }
}

SolObject sol_obj_clone(SolObject obj) {
    SolObject new_obj = malloc(sizeof(*new_obj));
    memcpy(new_obj, &DEFAULT_OBJECT, sizeof(*new_obj));
    new_obj->parent = sol_obj_retain(obj);
    return sol_obj_retain(new_obj);
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
    obj_type type = *((obj_type*) obj);
    switch (type) {
        case TYPE_SOL_OBJ:
        case TYPE_SOL_FUNC:
        case TYPE_SOL_DATATYPE:
            return sol_obj_retain(obj);
        case TYPE_SOL_LIST: {
            SolList list = (SolList) obj;
            SolObject self = list->object_mode ? sol_obj_evaluate(list->first->value) : nil;
            SolObject first_object = list->object_mode ? sol_obj_get_prop(self, ((SolToken) list->first->next->value)->identifier) : sol_obj_evaluate(list->first->value);
            obj_type first_type = first_object->type_id;
            switch (first_type) {
                case TYPE_SOL_FUNC: {
                    SolFunction func = (SolFunction) first_object;
                    SolList arguments = sol_list_slice_s(list, list->object_mode ? 2 : 1);
                    SolObject result = sol_func_execute(func, arguments, self);
                    sol_obj_release((SolObject) arguments);
                    sol_obj_release(self);
                    sol_obj_release(first_object);
                    return result;
                }
                default:
                    fprintf(stderr, "ERROR: Attempted to execute non-executable object.\n");
                    sol_obj_release(self);
                    sol_obj_release(first_object);
                    return nil;
            }
        }
        case TYPE_SOL_TOKEN: {
            SolToken token = (SolToken) obj;
            return sol_token_resolve(token->identifier);
        }
        case TYPE_SOL_OBJ_FROZEN:
            return sol_obj_retain(((SolObjectFrozen) obj)->value);
        default:
            fprintf(stderr, "WARNING: Encountered unknown obj_type.\n");
            return sol_obj_retain(obj);
    }
}

SolObjectFrozen sol_obj_freeze(SolObject obj) {
    SolObjectFrozen frozen = sol_obj_clone_type(Object, &(struct sol_obj_frozen_raw){
        sol_obj_retain(obj)
    }, sizeof(*frozen));
    frozen->super.type_id = TYPE_SOL_OBJ_FROZEN;
    return frozen;
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
                    return ((SolNumber) obj_a)->value == ((SolNumber) obj_b)->value;
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
    if (obj == NULL) return strdup("undefined");
    SolFunction to_string = (SolFunction) sol_obj_get_prop(obj, "->string");
    SolString ret = (SolString) sol_func_execute(to_string, (SolList) nil, obj);
    if (ret == NULL) return strdup("undefined");
    char* ret_value = strdup(ret->value);
    sol_obj_release((SolObject) ret);
    sol_obj_release((SolObject) to_string);
    return ret_value;
}

SolObject sol_obj_get_prop(SolObject obj, char* token) {
    // first check properties for token
    TokenPoolEntry resolved_token;
    HASH_FIND_STR(obj->properties, token, resolved_token);
    if (resolved_token != NULL) {
        if (resolved_token->metadata.get != NULL) {
            return sol_func_execute(resolved_token->metadata.get, (SolList) nil, obj);
        }
        return sol_obj_retain(resolved_token->binding->value);
    }
    
    // loop through prototypes to find the token
    SolObject current_obj = obj;
    do {
        TokenPoolEntry resolved_token;
        HASH_FIND_STR(current_obj->prototype, token, resolved_token);
        if (resolved_token != NULL) {
            if (resolved_token->metadata.get != NULL) {
                return sol_func_execute(resolved_token->metadata.get, (SolList) nil, obj);
            }
            return sol_obj_retain(resolved_token->binding->value);
        }
    } while ((current_obj = current_obj->parent) != NULL);
    
    // if it cannot be found, return NULL
    return NULL;
}

void sol_obj_set_prop(SolObject obj, char* token, SolObject value) {
    TokenPoolEntry new_token;
    sol_obj_retain(value);
    // check if entry already exists
    HASH_FIND_STR(obj->properties, token, new_token);
    if (new_token == NULL) {
        new_token = calloc(1, sizeof(*new_token));
        new_token->identifier = strdup(token);
        new_token->binding = malloc(sizeof(*new_token->binding));
        HASH_ADD_KEYPTR(hh, obj->properties, new_token->identifier, strlen(new_token->identifier), new_token);
    } else {
        sol_obj_release(new_token->binding->value);
    }
    if (new_token->metadata.set != NULL) {
        SolList arguments = (SolList) sol_obj_retain((SolObject) sol_list_create(false));
        sol_list_add_obj(arguments, value);
        sol_obj_release(value);
        value = sol_func_execute(new_token->metadata.set, arguments, obj);
        sol_obj_release((SolObject) arguments);
    } else {
        // loop through prototypes to find setters
        SolObject current_obj = obj;
        do {
            TokenPoolEntry resolved_token;
            HASH_FIND_STR(current_obj->prototype, token, resolved_token);
            if (resolved_token != NULL && resolved_token->metadata.set != NULL) {
                SolList arguments = (SolList) sol_obj_retain((SolObject) sol_list_create(false));
                sol_list_add_obj(arguments, value);
                sol_obj_release(value);
                value = sol_func_execute(resolved_token->metadata.set, arguments, obj);
                sol_obj_release((SolObject) arguments);
                break;
            }
        } while ((current_obj = current_obj->parent) != NULL);
    }
    new_token->binding->value = value;
    new_token->binding->retain_count = 1;
}

void* sol_obj_get_prop_metadata(SolObject obj, char* token, enum token_binding_metadata_type type) {
    // first check properties for token
    TokenPoolEntry resolved_token;
    HASH_FIND_STR(obj->properties, token, resolved_token);
    if (resolved_token != NULL) {
        switch (type) {
            case METADATA_GET:
                return sol_obj_retain((SolObject) resolved_token->metadata.get);
                break;
            case METADATA_SET:
                return sol_obj_retain((SolObject) resolved_token->metadata.set);
                break;
        }
    }
    
    // loop through prototypes to find the token
    SolObject current_obj = obj;
    do {
        TokenPoolEntry resolved_token;
        HASH_FIND_STR(current_obj->prototype, token, resolved_token);
        if (resolved_token != NULL) {
            switch (type) {
                case METADATA_GET:
                    return sol_obj_retain((SolObject) resolved_token->metadata.get);
                    break;
                case METADATA_SET:
                    return sol_obj_retain((SolObject) resolved_token->metadata.set);
                    break;
            }
        }
    } while ((current_obj = current_obj->parent) != NULL);
    
    // if it cannot be found, return NULL
    return NULL;
}

void sol_obj_set_prop_metadata(SolObject obj, char* token, enum token_binding_metadata_type type, void* value) {
    TokenPoolEntry new_token;
    // check if entry already exists
    HASH_FIND_STR(obj->properties, token, new_token);
    if (new_token == NULL) {
        new_token = calloc(1, sizeof(*new_token));
        new_token->identifier = strdup(token);
        new_token->binding = malloc(sizeof(*new_token->binding));
        new_token->binding->retain_count = 1;
        HASH_ADD_KEYPTR(hh, obj->properties, new_token->identifier, strlen(new_token->identifier), new_token);
    }
    switch (type) {
        case METADATA_GET:
            new_token->metadata.get = (SolFunction) sol_obj_retain(value);
            break;
        case METADATA_SET:
            new_token->metadata.set = (SolFunction) sol_obj_retain(value);
            break;
    }
}

SolObject sol_obj_get_proto(SolObject obj, char* token) {
    TokenPoolEntry resolved_token;
    HASH_FIND_STR(obj->prototype, token, resolved_token);
    if (resolved_token != NULL) {
        if (resolved_token->metadata.get != NULL) {
            return sol_func_execute(resolved_token->metadata.get, (SolList) nil, obj);
        }
        return sol_obj_retain(resolved_token->binding->value);
    }
    
    // if it cannot be found, return NULL
    return NULL;
}

void sol_obj_set_proto(SolObject obj, char* token, SolObject value) {
    TokenPoolEntry new_token;
    sol_obj_retain(value);
    // check if entry already exists
    HASH_FIND_STR(obj->prototype, token, new_token);
    if (new_token == NULL) {
        new_token = calloc(1, sizeof(*new_token));
        new_token->identifier = strdup(token);
        new_token->binding = malloc(sizeof(*new_token->binding));
        HASH_ADD_KEYPTR(hh, obj->prototype, new_token->identifier, strlen(new_token->identifier), new_token);
    } else {
        sol_obj_release(new_token->binding->value);
    }
    if (new_token->metadata.set != NULL) {
        SolList arguments = (SolList) sol_obj_retain((SolObject) sol_list_create(false));
        sol_list_add_obj(arguments, value);
        sol_obj_release(value);
        value = sol_func_execute(new_token->metadata.set, arguments, obj);
        sol_obj_release((SolObject) arguments);
    } else {
        // loop through prototypes to find setters
        SolObject current_obj = obj;
        while ((current_obj = current_obj->parent) != NULL) {
            TokenPoolEntry resolved_token;
            HASH_FIND_STR(current_obj->prototype, token, resolved_token);
            if (resolved_token != NULL && resolved_token->metadata.set != NULL) {
                SolList arguments = (SolList) sol_obj_retain((SolObject) sol_list_create(false));
                sol_list_add_obj(arguments, value);
                sol_obj_release(value);
                value = sol_func_execute(resolved_token->metadata.set, arguments, obj);
                sol_obj_release((SolObject) arguments);
                break;
            }
        }
    }
    new_token->binding->value = value;
    new_token->binding->retain_count = 1;
}

void* sol_obj_get_proto_metadata(SolObject obj, char* token, enum token_binding_metadata_type type) {
    TokenPoolEntry resolved_token;
    HASH_FIND_STR(obj->prototype, token, resolved_token);
    if (resolved_token != NULL) {
        switch (type) {
            case METADATA_GET:
                return sol_obj_retain((SolObject) resolved_token->metadata.get);
                break;
            case METADATA_SET:
                return sol_obj_retain((SolObject) resolved_token->metadata.set);
                break;
        }
    }
    
    // if it cannot be found, return NULL
    return NULL;
}

void sol_obj_set_proto_metadata(SolObject obj, char* token, enum token_binding_metadata_type type, void* value) {
    TokenPoolEntry new_token;
    // check if entry already exists
    HASH_FIND_STR(obj->prototype, token, new_token);
    if (new_token == NULL) {
        new_token = calloc(1, sizeof(*new_token));
        new_token->identifier = strdup(token);
        new_token->binding = malloc(sizeof(*new_token->binding));
        new_token->binding->retain_count = 1;
        HASH_ADD_KEYPTR(hh, obj->prototype, new_token->identifier, strlen(new_token->identifier), new_token);
    }
    switch (type) {
        case METADATA_GET:
            new_token->metadata.get = (SolFunction) sol_obj_retain(value);
            break;
        case METADATA_SET:
            new_token->metadata.set = (SolFunction) sol_obj_retain(value);
            break;
    }
}
