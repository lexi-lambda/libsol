
#include <string.h>
#include <math.h>
#include <uv.h>
#include "solop.h"
#include "soltypes.h"
#include "soltoken.h"
#include "solar.h"
#include "solerror.h"
#include "solevent.h"

SolOperator sol_operator_create(SolOperatorRef operator_ref) {
    return (SolOperator) sol_obj_clone_type((SolObject) Function, &(struct sol_operator_raw){
            FUNC_TYPE_OPERATOR,
            operator_ref
        }, sizeof(sol_operator));
}

#define DEFINEOP(opname) \
    SolObject perform_OP_ ## opname (SolList arguments, SolObject self); \
    const SolOperatorRef OP_ ## opname = &perform_OP_ ## opname; \
    SolObject perform_OP_ ## opname (SolList arguments, SolObject self)

DEFINEOP(ADD) {
    SolNumber result = (SolNumber) sol_obj_retain((SolObject) sol_num_create(0));
    SOL_LIST_ITR(arguments, current, i) {
        SolNumber num = (SolNumber) current->value;
        SOL_REQUIRE_DATATYPE(num, DATA_TYPE_NUM) {
            sol_obj_release((SolObject) result);
        }
        result->value += num->value;
    }
    return (SolObject) result;
}

DEFINEOP(SUBTRACT) {
    SolNumber result = (SolNumber) sol_obj_retain((SolObject) sol_num_create(((SolNumber) arguments->first->value)->value));
    SolList minusArguments = sol_list_slice_s(arguments, 1);
    SOL_LIST_ITR(minusArguments, current, i) {
        SolNumber num = (SolNumber) current->value;
        SOL_REQUIRE_DATATYPE(num, DATA_TYPE_NUM) {
            sol_obj_release((SolObject) result);
        }
        result->value -= num->value;
    }
    sol_obj_release((SolObject) minusArguments);
    return (SolObject) result;
}

DEFINEOP(MULTIPLY) {
    SolNumber result = (SolNumber) sol_obj_retain((SolObject) sol_num_create(1));
    SOL_LIST_ITR(arguments, current, i) {
        SolNumber num = (SolNumber) current->value;
        SOL_REQUIRE_DATATYPE(num, DATA_TYPE_NUM) {
            sol_obj_release((SolObject) result);
        }
        result->value *= num->value;
    }
    return (SolObject) result;
}

DEFINEOP(DIVIDE) {
    SolNumber result = (SolNumber) sol_obj_retain((SolObject) sol_num_create(((SolNumber) arguments->first->value)->value));
    SolList minusArguments = sol_list_slice_s(arguments, 1);
    SOL_LIST_ITR(minusArguments, current, i) {
        SolNumber num = (SolNumber) current->value;
        SOL_REQUIRE_DATATYPE(num, DATA_TYPE_NUM) {
            sol_obj_release((SolObject) result);
        }
        result->value /= num->value;
    }
    sol_obj_release((SolObject) minusArguments);
    return (SolObject) result;
}

DEFINEOP(MOD) {
    SolNumber result = (SolNumber) sol_obj_retain((SolObject) sol_num_create(((SolNumber) arguments->first->value)->value));
    SolList minusArguments = sol_list_slice_s(arguments, 1);
    SOL_LIST_ITR(minusArguments, current, i) {
        SolNumber num = (SolNumber) current->value;
        SOL_REQUIRE_DATATYPE(num, DATA_TYPE_NUM) {
            sol_obj_release((SolObject) result);
        }
        result->value = fmod(result->value, num->value);
    }
    sol_obj_release((SolObject) minusArguments);
    return (SolObject) result;
}

DEFINEOP(REQUIRE) {
    SolObject ret = nil;
    SOL_LIST_ITR(arguments, current, i) {
        sol_obj_release(ret);
        SOL_REQUIRE_DATATYPE(current->value, DATA_TYPE_STR);
        SolString string = (SolString) sol_obj_retain(current->value);
        ret = solar_load(string->value);
        sol_obj_release((SolObject) string);
    }
    return ret;
}

DEFINEOP(EXIT) {
    if (arguments->length > 0) {
        SOL_REQUIRE_DATATYPE(arguments->first->value, DATA_TYPE_NUM);
        exit((int) ((SolNumber) arguments->first->value)->value);
    }
    exit(0);
}

