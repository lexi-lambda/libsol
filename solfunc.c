
#include "soltoken.h"
#include "solfunc.h"
#include "solop.h"

SolFunction sol_func_create(SolList parameters, SolList statements) {
    return (SolFunction) sol_obj_clone_type((SolObject) Function, &(struct sol_func_raw){
            false,
            parameters,
            statements,
            sol_token_pool_snapshot()
        }, sizeof(sol_func));
}

SolObject sol_func_execute(SolFunction func, SolList arguments, SolObject self) {
    if (func->is_operator) return ((SolOperator) func)->operator_ref(arguments, self);
    
    SolList parameters = func->parameters;
    SolList statements = func->statements;
    
    // establish closure scope
    sol_token_pool_push_m(func->closure_scope);
    // create function scope
    sol_token_pool_push();
    
    // create 'self' reference
    sol_token_register("self", self);
    
    // perform parameter substitution
    arguments->current = arguments->first;
    SOL_LIST_ITR_BEGIN(parameters)
        SolToken token = (SolToken) parameters->current->value;
        SolObject object = arguments->current->value;
        sol_token_register(token->identifier, sol_obj_evaluate(object));
        arguments->current = arguments->current->next;
    SOL_LIST_ITR_END(parameters)
    
    // execute function statements
    SolObject ans = nil;
    
    SOL_LIST_ITR_BEGIN(statements)
        ans = sol_obj_evaluate(statements->current->value);
    SOL_LIST_ITR_END(statements)
    
    // destroy function/closure scope
    sol_token_pool_pop();
    
    return ans;
}
