
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "solop.h"
#include "soltypes.h"
#include "soltoken.h"
#include "solfunc.h"

#define DEFINEOP(opname) SolObject perform_OP_ ## opname (SolList arguments)

DEFINEOP(ADD) {
    SolNumber result = sol_num_create(0);
    SOL_LIST_ITR_BEGIN(arguments)
        SolNumber num = (SolNumber) sol_obj_evaluate(arguments->current->value);
        result->value += num->value;
    SOL_LIST_ITR_END(arguments)
    return (SolObject) result;
}

DEFINEOP(SUBTRACT) {
    SolNumber result = sol_num_create(((SolNumber) sol_obj_evaluate(arguments->first->value))->value);
    SolList minusArguments = sol_list_get_sublist_s(arguments, 1);
    SOL_LIST_ITR_BEGIN(minusArguments)
        SolNumber num = (SolNumber) sol_obj_evaluate(minusArguments->current->value);
        result->value -= num->value;
    SOL_LIST_ITR_END(minusArguments)
    return (SolObject) result;
}

DEFINEOP(MULTIPLY) {
    SolNumber result = sol_num_create(1);
    SOL_LIST_ITR_BEGIN(arguments)
        SolNumber num = (SolNumber) sol_obj_evaluate(arguments->current->value);
        result->value *= num->value;
    SOL_LIST_ITR_END(arguments)
    return (SolObject) result;
}

DEFINEOP(DIVIDE) {
    SolNumber result = sol_num_create(((SolNumber) sol_obj_evaluate(arguments->first->value))->value);
    SolList minusArguments = sol_list_get_sublist_s(arguments, 1);
    SOL_LIST_ITR_BEGIN(minusArguments)
        SolNumber num = (SolNumber) sol_obj_evaluate(minusArguments->current->value);
        result->value /= num->value;
    SOL_LIST_ITR_END(minusArguments)
    return (SolObject) result;
}

DEFINEOP(MOD) {
    SolNumber result = sol_num_create(((SolNumber) sol_obj_evaluate(arguments->first->value))->value);
    SolList minusArguments = sol_list_get_sublist_s(arguments, 1);
    SOL_LIST_ITR_BEGIN(minusArguments)
        SolNumber num = (SolNumber) sol_obj_evaluate(minusArguments->current->value);
        result->value = fmod(result->value, num->value);
    SOL_LIST_ITR_END(minusArguments)
    return (SolObject) result;
}

DEFINEOP(BIND) {
    SolObject result = nil;
    if (arguments->first->value->type_id == TYPE_SOL_LIST) {
        SolList list_args = (SolList) arguments;
        SOL_LIST_ITR_BEGIN(list_args)
            SolList binding = (SolList) list_args->current->value;
            SolToken token = (SolToken) binding->first->value;
            result = binding->length > 1 ? binding->first->next->value : nil;
            sol_token_register(token->identifier, result);
        SOL_LIST_ITR_END(list_args)
    } else {
        SolToken token = (SolToken) arguments->first->value;
        result = arguments->length > 1 ? arguments->first->next->value : nil;
        sol_token_register(token->identifier, result);
    }
    return result;
}

DEFINEOP(SET) {
    SolObject result = nil;
    if (arguments->first->value->type_id == TYPE_SOL_LIST) {
        SolList list_args = (SolList) arguments;
        SOL_LIST_ITR_BEGIN(list_args)
            SolList binding = (SolList) list_args->current->value;
            SolToken token = (SolToken) binding->first->value;
            result = sol_obj_evaluate(binding->first->next->value);
            sol_token_update(token->identifier, result);
        SOL_LIST_ITR_END(list_args)
    } else {
        SolToken token = (SolToken) arguments->first->value;
        result = sol_obj_evaluate(arguments->first->next->value);
        sol_token_update(token->identifier, result);
    }
    return result;
}

DEFINEOP(EVALUATE) {
    return sol_obj_evaluate(arguments->first->value);
}