DEFINEOP(BIND) {
    SolToken token = (SolToken) arguments->first->value;
    SOL_REQUIRE_TYPE(token, TYPE_SOL_TOKEN);
    SolObject evaluated = arguments->length > 1 ? sol_obj_retain(arguments->first->next->value) : nil;
    SolObject result = sol_obj_retain(sol_token_register(token->identifier, evaluated));
    sol_obj_release(evaluated);
    return result;
}

DEFINEOP(BOUND) {
    SOL_REQUIRE_TYPE(arguments->first->value, TYPE_SOL_TOKEN);
    SolObject resolved = sol_token_resolve(((SolToken) arguments->first->value)->identifier);
    SolObject result = sol_obj_retain((SolObject) sol_bool_create(resolved != NULL));
    sol_obj_release(resolved);
    return result;
}

DEFINEOP(SET) {
    SolToken token = (SolToken) arguments->first->value;
    SOL_REQUIRE_TYPE(token, TYPE_SOL_TOKEN);
    SolObject result = sol_obj_retain(sol_token_update(token->identifier, arguments->first->next->value));
    return result;
}

DEFINEOP(DEFINE) {
    SolToken token = (SolToken) arguments->first->value;
    SOL_REQUIRE_TYPE(token, TYPE_SOL_TOKEN);
    if (sol_token_resolve(token->identifier) == NULL)
        sol_token_register(token->identifier, nil);
    SolObject result = sol_obj_retain(arguments->first->next->value);
    sol_token_update(token->identifier, result);
    return result;
}

DEFINEOP(EVALUATE) {
    return sol_obj_evaluate(arguments->first->value);
}

DEFINEOP(FREEZE) {
    SolObject obj = arguments->first->value;
    switch (obj->type_id) {
        case TYPE_SOL_LIST: {
            SolList list = (SolList) obj;
            SolList result = (SolList) sol_obj_retain((SolObject) sol_list_create(list->object_mode));
            SOL_LIST_ITR(list, current, i) {
                SolObject evaluated = sol_obj_evaluate(current->value);
                sol_list_add_obj(result, evaluated);
                sol_obj_release(evaluated);
            }
            return (SolObject) result;
        }
        default:
            return sol_obj_retain(obj);
    }
}

DEFINEOP(LAMBDA) {
    SolList parameters = (SolList) sol_obj_evaluate((SolObject) arguments->first->value);
    SOL_REQUIRE_TYPE(parameters, TYPE_SOL_LIST);
    SolList statements = sol_list_slice_s(arguments, 1);
    SolFunction func = (SolFunction) sol_obj_retain((SolObject) sol_func_create(parameters, statements));
    sol_obj_release((SolObject) statements);
    return (SolObject) func;
}

DEFINEOP(MACRO) {
    SolList parameters = (SolList) sol_obj_evaluate((SolObject) arguments->first->value);
    SOL_REQUIRE_TYPE(parameters, TYPE_SOL_LIST);
    SolList statements = sol_list_slice_s(arguments, 1);
    SolFunction func = (SolFunction) sol_obj_retain((SolObject) sol_macro_create(parameters, statements));
    sol_obj_release((SolObject) statements);
    return (SolObject) func;
}

DEFINEOP(WRAP) {
    SolObject ret = sol_obj_clone(RawObject);
    SOL_LIST_ITR(arguments, current_argument, i) {
        SolObject values = current_argument->value;
        if (values->type_id == TYPE_SOL_LIST) {
            SolList keys = (SolList) values;
            SOL_LIST_ITR(keys, current_key, i) {
                SolToken key = (SolToken) current_key->value;
                SOL_REQUIRE_TYPE(key, TYPE_SOL_TOKEN) {
                    sol_obj_release(ret);
                }
                SolObject value = sol_token_resolve(key->identifier);
                sol_obj_set_prop(ret, key->identifier, value);
                sol_obj_release(value);
            }
        } else {
            TokenPoolEntry current_token, tmp;
            HASH_ITER(hh, values->properties, current_token, tmp) {
                SolToken key = (SolToken) current_token->binding->value;
                SOL_REQUIRE_TYPE(key, TYPE_SOL_TOKEN) {
                    sol_obj_release(ret);
                }
                SolObject value = sol_token_resolve(key->identifier);
                sol_obj_set_prop(ret, current_token->identifier, value);
                sol_obj_release(value);
            }
        }
    }
    return ret;
}

