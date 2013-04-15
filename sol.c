
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
    obj_type type = *((int*) obj);
    switch (type) {
        case TYPE_SOL_OBJ:
        case TYPE_SOL_FUNC:
        case TYPE_SOL_DATATYPE:
            return sol_obj_retain(obj);
        case TYPE_SOL_LIST: {
            SolList list = (SolList) obj;
            if (list->freezeCount < 0) {
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
            } else {
                return sol_obj_retain((SolObject) list);
            }
        }
        case TYPE_SOL_TOKEN:
            return sol_token_resolve(((SolToken) obj)->identifier);
        default:
            fprintf(stderr, "WARNING: Encountered unknown obj_type.\n");
            return sol_obj_retain(obj);
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

#define strbuild(buff, offset, buff_size, out, format, ...) \
  do {                                                      \
    snprintf(NULL, 0, format "%n", ##__VA_ARGS__, &out);    \
    while (out > buff_size - offset) {                      \
        buff_size *= 2;                                     \
        realloc(buff, buff_size);                           \
    }                                                       \
    sprintf(buff + offset, format, ##__VA_ARGS__);          \
    offset += out;                                          \
  } while (1);
static int sol_obj_indent_level = 0;
char* sol_obj_to_string(SolObject obj) {
    if (obj == NULL) return strdup("undefined");
    switch (obj->type_id) {
        case TYPE_SOL_OBJ: {
            size_t buff_size = 128;
            char* buff = malloc(buff_size);
            off_t buff_offset = 0;
            int buff_tmp;
            if (obj->parent == RawObject) {
                strbuild(buff, buff_offset, buff_size, buff_tmp, "{\n");
            } else {
                strbuild(buff, buff_offset, buff_size, buff_tmp, "@{\n");
            }
            sol_obj_indent_level++;
            TokenPoolEntry el, tmp;
            HASH_ITER(hh, obj->properties, el, tmp) {
                char* key = el->identifier;
                char* value = sol_obj_to_string(el->binding->value);
                for (int i = 0; i < sol_obj_indent_level; i++) {
                    strbuild(buff, buff_offset, buff_size, buff_tmp, "  ");
                }
                strbuild(buff, buff_offset, buff_size, buff_tmp, "%s %s\n", key, value);
                free(value);
            }
            sol_obj_indent_level--;
            for (int i = 0; i < sol_obj_indent_level; i++) {
                strbuild(buff, buff_offset, buff_size, buff_tmp, "  ");
            }
            strbuild(buff, buff_offset, buff_size, buff_tmp, "}");
            return buff;
        }
        case TYPE_SOL_FUNC:
            return strdup("Function");
        case TYPE_SOL_DATATYPE: {
            SolDatatype datatype = (SolDatatype) obj;
            switch (datatype->type_id) {
                case DATA_TYPE_NUM: {
                    char* value;
                    asprintf(&value, "%.8f", ((SolNumber) datatype)->value);
                    char* value_end = value + strlen(value) - 1;
                    while (*value_end == '0') {
                        *value_end = '\0';
                        value_end--;
                    }
                    if (*value_end == '.') {
                        *value_end = '\0';
                    }
                    char* ret = strdup(value);
                    free(value);
                    return ret;
                }
                case DATA_TYPE_STR: {
                    char* ret;
                    asprintf(&ret, "\"%s\"", ((SolString) datatype)->value);
                    return ret;
                }
                case DATA_TYPE_BOOL:
                    return strdup(((SolBoolean) datatype)->value ? "true" : "false");
            }
        }
        case TYPE_SOL_LIST: {
            SolList list = (SolList) obj;
            if (list->length > 0) {
                int buffer_len = 512;
                char* buffer = malloc(buffer_len);
                buffer[0] = '\0';
                char* str = buffer;
                
                if (list->object_mode) str += sprintf(str, "@");
                if (list->freezeCount < 0) str += sprintf(str, "[");
                else str += sprintf(str, "(");
                
                int i = 0;
                SOL_LIST_ITR_BEGIN(list)
                    char* obj = sol_obj_to_string(list->current->value);
                    while (str + strlen(obj) - buffer > buffer_len) {
                        buffer = realloc(buffer, buffer_len *= 2);
                        str = buffer + strlen(buffer);
                    }
                    str += sprintf(str, "%s", obj);
                    free(obj);
                    if (i < list->length - 1) str += sprintf(str, " ");
                    i++;
                SOL_LIST_ITR_END(list)
                
                if (list->freezeCount < 0) str += sprintf(str, "]");
                else str += sprintf(str, ")");
                
                char* ret = malloc(str - buffer + 1);
                memcpy(ret, buffer, str - buffer + 1);
                free(buffer);
                return ret;
            } else {
                return strdup("nil");
            }
        }
        case TYPE_SOL_TOKEN:
            return strdup(((SolToken) obj)->identifier);
    }
}

SolObject sol_obj_get_prop(SolObject obj, char* token) {
    // first check properties for token
    TokenPoolEntry resolved_token;
    HASH_FIND_STR(obj->properties, token, resolved_token);
    if (resolved_token != NULL) {
        return sol_obj_retain(resolved_token->binding->value);
    }
    
    // loop through prototypes to find the token
    SolObject current_obj = obj;
    do {
        TokenPoolEntry resolved_token;
        HASH_FIND_STR(current_obj->prototype, token, resolved_token);
        if (resolved_token != NULL) {
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
        new_token = malloc(sizeof(token_pool_entry));
        new_token->identifier = strdup(token);
        new_token->binding = malloc(sizeof(*new_token->binding));
        HASH_ADD_KEYPTR(hh, obj->properties, new_token->identifier, strlen(new_token->identifier), new_token);
    } else {
        sol_obj_release(new_token->binding->value);
    }
    new_token->binding->value = value;
    new_token->binding->retain_count = 1;
}

SolObject sol_obj_get_proto(SolObject obj, char* token) {
    TokenPoolEntry resolved_token;
    HASH_FIND_STR(obj->prototype, token, resolved_token);
    if (resolved_token != NULL) {
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
        new_token = malloc(sizeof(token_pool_entry));
        new_token->identifier = strdup(token);
        new_token->binding = malloc(sizeof(*new_token->binding));
        HASH_ADD_KEYPTR(hh, obj->prototype, new_token->identifier, strlen(new_token->identifier), new_token);
    } else {
        sol_obj_release(new_token->binding->value);
    }
    new_token->binding->value = value;
    new_token->binding->retain_count = 1;
}
