
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "solop.h"
#include "soltypes.h"
#include "soltoken.h"
#include "solfunc.h"
#include "solar.h"

SolOperator sol_operator_create(SolOperatorRef operator_ref) {
    return (SolOperator) sol_obj_clone_type((SolObject) Function, &(struct sol_operator_raw){
            true,
            operator_ref
        }, sizeof(sol_operator));
}

#define DEFINEOP(opname, impl) SolObject perform_OP_ ## opname (SolList arguments, SolObject self) impl const SolOperatorRef OP_ ## opname = &perform_OP_ ## opname

DEFINEOP(ADD, {
    SolNumber result = sol_num_create(0);
    SOL_LIST_ITR_BEGIN(arguments)
        SolNumber num = (SolNumber) arguments->current->value;
        result->value += num->value;
    SOL_LIST_ITR_END(arguments)
    return (SolObject) result;
});

DEFINEOP(SUBTRACT, {
    SolNumber result = sol_num_create(((SolNumber) arguments->first->value)->value);
    SolList minusArguments = sol_list_get_sublist_s(arguments, 1);
    SOL_LIST_ITR_BEGIN(minusArguments)
        SolNumber num = (SolNumber) minusArguments->current->value;
        result->value -= num->value;
    SOL_LIST_ITR_END(minusArguments)
    return (SolObject) result;
});

DEFINEOP(MULTIPLY, {
    SolNumber result = sol_num_create(1);
    SOL_LIST_ITR_BEGIN(arguments)
        SolNumber num = (SolNumber) arguments->current->value;
        result->value *= num->value;
    SOL_LIST_ITR_END(arguments)
    return (SolObject) result;
});

DEFINEOP(DIVIDE, {
    SolNumber result = sol_num_create(((SolNumber) arguments->first->value)->value);
    SolList minusArguments = sol_list_get_sublist_s(arguments, 1);
    SOL_LIST_ITR_BEGIN(minusArguments)
        SolNumber num = (SolNumber) minusArguments->current->value;
        result->value /= num->value;
    SOL_LIST_ITR_END(minusArguments)
    return (SolObject) result;
});

DEFINEOP(MOD, {
    SolNumber result = sol_num_create(((SolNumber) arguments->first->value)->value);
    SolList minusArguments = sol_list_get_sublist_s(arguments, 1);
    SOL_LIST_ITR_BEGIN(minusArguments)
        SolNumber num = (SolNumber) minusArguments->current->value;
        result->value = fmod(result->value, num->value);
    SOL_LIST_ITR_END(minusArguments)
    return (SolObject) result;
});

DEFINEOP(REQUIRE, {
    SolString string = (SolString) arguments->first->value;
    solar_load(string->value);
    return nil;
});

DEFINEOP(BIND, {
    SolToken token = (SolToken) arguments->first->value;
    SolObject result = arguments->length > 1 ? arguments->first->next->value : nil;
    sol_token_register(token->identifier, result);
    return result;
});

DEFINEOP(SET, {
    SolToken token = (SolToken) arguments->first->value;
    SolObject result = arguments->first->next->value;
    sol_token_update(token->identifier, result);
    return result;
});

DEFINEOP(EVALUATE, {
    return sol_obj_evaluate(arguments->first->value);
});

DEFINEOP(LAMBDA, {
    SolList parameters = (SolList) arguments->first->value;
    SolList statements = sol_list_get_sublist_s(arguments, 1);
    return (SolObject) sol_func_create(parameters, statements);
});

DEFINEOP(PRINT, {
    SolObject argument = arguments->first->value;
    printf("%s\n", sol_obj_to_string(argument));
    return argument;
});

DEFINEOP(NOT, {
    return (SolObject) (sol_bool_value_of(arguments->first->value)->value ? sol_bool_create(0) : sol_bool_create(1));
});

DEFINEOP(AND, {
    SOL_LIST_ITR_BEGIN(arguments)
        if (!sol_bool_value_of(arguments->current->value)->value) return arguments->current->value;
    SOL_LIST_ITR_END(arguments)
    return (SolObject) sol_bool_create(1);
});

DEFINEOP(OR, {
    SOL_LIST_ITR_BEGIN(arguments)
        if (sol_bool_value_of(arguments->current->value)->value) return arguments->current->value;
    SOL_LIST_ITR_END(arguments)
    return arguments->last->value;
});

DEFINEOP(EQUALITY, {
    SOL_LIST_ITR_BEGIN(arguments)
        if (!sol_obj_equals(arguments->first->value, arguments->first->next->value)) return (SolObject) sol_bool_create(false);
    SOL_LIST_ITR_END(arguments)
    return (SolObject) sol_bool_create(true);
});

DEFINEOP(LESS_THAN, {
    return (SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value < ((SolNumber) arguments->first->next->value)->value);
});

DEFINEOP(GREATER_THAN, {
    return (SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value > ((SolNumber) arguments->first->next->value)->value);
});

DEFINEOP(LESS_THAN_EQUALITY, {
    return (SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value <= ((SolNumber) arguments->first->next->value)->value);
});

DEFINEOP(GREATER_THAN_EQUALITY, {
    return (SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value >= ((SolNumber) arguments->first->next->value)->value);
});

DEFINEOP(IF, {
    if (sol_bool_value_of(arguments->first->value)->value) {
        return sol_func_execute((SolFunction) arguments->first->next->value, sol_list_create(false), nil);
    } else if (arguments->first->next->next != NULL) {
        return sol_func_execute((SolFunction) arguments->first->next->next->value, sol_list_create(false), nil);
    }
    return nil;
});

DEFINEOP(LOOP, {
    SolObject result = nil;
    SolFunction function = (SolFunction) sol_obj_evaluate(arguments->first->next->value);
    while (sol_bool_value_of(sol_obj_evaluate(arguments->first->value))->value) {
        result = sol_func_execute(function, sol_list_create(false), nil);
    }
    return result;
});

DEFINEOP(CAT, {
    int len = 1;
    SolList strings = sol_list_create(false);
    SOL_LIST_ITR_BEGIN(arguments)
    sol_list_add_obj(strings, (arguments->current->value->type_id == TYPE_SOL_DATATYPE && ((SolDatatype) arguments->current->value)->type_id == DATA_TYPE_STR) ? arguments->current->value : (SolObject) sol_string_create(sol_obj_to_string(arguments->current->value)));
        len += strlen(((SolString) strings->last->value)->value);
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
    return (SolObject) sol_string_create(result);
});

DEFINEOP(OBJECT_GET, {
    return sol_obj_get_prop(self, ((SolToken) arguments->first->value)->identifier);
});

DEFINEOP(OBJECT_SET, {
    sol_obj_set_prop(self, ((SolToken) arguments->first->value)->identifier, arguments->first->next->value);
    return arguments->first->next->value;
});

DEFINEOP(OBJECT_CLONE, {
    return sol_obj_clone(self);
});