DEFINEOP(UNWRAP) {
    SolObject obj = arguments->first->value;
    SolList keys_list = sol_list_slice_s(arguments, 1);
    SOL_LIST_ITR(keys_list, current_key_list, i) {
        SolObject values = current_key_list->value;
        if (values->type_id == TYPE_SOL_LIST) {
            SolList keys = (SolList) values;
            SOL_LIST_ITR(keys, current_key, i) {
                SolToken key = (SolToken) current_key->value;
                SOL_REQUIRE_TYPE(key, TYPE_SOL_TOKEN) {
                    sol_obj_release((SolObject) keys_list);
                }
                SolObject value = sol_obj_get_prop(obj, key->identifier);
                sol_token_register(key->identifier, value);
                sol_obj_release(value);
            }
        } else {
            TokenPoolEntry current_token, tmp;
            HASH_ITER(hh, values->properties, current_token, tmp) {
                SolToken key = (SolToken) current_token->binding->value;
                SOL_REQUIRE_TYPE(key, TYPE_SOL_TOKEN) {
                    sol_obj_release((SolObject) keys_list);
                }
                SolObject value = sol_obj_get_prop(obj, current_token->identifier);
                sol_token_register(key->identifier, value);
                sol_obj_release(value);
            }
        }
    }
    sol_obj_release((SolObject) keys_list);
    return nil;
}

DEFINEOP(PRINT) {
    SolString value = (SolString) OP_CAT(arguments, nil);
    char* string = value->value;
    printf("%s\n", string);
    return (SolObject) value;
}

DEFINEOP(NOT) {
    return sol_obj_retain((SolObject) (sol_bool_value_of(arguments->first->value)->value ? sol_bool_create(0) : sol_bool_create(1)));
}

DEFINEOP(AND) {
    SolObject evaluated = nil;
    SOL_LIST_ITR(arguments, current, i) {
        sol_obj_release(evaluated);
        evaluated = sol_obj_evaluate(current->value);
        SolBoolean value = sol_bool_value_of(evaluated);
        if (!value->value) {
            sol_obj_release((SolObject) value);
            return evaluated;
        }
        sol_obj_release((SolObject) value);
    }
    return sol_obj_retain(evaluated);
}

DEFINEOP(OR) {
    SolObject evaluated = nil;
    SOL_LIST_ITR(arguments, current, i) {
        sol_obj_release(evaluated);
        evaluated = sol_obj_evaluate(current->value);
        SolBoolean value = sol_bool_value_of(evaluated);
        if (value->value) {
            sol_obj_release((SolObject) value);
            return evaluated;
        }
        sol_obj_release((SolObject) value);
    }
    return sol_obj_retain(evaluated);
}

DEFINEOP(EQUALITY) {
    SOL_LIST_ITR(arguments, current, i) {
        if (i >= arguments->length - 1) break;
        if (!sol_obj_equals(current->value, current->next->value)) return sol_obj_retain((SolObject) sol_bool_create(false));
    }
    return sol_obj_retain((SolObject) sol_bool_create(true));
}

DEFINEOP(LESS_THAN) {
    SOL_REQUIRE_DATATYPE(arguments->first->value, DATA_TYPE_NUM);
    SOL_REQUIRE_DATATYPE(arguments->first->next->value, DATA_TYPE_NUM);
    return sol_obj_retain((SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value < ((SolNumber) arguments->first->next->value)->value));
}

DEFINEOP(GREATER_THAN) {
    SOL_REQUIRE_DATATYPE(arguments->first->value, DATA_TYPE_NUM);
    SOL_REQUIRE_DATATYPE(arguments->first->next->value, DATA_TYPE_NUM);
    return sol_obj_retain((SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value > ((SolNumber) arguments->first->next->value)->value));
}

DEFINEOP(LESS_THAN_EQUALITY) {
    SOL_REQUIRE_DATATYPE(arguments->first->value, DATA_TYPE_NUM);
    SOL_REQUIRE_DATATYPE(arguments->first->next->value, DATA_TYPE_NUM);
    return sol_obj_retain((SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value <= ((SolNumber) arguments->first->next->value)->value));
}

DEFINEOP(GREATER_THAN_EQUALITY) {
    SOL_REQUIRE_DATATYPE(arguments->first->value, DATA_TYPE_NUM);
    SOL_REQUIRE_DATATYPE(arguments->first->next->value, DATA_TYPE_NUM);
    return sol_obj_retain((SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value >= ((SolNumber) arguments->first->next->value)->value));
}

