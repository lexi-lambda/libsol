
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "solop.h"
#include "soltypes.h"
#include "soltoken.h"
#include "solfunc.h"
#include "solar.h"
#include "solevent.h"

SolOperator sol_operator_create(SolOperatorRef operator_ref) {
    return (SolOperator) sol_obj_clone_type((SolObject) Function, &(struct sol_operator_raw){
            true,
            operator_ref
        }, sizeof(sol_operator));
}

#define DEFINEOP(opname) \
    SolObject perform_OP_ ## opname (SolList arguments, SolObject self); \
    const SolOperatorRef OP_ ## opname = &perform_OP_ ## opname; \
    SolObject perform_OP_ ## opname (SolList arguments, SolObject self)

DEFINEOP(ADD) {
    SolNumber result = (SolNumber) sol_obj_retain((SolObject) sol_num_create(0));
    SOL_LIST_ITR_BEGIN(arguments)
        SolNumber num = (SolNumber) arguments->current->value;
        result->value += num->value;
    SOL_LIST_ITR_END(arguments)
    return (SolObject) result;
}

DEFINEOP(SUBTRACT) {
    SolNumber result = (SolNumber) sol_obj_retain((SolObject) sol_num_create(((SolNumber) arguments->first->value)->value));
    SolList minusArguments = sol_list_slice_s(arguments, 1);
    SOL_LIST_ITR_BEGIN(minusArguments)
        SolNumber num = (SolNumber) minusArguments->current->value;
        result->value -= num->value;
    SOL_LIST_ITR_END(minusArguments)
    sol_obj_release((SolObject) minusArguments);
    return (SolObject) result;
}

DEFINEOP(MULTIPLY) {
    SolNumber result = (SolNumber) sol_obj_retain((SolObject) sol_num_create(1));
    SOL_LIST_ITR_BEGIN(arguments)
        SolNumber num = (SolNumber) arguments->current->value;
        result->value *= num->value;
    SOL_LIST_ITR_END(arguments)
    return (SolObject) result;
}

DEFINEOP(DIVIDE) {
    SolNumber result = (SolNumber) sol_obj_retain((SolObject) sol_num_create(((SolNumber) arguments->first->value)->value));
    SolList minusArguments = sol_list_slice_s(arguments, 1);
    SOL_LIST_ITR_BEGIN(minusArguments)
        SolNumber num = (SolNumber) minusArguments->current->value;
        result->value /= num->value;
    SOL_LIST_ITR_END(minusArguments)
    sol_obj_release((SolObject) minusArguments);
    return (SolObject) result;
}

DEFINEOP(MOD) {
    SolNumber result = (SolNumber) sol_obj_retain((SolObject) sol_num_create(((SolNumber) arguments->first->value)->value));
    SolList minusArguments = sol_list_slice_s(arguments, 1);
    SOL_LIST_ITR_BEGIN(minusArguments)
        SolNumber num = (SolNumber) minusArguments->current->value;
        result->value = fmod(result->value, num->value);
    SOL_LIST_ITR_END(minusArguments)
    sol_obj_release((SolObject) minusArguments);
    return (SolObject) result;
}

DEFINEOP(REQUIRE) {
    SOL_LIST_ITR_BEGIN(arguments)
        SolString string = (SolString) sol_obj_retain(arguments->current->value);
        solar_load(string->value);
        sol_obj_release((SolObject) string);
    SOL_LIST_ITR_END(arguments)
    return nil;
}

DEFINEOP(EXIT) {
    if (arguments->length > 0)
        exit((int) ((SolNumber) arguments->first->value)->value);
    exit(0);
}

DEFINEOP(BIND) {
    SolToken token = (SolToken) arguments->first->value;
    SolObject evaluated = arguments->length > 1 ? sol_obj_retain(arguments->first->next->value) : nil;
    SolObject result = sol_obj_retain(sol_token_register(token->identifier, evaluated));
    sol_obj_release(evaluated);
    return result;
}

DEFINEOP(BOUND) {
    SolObject resolved = sol_token_resolve(((SolToken) arguments->first->value)->identifier);
    SolObject result = sol_obj_retain((SolObject) sol_bool_create(resolved != NULL));
    sol_obj_release(resolved);
    return result;
}

DEFINEOP(SET) {
    SolToken token = (SolToken) arguments->first->value;
    SolObject evaluated = (arguments->first->next->value->type_id == TYPE_SOL_TOKEN ? sol_obj_evaluate : sol_obj_retain)(arguments->first->next->value);
    SolObject result = sol_obj_retain(sol_token_update(token->identifier, evaluated));
    sol_obj_release(evaluated);
    return result;
}

