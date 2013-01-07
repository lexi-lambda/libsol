
#include "soltoken.h"
#include "solfunc.h"
#include "solop.h"
#include "soltypes.h"

static void inline sol_func_substitute_parameters(SolList parameters, SolList arguments, bool evaluate_tokens, bool evaluate_lists);

SolFunction sol_func_create(SolList parameters, SolList statements) {
    return (SolFunction) sol_obj_clone_type((SolObject) Function, &(struct sol_func_raw){
            false,
            parameters,
            statements,
            sol_token_pool_snapshot()
        }, sizeof(sol_func));
}

SolObject sol_func_execute(SolFunction func, SolList arguments, SolObject self) {
    bool evaluate_tokens = sol_bool_value_of(sol_obj_get_prop((SolObject) func, "$evaluate-tokens"))->value;
    bool evaluate_lists = sol_bool_value_of(sol_obj_get_prop((SolObject) func, "$evaluate-lists"))->value;
    
    if (func->is_operator) {
        SolList evaluated = sol_list_create(false);
        SOL_LIST_ITR_BEGIN(arguments)
            SolObject object = arguments->current->value;
            switch (object->type_id) {
                case TYPE_SOL_TOKEN:
                    sol_list_add_obj(evaluated, evaluate_tokens ? sol_obj_evaluate(object) : object);
                    break;
                case TYPE_SOL_LIST:
                    sol_list_add_obj(evaluated, evaluate_lists ? sol_obj_evaluate(object) : object);
                    break;
                default:
                    sol_list_add_obj(evaluated, sol_obj_evaluate(object));
            }
        SOL_LIST_ITR_END(arguments)
        return ((SolOperator) func)->operator_ref(evaluated, self);
    }
    
    SolList statements = func->statements;
    
    // establish closure scope
    sol_token_pool_push_m(func->closure_scope);
    // create function scope
    sol_token_pool_push();
    
    // create 'self' reference
    sol_token_register("self", self);
    
    // perform parameter substitution
    sol_func_substitute_parameters(func->parameters, arguments, evaluate_tokens, evaluate_lists);
    
    // execute function statements
    SolObject ans = nil;
    
    SOL_LIST_ITR_BEGIN(statements)
        ans = sol_obj_evaluate(statements->current->value);
    SOL_LIST_ITR_END(statements)
    
    // destroy function/closure scope
    sol_token_pool_pop();
    func->closure_scope = sol_token_pool_pop_m();
    
    return ans;
}

static void inline sol_func_substitute_parameters(SolList parameters, SolList arguments, bool evaluate_tokens, bool evaluate_lists) {
    arguments->current = arguments->first;
    SOL_LIST_ITR_BEGIN(parameters)
        SolToken token = (SolToken) parameters->current->value;
        SolObject object = arguments->current->value;
        switch (object->type_id) {
            case TYPE_SOL_TOKEN:
                sol_token_register(token->identifier, evaluate_tokens ? sol_obj_evaluate(object) : object);
                break;
            case TYPE_SOL_LIST:
                sol_token_register(token->identifier, evaluate_lists ? sol_obj_evaluate(object) : object);
                break;
            default:
                sol_token_register(token->identifier, sol_obj_evaluate(object));
        }
        arguments->current = arguments->current->next;
    SOL_LIST_ITR_END(parameters)
}