DEFINEOP(CONDITIONAL) {
    SolObject evaluated = sol_obj_evaluate(arguments->first->value);
    SolBoolean condition_object = sol_bool_value_of(evaluated);
    bool condition = condition_object->value;
    sol_obj_release((SolObject) condition_object);
    if (condition) {
        if (arguments->length > 2) {
            sol_obj_release(evaluated);
            return sol_obj_evaluate(arguments->first->next->value);
        } else {
            return evaluated;
        }
    } else {
        sol_obj_release(evaluated);
        if (arguments->length > 2) {
            return sol_obj_evaluate(arguments->first->next->next->value);
        } else {
            return sol_obj_evaluate(arguments->first->next->value);
        }
    }
}

DEFINEOP(LOOP) {
    SolObject result = nil;
    SOL_REQUIRE_TYPE(arguments->first->value, TYPE_SOL_FUNC);
    SOL_REQUIRE_TYPE(arguments->first->next->value, TYPE_SOL_FUNC);
    SolFunction condition_function = (SolFunction) arguments->first->value;
    SolFunction body_function = (SolFunction) arguments->first->next->value;
    for (SolObject condition; sol_bool_value_of(condition = sol_func_execute(condition_function, (SolList) nil, nil))->value; sol_obj_release(condition)) {
        sol_obj_release(result);
        result = sol_func_execute(body_function, (SolList) nil, nil);
    }
    return result;
}

DEFINEOP(CAT) {
    int len = 1;
    SolList strings = (SolList) sol_obj_retain((SolObject) sol_list_create(false));
    SOL_LIST_ITR(arguments, current, i) {
        char* string_value = (current->value && current->value->type_id == TYPE_SOL_DATATYPE && ((SolDatatype) current->value)->type_id == DATA_TYPE_STR) ? strdup(((SolString) current->value)->value) : sol_obj_to_string(current->value);
        sol_list_add_obj(strings, (SolObject) sol_string_create(string_value));
        len += strlen(string_value);
        free(string_value);
    }
    char* result = malloc(len);
    char* pos = result;
    SOL_LIST_ITR(strings, current, i) {
        char* arg = ((SolString) current->value)->value;
        while (*arg != '\0') {
            *pos = *arg;
            arg++;
            pos++;
        }
    }
    *pos = '\0';
    sol_obj_release((SolObject) strings);
    SolObject result_object = sol_obj_retain((SolObject) sol_string_create(result));
    free(result);
    return result_object;
}

DEFINEOP(OBJECT_GET) {
    SOL_REQUIRE_TYPE(arguments->first->value, TYPE_SOL_TOKEN);
    return sol_obj_get_prop(self, ((SolToken) arguments->first->value)->identifier);
}

DEFINEOP(OBJECT_SET) {
    SOL_REQUIRE_TYPE(arguments->first->value, TYPE_SOL_TOKEN);
    SolObject result = (arguments->first->next->value->type_id == TYPE_SOL_TOKEN ? sol_obj_evaluate : sol_obj_retain)(arguments->first->next->value);
    sol_obj_set_prop(self, ((SolToken) arguments->first->value)->identifier, result);
    return result;
}

DEFINEOP(OBJECT_GET_METADATA) {
    SOL_REQUIRE_TYPE(arguments->first->value, TYPE_SOL_TOKEN);
    SOL_REQUIRE_TYPE(arguments->first->next->value, TYPE_SOL_TOKEN);
    char* identifier = ((SolToken) arguments->first->next->value)->identifier;
    if (!strcmp(identifier, "get")) {
        return sol_obj_get_prop_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_GET);
    } else if (!strcmp(identifier, "set")) {
        return sol_obj_get_prop_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_SET);
    } else {
        throw_msg (Error, "illegal property metadata '%s'", identifier);
    }
    return nil;
}

DEFINEOP(OBJECT_SET_METADATA) {
    char* identifier = ((SolToken) arguments->first->next->value)->identifier;
    if (!strcmp(identifier, "get")) {
        sol_obj_set_prop_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_GET, arguments->first->next->next->value);
    } else if (!strcmp(identifier, "set")) {
        sol_obj_set_prop_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_SET, arguments->first->next->next->value);
    } else {
        throw_msg (Error, "illegal property metadata '%s'", identifier);
    }
    return nil;
}

DEFINEOP(PROTOTYPE_GET) {
    SOL_REQUIRE_TYPE(arguments->first->value, TYPE_SOL_TOKEN);
    return sol_obj_get_proto(self, ((SolToken) arguments->first->value)->identifier);
}

DEFINEOP(PROTOTYPE_SET) {
    SOL_REQUIRE_TYPE(arguments->first->value, TYPE_SOL_TOKEN);
    SolObject result = (arguments->first->next->value->type_id == TYPE_SOL_TOKEN ? sol_obj_evaluate : sol_obj_retain)(arguments->first->next->value);
    sol_obj_set_proto(self, ((SolToken) arguments->first->value)->identifier, result);
    return result;
}