DEFINEOP(DEFINE) {
    SolToken token = (SolToken) arguments->first->value;
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
            if (list->first->value->type_id == TYPE_SOL_TOKEN) {
                SolObject first = sol_obj_evaluate(list->first->value);
                if (first->type_id == TYPE_SOL_FUNC && ((SolFunction) first)->is_operator && ((SolOperator) first)->operator_ref == OP_FREEZE) {
                    SolObject frozen = sol_obj_evaluate(obj);
                    SolObjectFrozen ret = (SolObjectFrozen) sol_obj_retain((SolObject) sol_obj_freeze(frozen));
                    sol_obj_release(frozen);
                    return (SolObject) ret;
                }
            }
            return sol_obj_retain(obj);
        }
        case TYPE_SOL_TOKEN:
            return sol_obj_retain(obj);
        default:
            return sol_obj_retain((SolObject) sol_obj_freeze(arguments->first->value));
            break;
    }
}

DEFINEOP(LAMBDA) {
    SolList parameters = (SolList) arguments->first->value;
    SolList statements = sol_list_slice_s(arguments, 1);
    SolFunction func = (SolFunction) sol_obj_retain((SolObject) sol_func_create(parameters, statements));
    sol_obj_release((SolObject) statements);
    return (SolObject) func;
}

DEFINEOP(TO_TOKEN) {
    return sol_obj_retain((SolObject) sol_token_create(((SolString) arguments->first->value)->value));
}

DEFINEOP(PRINT) {
    SolObject argument = sol_obj_retain(arguments->first->value);
    char* string = (argument && argument->type_id == TYPE_SOL_DATATYPE && ((SolDatatype) argument)->type_id == DATA_TYPE_STR) ? strdup(((SolString) argument)->value) : sol_obj_to_string(argument);
    printf("%s\n", string);
    free(string);
    return argument;
}

DEFINEOP(NOT) {
    return sol_obj_retain((SolObject) (sol_bool_value_of(arguments->first->value)->value ? sol_bool_create(0) : sol_bool_create(1)));
}

DEFINEOP(AND) {
    SolObject evaluated = nil;
    SOL_LIST_ITR_BEGIN(arguments)
        sol_obj_release(evaluated);
        evaluated = sol_obj_evaluate(arguments->current->value);
        SolBoolean value = sol_bool_value_of(evaluated);
        if (!value->value) {
            sol_obj_release((SolObject) value);
            return evaluated;
        }
        sol_obj_release((SolObject) value);
    SOL_LIST_ITR_END(arguments)
    return sol_obj_retain(evaluated);
}

DEFINEOP(OR) {
    SolObject evaluated = nil;
    SOL_LIST_ITR_BEGIN(arguments)
        sol_obj_release(evaluated);
        evaluated = sol_obj_evaluate(arguments->current->value);
        SolBoolean value = sol_bool_value_of(evaluated);
        if (value->value) {
            sol_obj_release((SolObject) value);
            return evaluated;
        }
        sol_obj_release((SolObject) value);
    SOL_LIST_ITR_END(arguments)
    return sol_obj_retain(evaluated);
}

DEFINEOP(EQUALITY) {
    SOL_LIST_ITR_BEGIN(arguments)
        if (!sol_obj_equals(arguments->first->value, arguments->first->next->value)) return sol_obj_retain((SolObject) sol_bool_create(false));
    SOL_LIST_ITR_END(arguments)
    return sol_obj_retain((SolObject) sol_bool_create(true));
}

DEFINEOP(LESS_THAN) {
    return sol_obj_retain((SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value < ((SolNumber) arguments->first->next->value)->value));
}

DEFINEOP(GREATER_THAN) {
    return sol_obj_retain((SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value > ((SolNumber) arguments->first->next->value)->value));
}

DEFINEOP(LESS_THAN_EQUALITY) {
    return sol_obj_retain((SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value <= ((SolNumber) arguments->first->next->value)->value));
}

DEFINEOP(GREATER_THAN_EQUALITY) {
    return sol_obj_retain((SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value >= ((SolNumber) arguments->first->next->value)->value));
}

DEFINEOP(CONDITIONAL) {
    SolBoolean condition_object = sol_bool_value_of(arguments->first->value);
    bool condition = condition_object->value;
    sol_obj_release((SolObject) condition_object);
    if (condition) {
        return sol_obj_retain(arguments->first->next->value);
    } else if (arguments->first->next->next != NULL) {
        return sol_obj_retain(arguments->first->next->next->value);
    } else {
        return nil;
    }
}

DEFINEOP(IF) {
    SolBoolean condition_object = sol_bool_value_of(arguments->first->value);
    bool condition = condition_object->value;
    sol_obj_release((SolObject) condition_object);
    if (condition) {
        return sol_func_execute((SolFunction) arguments->first->next->value, (SolList) nil, nil);
    } else if (arguments->first->next->next != NULL) {
        return sol_func_execute((SolFunction) arguments->first->next->next->value, (SolList) nil, nil);
    } else {
        return nil;
    }
}