DEFINEOP(LAMBDA) {
    SolList parameters = (SolList) arguments->first->value;
    SolList statements = sol_list_get_sublist_s(arguments, 1);
    return (SolObject) sol_func_create(parameters, statements);
}

DEFINEOP(PRINT) {
    SolObject argument = sol_obj_evaluate(arguments->first->value);
    printf("%s\n", sol_obj_to_string(argument));
    return argument;
}

DEFINEOP(NOT) {
    return (SolObject) (sol_bool_value_of(arguments->first->value)->value ? sol_bool_create(0) : sol_bool_create(1));
}

DEFINEOP(AND) {
    SOL_LIST_ITR_BEGIN(arguments)
        if (!sol_bool_value_of(arguments->current->value)->value) return (SolObject) sol_bool_create(0);
    SOL_LIST_ITR_END(arguments)
    return (SolObject) sol_bool_create(1);
}

DEFINEOP(OR) {
    SOL_LIST_ITR_BEGIN(arguments)
        if (sol_bool_value_of(arguments->current->value)->value) return (SolObject) sol_bool_create(1);
    SOL_LIST_ITR_END(arguments)
    return (SolObject) sol_bool_create(0);
}

DEFINEOP(EQUALITY) {
    return (SolObject) sol_bool_create(sol_obj_equals(sol_obj_evaluate(arguments->first->value), sol_obj_evaluate(arguments->first->next->value)));
}

DEFINEOP(LESS_THAN) {
    return (SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value < ((SolNumber) arguments->first->next->value)->value);
}

DEFINEOP(GREATER_THAN) {
    return (SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value > ((SolNumber) arguments->first->next->value)->value);
}

DEFINEOP(LESS_THAN_EQUALITY) {
    return (SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value <= ((SolNumber) arguments->first->next->value)->value);
}

DEFINEOP(GREATER_THAN_EQUALITY) {
    return (SolObject) sol_bool_create(((SolNumber) arguments->first->value)->value >= ((SolNumber) arguments->first->next->value)->value);
}

DEFINEOP(IF) {
    if (sol_bool_value_of(sol_obj_evaluate(arguments->first->value))->value) {
        return sol_obj_evaluate((SolObject) arguments->first->next);
    } else if (arguments->first->next->next != NULL) {
        return sol_obj_evaluate((SolObject) arguments->first->next->next);
    }
    return nil;
}

DEFINEOP(LOOP) {
    SolObject result = nil;
    while (sol_bool_value_of(sol_obj_evaluate(arguments->first->value))->value) {
        result = sol_obj_evaluate(arguments->first->next->value);
    }
    return result;
}

#define DECLAREOP(opname) const sol_operator OP_ ## opname = {      \
    { TYPE_SOL_OPERATOR, NULL, NULL, NULL },                  \
    &perform_OP_ ## opname                                    \
}

DECLAREOP(ADD);
DECLAREOP(SUBTRACT);
DECLAREOP(MULTIPLY);
DECLAREOP(DIVIDE);
DECLAREOP(MOD);

DECLAREOP(BIND);
DECLAREOP(SET);
DECLAREOP(EVALUATE);
DECLAREOP(LAMBDA);

DECLAREOP(PRINT);

DECLAREOP(NOT);
DECLAREOP(AND);
DECLAREOP(OR);

DECLAREOP(EQUALITY);
DECLAREOP(LESS_THAN);
DECLAREOP(GREATER_THAN);
DECLAREOP(LESS_THAN_EQUALITY);
DECLAREOP(GREATER_THAN_EQUALITY);

DECLAREOP(IF);
DECLAREOP(LOOP);

SolOperator sol_operator_create(sol_operator operation) {
    SolOperator operation_obj = malloc(sizeof(*operation_obj));
    memcpy(operation_obj, &operation, sizeof(*operation_obj));
    return operation_obj;
}
