
#include "soltoken.h"
#include "solfunc.h"
#include "solop.h"
#include "soltypes.h"
#include "solerror.h"

static void inline sol_func_substitute_parameters(SolList parameters, SolList arguments);

SolFunction sol_func_create(SolList parameters, SolList statements) {
    return (SolFunction) sol_obj_clone_type((SolObject) Function, &(struct sol_func_raw){
            false,
            (SolList) sol_obj_retain((SolObject) parameters),
            (SolList) sol_obj_retain((SolObject) statements),
            sol_token_pool_snapshot()
        }, sizeof(sol_func));
}

SolObject sol_func_execute(SolFunction func, SolList arguments, SolObject self) {
    if (func->is_operator) {
        SolObject result = ((SolOperator) func)->operator_ref(arguments, self);
        return result;
    }
    
    SolList statements = func->statements;
    SolObject ans = nil;
    
    try {
        // establish closure scope
        sol_token_pool_push_m(func->closure_scope);
        // create function scope
        sol_token_pool_push();
        
        // create 'this' reference
        sol_token_register("this", (SolObject) func);
        // create 'self' reference
        sol_token_register("self", self);
        
        // insert 'arguments' reference and perform parameter substitution
        sol_func_substitute_parameters(func->parameters, arguments);
        
        // execute function statements
        SOL_LIST_ITR(statements, current, i) {
            sol_obj_release(ans);
            ans = sol_obj_evaluate(current->value);
        }
    } finally {
        // destroy function/closure scope
        sol_token_pool_pop();
        func->closure_scope = sol_token_pool_pop_m();
    }
    
    return ans;
}

static void inline sol_func_substitute_parameters(SolList parameters, SolList arguments) {
    // evaluate arguments
    sol_token_register("arguments", (SolObject) arguments);
    
    // register parameters
    SOL_LIST_ITR_PARALLEL(parameters, current_parameter, i_parameter, arguments, current_argument, i_argument) {
        if (!current_parameter) break;
        SolToken token = (SolToken) current_parameter->value;
        SolObject object = current_argument ? current_argument->value : nil;
        sol_token_register(token->identifier, object);
    }
}