DEFINEOP(PROTOTYPE_GET_METADATA) {
    SOL_REQUIRE_TYPE(arguments->first->value, TYPE_SOL_TOKEN);
    SOL_REQUIRE_TYPE(arguments->first->next->value, TYPE_SOL_TOKEN);
    char* identifier = ((SolToken) arguments->first->next->value)->identifier;
    if (!strcmp(identifier, "get")) {
        return sol_obj_get_proto_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_GET);
    } else if (!strcmp(identifier, "set")) {
        return sol_obj_get_proto_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_SET);
    } else {
        throw_msg (Error, "illegal property metadata '%s'", identifier);
    }
    return nil;
}

DEFINEOP(PROTOTYPE_SET_METADATA) {
    SOL_REQUIRE_TYPE(arguments->first->value, TYPE_SOL_TOKEN);
    SOL_REQUIRE_TYPE(arguments->first->next->value, TYPE_SOL_TOKEN);
    char* identifier = ((SolToken) arguments->first->next->value)->identifier;
    if (!strcmp(identifier, "get")) {
        sol_obj_set_proto_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_GET, arguments->first->next->next->value);
    } else if (!strcmp(identifier, "set")) {
        sol_obj_set_proto_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_SET, arguments->first->next->next->value);
    } else {
        throw_msg (Error, "illegal property metadata '%s'", identifier);
    }
    return nil;
}

DEFINEOP(OBJECT_CREATE) {
    SolObject ret = (SolObject) sol_obj_create_raw();
    SOL_LIST_ITR(arguments, current, i) {
        SolToken token = (SolToken) current->value;
        SOL_REQUIRE_TYPE(token, TYPE_SOL_TOKEN);
        SolObject value = current->next->value;
        sol_obj_set_prop(ret, token->identifier, value);
        current = current->next;
    }
    return ret;
}

DEFINEOP(OBJECT_CLONE) {
    SolObject ret = sol_obj_clone(self);
    if (arguments->first)
        sol_obj_patch(ret, arguments->first->value);
    return ret;
}

DEFINEOP(OBJECT_PATCH) {
    SolObject patch = arguments->first->value;
    
    bool proto = false;
    if (arguments->length > 1) {
        SolBoolean proto_obj = sol_bool_value_of(arguments->first->next->value);
        proto = proto_obj->value;
        sol_obj_release((SolObject) proto_obj);
    }
    
    TokenPoolEntry current_token, tmp;
    HASH_ITER(hh, patch->properties, current_token, tmp) {
        sol_obj_set_prop(self, current_token->identifier, current_token->binding->value);
    }
    if (proto) {
        HASH_ITER(hh, patch->prototype, current_token, tmp) {
            sol_obj_set_proto(self, current_token->identifier, current_token->binding->value);
        }
    }
    
    return sol_obj_retain(self);
}

DEFINEOP(OBJECT_TO_STRING) {
    char* value = sol_obj_inspect(self);
    SolObject ret = sol_obj_retain((SolObject) sol_string_create(value));
    free(value);
    return ret;
}

