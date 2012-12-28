
#include "soltoken.h"
#include "solfunc.h"

const sol_func DEFAULT_FUNC = {
    {
        TYPE_SOL_FUNC, NULL, NULL, NULL
    }, NULL, NULL, NULL
};

SolFunction sol_func_create(SolList parameters, SolList statements) {
    SolFunction func = malloc(sizeof(*func));
    memcpy(func, &DEFAULT_FUNC, sizeof(*func));
    func->parameters = parameters;
    func->statements = statements;
    func->closure_scope = sol_token_pool_snapshot();
    return func;
}

SolObject sol_func_execute(SolFunction func, SolList arguments) {
    SolList parameters = func->parameters;
    SolList statements = func->statements;
    
    // establish closure scope
    sol_token_pool_push_m(func->closure_scope);
    // create function scope
    sol_token_pool_push();
    
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
