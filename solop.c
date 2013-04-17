
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

DEFINEOP(IF) {
    SolBoolean condition_object = sol_bool_value_of(arguments->first->value);
    bool condition = condition_object->value;
    sol_obj_release((SolObject) condition_object);
    if (condition) {
        return sol_func_execute((SolFunction) arguments->first->next->value, (SolList) nil, nil);
    } else if (arguments->first->next->next != NULL) {
        return sol_func_execute((SolFunction) arguments->first->next->next->value, (SolList) nil, nil);
    }
    return nil;
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

DEFINEOP(PROTOTYPE_GET) {
    return sol_obj_get_proto(self, ((SolToken) arguments->first->value)->identifier);
}

DEFINEOP(PROTOTYPE_SET) {
    SolObject result = (arguments->first->next->value->type_id == TYPE_SOL_TOKEN ? sol_obj_evaluate : sol_obj_retain)(arguments->first->next->value);
    sol_obj_set_proto(self, ((SolToken) arguments->first->value)->identifier, result);
    return result;
}

DEFINEOP(OBJECT_CLONE) {
    return sol_obj_clone(self);
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