DEFINEOP(LOOP) {
    SolObject result = nil;
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
    SOL_LIST_ITR_BEGIN(arguments)
        char* string_value = (arguments->current->value && arguments->current->value->type_id == TYPE_SOL_DATATYPE && ((SolDatatype) arguments->current->value)->type_id == DATA_TYPE_STR) ? strdup(((SolString) arguments->current->value)->value) : sol_obj_to_string(arguments->current->value);
        sol_list_add_obj(strings, (SolObject) sol_string_create(string_value));
        len += strlen(string_value);
        free(string_value);
    SOL_LIST_ITR_END(arguments)
    char* result = malloc(len);
    char* pos = result;
    SOL_LIST_ITR_BEGIN(strings)
        char* arg = ((SolString) strings->current->value)->value;
        while (*arg != '\0') {
            *pos = *arg;
            arg++;
            pos++;
        }
    SOL_LIST_ITR_END(strings)
    *pos = '\0';
    sol_obj_release((SolObject) strings);
    SolObject result_object = sol_obj_retain((SolObject) sol_string_create(result));
    free(result);
    return result_object;
}

DEFINEOP(OBJECT_GET) {
    return sol_obj_get_prop(self, ((SolToken) arguments->first->value)->identifier);
}

DEFINEOP(OBJECT_SET) {
    SolObject result = (arguments->first->next->value->type_id == TYPE_SOL_TOKEN ? sol_obj_evaluate : sol_obj_retain)(arguments->first->next->value);
    sol_obj_set_prop(self, ((SolToken) arguments->first->value)->identifier, result);
    return result;
}

DEFINEOP(OBJECT_GET_METADATA) {
    char* identifier = ((SolToken) arguments->first->next->value)->identifier;
    if (!strcmp(identifier, "get")) {
        return sol_obj_get_prop_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_GET);
    } else if (!strcmp(identifier, "set")) {
        return sol_obj_get_prop_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_SET);
    } else {
        fprintf(stderr, "ERROR: Illegal property metadata '%s'.\n", identifier);
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
        fprintf(stderr, "ERROR: Illegal property metadata '%s'.\n", identifier);
    }
    return nil;
}

DEFINEOP(PROTOTYPE_GET) {
    return sol_obj_get_proto(self, ((SolToken) arguments->first->value)->identifier);
}

DEFINEOP(PROTOTYPE_SET) {
    SolObject result = (arguments->first->next->value->type_id == TYPE_SOL_TOKEN ? sol_obj_evaluate : sol_obj_retain)(arguments->first->next->value);
    sol_obj_set_proto(self, ((SolToken) arguments->first->value)->identifier, result);
    return result;
}

DEFINEOP(PROTOTYPE_GET_METADATA) {
    char* identifier = ((SolToken) arguments->first->next->value)->identifier;
    if (!strcmp(identifier, "get")) {
        return sol_obj_get_proto_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_GET);
    } else if (!strcmp(identifier, "set")) {
        return sol_obj_get_proto_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_SET);
    } else {
        fprintf(stderr, "ERROR: Illegal property metadata '%s'.\n", identifier);
    }
    return nil;
}

DEFINEOP(PROTOTYPE_SET_METADATA) {
    char* identifier = ((SolToken) arguments->first->next->value)->identifier;
    if (!strcmp(identifier, "get")) {
        sol_obj_set_proto_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_GET, arguments->first->next->next->value);
    } else if (!strcmp(identifier, "set")) {
        sol_obj_set_proto_metadata(self, ((SolToken) arguments->first->value)->identifier, METADATA_SET, arguments->first->next->next->value);
    } else {
        fprintf(stderr, "ERROR: Illegal property metadata '%s'.\n", identifier);
    }
    return nil;
}

DEFINEOP(OBJECT_CLONE) {
    return sol_obj_clone(self);
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
DEFINEOP(OBJECT_TO_STRING) {
    int freeze_count = 0;
    while (self->type_id == TYPE_SOL_OBJ_FROZEN) {
        freeze_count++;
        self = ((SolObjectFrozen) self)->value;
    }
    switch (self->type_id) {
        case TYPE_SOL_OBJ: {
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
        case TYPE_SOL_OBJ_FROZEN:
            fprintf(stderr, "fatal error: impossible case reached in sol_obj_to_string\n");
            exit(EXIT_FAILURE);
    }
}

DEFINEOP(OBJECT_LISTEN) {
    sol_event_listener_add(self, ((SolString) arguments->first->value)->value, (SolFunction) arguments->first->next->value);
    return nil;
}

struct event_callback_data {
    SolObject object;
    SolEvent event;
};
void sol_event_callback(evutil_socket_t fd, short flags, void* arg) {
    struct event_callback_data* data = arg;
    SolString type = (SolString) sol_obj_get_prop(data->event, "type");
    sol_event_listener_dispatch(data->object, type->value, data->event);
    sol_obj_release((SolObject) type);
    if ((flags & EV_PERSIST) == 0) {
        sol_obj_release(data->object);
        sol_obj_release(data->event);
        free(data);
    }
}
DEFINEOP(OBJECT_DISPATCH) {
    struct sol_event event;
    event.fd = -1;
    event.flags = EV_TIMEOUT;
    event.callback = sol_event_callback;
    event.timeout = NULL;
    
    struct event_callback_data* data = malloc(sizeof(*data));
    data->object = sol_obj_retain(self);
    data->event = sol_obj_retain(arguments->first->value);
    event.arg = data;
    
    sol_event_loop_add_once(&event);
    return nil;
}