#define strbuild(buff, offset, buff_size, out, format, ...) \
  do {                                                      \
    snprintf(NULL, 0, format "%n", ##__VA_ARGS__, &out);    \
    while (offset + out >= buff_size) {                     \
        buff_size *= 2;                                     \
        buff = realloc(buff, buff_size);                    \
    }                                                       \
    sprintf(buff + offset, format, ##__VA_ARGS__);          \
    offset += out;                                          \
  } while (0);
static int sol_obj_indent_level = 0;
DEFINEOP(OBJECT_INSPECT) {
    int freeze_count = 0;
    switch (self->type_id) {
        case TYPE_SOL_OBJ:
        case TYPE_SOL_OBJ_NATIVE: {
            size_t buff_size = 128;
            char* buff = malloc(buff_size);
            off_t buff_offset = 0;
            int buff_tmp;
            if (self->parent == RawObject) {
                strbuild(buff, buff_offset, buff_size, buff_tmp, "{\n");
            } else {
                strbuild(buff, buff_offset, buff_size, buff_tmp, "@{\n");
            }
            sol_obj_indent_level++;
            TokenPoolEntry el, tmp;
            HASH_ITER(hh, self->properties, el, tmp) {
                char* key = el->identifier;
                char* value = sol_obj_inspect(el->binding->value);
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
            SolObject ret_obj = sol_obj_retain((SolObject) sol_string_create(buff));
            free(buff);
            return ret_obj;
        }
        case TYPE_SOL_FUNC:
            return sol_obj_retain((SolObject) sol_string_create("Function"));
        case TYPE_SOL_DATATYPE: {
            SolDatatype datatype = (SolDatatype) self;
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
                    SolObject ret_obj = sol_obj_retain((SolObject) sol_string_create(value));
                    free(value);
                    return ret_obj;
                }
                case DATA_TYPE_STR: {
                    char* ret;
                    asprintf(&ret, "\"%s\"", ((SolString) datatype)->value);
                    SolObject ret_obj = sol_obj_retain((SolObject) sol_string_create(ret));
                    free(ret);
                    return ret_obj;
                }
                case DATA_TYPE_BOOL:
                    return sol_obj_retain((SolObject) sol_string_create(((SolBoolean) datatype)->value ? "true" : "false"));
            }
        }
        case TYPE_SOL_LIST: {
            SolList list = (SolList) self;
            if (list->length > 0) {
                int buffer_len = 512;
                char* buffer = malloc(buffer_len);
                buffer[0] = '\0';
                char* str = buffer;
                
                if (list->object_mode) str += sprintf(str, "@");
                if (freeze_count <= 0) str += sprintf(str, "[");
                else {
                    for (int i = 1; i < freeze_count; i++) {
                        str += sprintf(str, ":");
                    }
                    str += sprintf(str, "(");
                }
                
                SOL_LIST_ITR(list, current, i) {
                    char* obj = sol_obj_inspect(current->value);
                    while (str + strlen(obj) - buffer > buffer_len) {
                        buffer = realloc(buffer, buffer_len *= 2);
                        str = buffer + strlen(buffer);
                    }
                    str += sprintf(str, "%s", obj);
                    free(obj);
                    if (i < list->length - 1) str += sprintf(str, " ");
                }
                
                if (freeze_count <= 0) str += sprintf(str, "]");
                else str += sprintf(str, ")");
                
                char* ret = malloc(str - buffer + 1);
                memcpy(ret, buffer, str - buffer + 1);
                free(buffer);
                SolObject ret_obj = sol_obj_retain((SolObject) sol_string_create(ret));
                free(ret);
                return ret_obj;
            } else {
                return sol_obj_retain((SolObject) sol_string_create("nil"));
            }
        }
        case TYPE_SOL_TOKEN:
            if (freeze_count > 0) {
                SolToken token = (SolToken) self;
                int len = strlen(token->identifier);
                char* ret = malloc(sizeof(char) * (len + freeze_count + 1));
                for (int i = 0; i < freeze_count; i++) {
                    ret[i] = ':';
                }
                memcpy(ret + freeze_count, token->identifier, len + 1);
                SolObject ret_obj = sol_obj_retain((SolObject) sol_string_create(ret));
                free(ret);
                return ret_obj;
            } else {
                return sol_obj_retain((SolObject) sol_string_create(((SolToken) self)->identifier));
            }
    }
}

DEFINEOP(OBJECT_LISTEN) {
    SOL_REQUIRE_DATATYPE(arguments->first->value, DATA_TYPE_STR);
    SOL_REQUIRE_TYPE(arguments->first->next->value, TYPE_SOL_FUNC);
    sol_event_listener_add(self, ((SolString) arguments->first->value)->value, (SolFunction) arguments->first->next->value);
    return nil;
}

struct event_callback_data {
    SolObject object;
    SolEvent event;
};
void sol_event_callback(uv_timer_t* timer_req, int status) {
    struct event_callback_data* data = timer_req->data;
    SolString type = (SolString) sol_obj_get_prop(data->event, "type");
    sol_event_listener_dispatch(data->object, type->value, data->event);
    sol_obj_release((SolObject) type);
    sol_obj_release(data->object);
    sol_obj_release(data->event);
    free(data);
    free(timer_req);
}
DEFINEOP(OBJECT_DISPATCH) {
    struct event_callback_data* data = malloc(sizeof(*data));
    data->object = sol_obj_retain(self);
    data->event = sol_obj_retain(arguments->first->value);
    
    uv_timer_t* timer_req = malloc(sizeof(*timer_req));
    uv_timer_init(uv_default_loop(), timer_req);
    timer_req->data = data;
    uv_timer_start(timer_req, sol_event_callback, 0, 0);
    
    return nil;
}
