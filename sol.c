
#include <stdio.h>

#include "sol.h"
#include "uthash.h"
#include "soltoken.h"
#include "solfunc.h"
#include "solop.h"
#include "soltypes.h"

const sol_obj DEFAULT_OBJ = {
    TYPE_SOL_OBJ, NULL, NULL, NULL
};

SolObject Object;
SolObject nil;

SolObject sol_obj_clone(SolObject obj) {
    SolObject new_obj = malloc(sizeof(*new_obj));
    new_obj->parent = obj;
    new_obj->prototype = NULL;
    new_obj->properties = NULL;
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
        case TYPE_SOL_OPERATOR:
            return obj;
        case TYPE_SOL_LIST: {;
            SolList list = (SolList) obj;
            if (list->freezeCount < 0) {
                SolObject first_object = sol_obj_evaluate(list->first->value);
                obj_type first_type = first_object->type_id;
                switch (first_type) {
                    case TYPE_SOL_FUNC: {;
                        SolFunction func = (SolFunction) first_object;
                        return sol_func_execute(func, sol_list_get_sublist_s(list, 1));
                    }
                    case TYPE_SOL_OPERATOR: {
                        SolOperator operation = (SolOperator) first_object;
                        return (*operation->operation_ref)(sol_list_get_sublist_s(list, 1));
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
        case TYPE_SOL_OPERATOR:
            return ((SolOperator) obj_a)->operation_ref == ((SolOperator) obj_b)->operation_ref;
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
            }
        }
        case TYPE_SOL_OPERATOR:
            return strdup("operator");
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
