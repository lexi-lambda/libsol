
#include <stdlib.h>
#include <time.h>
#include "sol.h"
#include "soltoken.h"
#include "soltypes.h"
#include "sollist.h"
#include "solfunc.h"
#include "solop.h"

SolObject Object;
SolToken Token;
SolFunction Function;
SolList List;
SolNumber Number;
SolString String;
SolBoolean Boolean;

SolObject nil;

static inline void sol_runtime_init_operators();

void sol_runtime_init() {
    local_token_pool = calloc(1, sizeof(*local_token_pool));
    
    // seed rand
    srand((unsigned int) time(NULL));
    
    // initialize object types
    Object = malloc(sizeof(*Object));
    memcpy(Object, &DEFAULT_OBJECT, sizeof(*Object));
    sol_token_register("Object", Object);
    
    // create token as object, then switch to token to avoid early resolution when binding in pool
    Token = (SolToken) sol_obj_create_global(Object, TYPE_SOL_OBJ, &(struct sol_token_raw){ NULL }, sizeof(*Token), "Token");
    Token->super.type_id = TYPE_SOL_TOKEN;
    
    Function = (SolFunction) sol_obj_create_global(Object, TYPE_SOL_FUNC, &(struct sol_func_raw){ NULL, NULL, NULL }, sizeof(*Function), "Function");
    List = (SolList) sol_obj_create_global(Object, TYPE_SOL_LIST, &(struct sol_list_raw){ false, NULL, NULL, 0, NULL, 0 }, sizeof(*List), "List");
    Number = (SolNumber) sol_obj_create_global(Object, TYPE_SOL_DATATYPE, &(struct sol_num_raw){ DATA_TYPE_NUM, 0 }, sizeof(*Number), "Number");
    String = (SolString) sol_obj_create_global(Object, TYPE_SOL_DATATYPE, &(struct sol_string_raw){ DATA_TYPE_STR, NULL }, sizeof(*String), "String");
    Boolean = (SolBoolean) sol_obj_create_global(Object, TYPE_SOL_DATATYPE, &(struct sol_bool_raw){ DATA_TYPE_BOOL, 1 }, sizeof(*Boolean), "Boolean");
    
    nil = (SolObject) sol_list_create(false);
    sol_token_register("nil", nil);
    
    // set up prototypes
    sol_obj_set_proto((SolObject) Function, "$evaluate-tokens", (SolObject) sol_bool_create(true));
    sol_obj_set_proto((SolObject) Function, "$evaluate-lists", (SolObject) sol_bool_create(true));
    
    sol_runtime_init_operators();
}

#define REGISTER_OP(token, name) SolOperator OBJ_ ## name = sol_operator_create(OP_ ## name); sol_token_register(#token, (SolObject) OBJ_ ## name)
#define REGISTER_METHOD(object, token, name) SolOperator OBJ_ ## name = sol_operator_create(OP_ ## name); sol_obj_set_proto(object, #token, (SolObject) OBJ_ ## name);
static inline void sol_runtime_init_operators() {
    REGISTER_OP(+, ADD);
    REGISTER_OP(-, SUBTRACT);
    REGISTER_OP(*, MULTIPLY);
    REGISTER_OP(/, DIVIDE);
    REGISTER_OP(mod, MOD);
    REGISTER_OP(require, REQUIRE);
    REGISTER_OP(bind, BIND);
    sol_obj_set_prop((SolObject) OBJ_BIND, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    REGISTER_OP(set, SET);
    sol_obj_set_prop((SolObject) OBJ_SET, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    REGISTER_OP(evaluate, EVALUATE);
    sol_obj_set_prop((SolObject) OBJ_EVALUATE, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    sol_obj_set_prop((SolObject) OBJ_EVALUATE, "$evaluate-lists", (SolObject) sol_bool_create(false));
    REGISTER_OP(^, LAMBDA);
    sol_obj_set_prop((SolObject) OBJ_LAMBDA, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    sol_obj_set_prop((SolObject) OBJ_LAMBDA, "$evaluate-lists", (SolObject) sol_bool_create(false));
    REGISTER_OP(print, PRINT);
    REGISTER_OP(not, NOT);
    REGISTER_OP(and, AND);
    REGISTER_OP(or, OR);
    REGISTER_OP(=, EQUALITY);
    REGISTER_OP(<, LESS_THAN);
    REGISTER_OP(>, GREATER_THAN);
    REGISTER_OP(<=, LESS_THAN_EQUALITY);
    REGISTER_OP(>=, GREATER_THAN_EQUALITY);
    REGISTER_OP(if, IF);
    REGISTER_OP(loop, LOOP);
    sol_obj_set_prop((SolObject) OBJ_LOOP, "$evaluate-lists", (SolObject) sol_bool_create(false));
    REGISTER_OP(cat, CAT);
    
    REGISTER_METHOD(Object, get, OBJECT_GET);
    sol_obj_set_prop((SolObject) OBJ_OBJECT_GET, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    REGISTER_METHOD(Object, set, OBJECT_SET);
    sol_obj_set_prop((SolObject) OBJ_OBJECT_SET, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    REGISTER_METHOD(Object, clone, OBJECT_CLONE);
}

void sol_runtime_destroy() {
}
