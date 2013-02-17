
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "sol.h"
#include "soltoken.h"
#include "soltypes.h"
#include "sollist.h"
#include "solfunc.h"
#include "solop.h"
#include "solevent.h"

SolObject Object;
SolToken Token;
SolFunction Function;
SolList List;
SolNumber Number;
SolString String;
SolBoolean Boolean;
SolEvent Event;

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
    
    Event = sol_obj_clone(Object);
    sol_token_register("Event", Event);
    
    nil = (SolObject) sol_list_create(false);
    sol_token_register("nil", nil);
    
    // set up prototypes
    sol_obj_set_proto((SolObject) Function, "$evaluate-tokens", (SolObject) sol_bool_create(true));
    sol_obj_set_proto((SolObject) Function, "$evaluate-lists", (SolObject) sol_bool_create(true));
    
    sol_runtime_init_operators();
    sol_event_loop_create();
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
    REGISTER_OP(exit, EXIT);
    REGISTER_OP(bind, BIND);
    sol_obj_set_prop((SolObject) OBJ_BIND, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    REGISTER_OP(set, SET);
    sol_obj_set_prop((SolObject) OBJ_SET, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    REGISTER_OP(define, DEFINE);
    sol_obj_set_prop((SolObject) OBJ_DEFINE, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    REGISTER_OP(evaluate, EVALUATE);
    sol_obj_set_prop((SolObject) OBJ_EVALUATE, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    sol_obj_set_prop((SolObject) OBJ_EVALUATE, "$evaluate-lists", (SolObject) sol_bool_create(false));
    REGISTER_OP(^, LAMBDA);
    sol_obj_set_prop((SolObject) OBJ_LAMBDA, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    sol_obj_set_prop((SolObject) OBJ_LAMBDA, "$evaluate-lists", (SolObject) sol_bool_create(false));
    REGISTER_OP(listen, LISTEN);
    REGISTER_OP(dispatch, DISPATCH);
    REGISTER_OP(->token, TO_TOKEN);
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
    REGISTER_METHOD(Object, @get, PROTOTYPE_GET);
    sol_obj_set_prop((SolObject) OBJ_PROTOTYPE_GET, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    REGISTER_METHOD(Object, @set, PROTOTYPE_SET);
    sol_obj_set_prop((SolObject) OBJ_PROTOTYPE_SET, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    REGISTER_METHOD(Object, clone, OBJECT_CLONE);
}

void sol_runtime_destroy() {
    // wait for events to complete
    if (sol_event_has_work())
        sol_event_loop_run();
}

static SolObject sol_runtime_execute_get_object(unsigned char** data);
void sol_runtime_execute(unsigned char* data) {
    // ensure magic number is correct
    if (data[0] != 'S' || data[1] != 'O' || data[2] != 'L' || data[3] != 'B' || data[4] != 'I' || data[5] != 'N') {
        fprintf(stderr, "Error executing sol bytecode: invalid magic number.\n");
        exit(EXIT_FAILURE);
    }
    
    // advance data pointer
    data += 6;
    
    // execute top-level objects
    SolObject obj;
    while ((obj = sol_runtime_execute_get_object(&data)) != NULL) {
        sol_obj_evaluate(obj);
        sol_obj_release(obj);
    }
}
static SolObject sol_runtime_execute_get_object(unsigned char** data) {
    switch (**data) {
        case 0x1: {
            SolObject object = sol_obj_clone(Object);
            uint32_t length = *++*data;
            *data += sizeof(length);
            for (; length > 0; length--) {
                SolString key = (SolString) sol_runtime_execute_get_object(data);
                SolObject value = sol_runtime_execute_get_object(data);
                sol_obj_set_prop(object, key->value, value);
                sol_obj_release((SolObject) key);
                sol_obj_release(value);
            }
            return object;
        }
        case 0x2: {
            SolList list = (SolList) sol_obj_retain((SolObject) sol_list_create((bool) *++*data));
            uint32_t freezeCount = *++*data;
            list->freezeCount = freezeCount - 1;
            *data += sizeof(freezeCount);
            uint32_t length = **data;
            *data += sizeof(length);
            for (; length > 0; length--) {
                SolObject value = sol_runtime_execute_get_object(data);
                sol_list_add_obj(list, value);
                sol_obj_release(value);
            }
            return (SolObject) list;
        }
        case 0x3: {
            ++*data;
            SolList parameters = (SolList) sol_runtime_execute_get_object(data);
            SolList statements = (SolList) sol_runtime_execute_get_object(data);
            SolFunction function = sol_func_create(parameters, statements);
            sol_obj_release((SolObject) parameters);
            sol_obj_release((SolObject) statements);
            return (SolObject) function;
        }
        case 0x4: {
            uint32_t length = *++*data;
            *data += sizeof(length);
            char* value = memcpy(malloc(sizeof(*value) * length + 1), *data, sizeof(*value) * length + 1);
            value[sizeof(*value) * length] = '\0';
            *data += sizeof(*value) * length;
            SolToken token = (SolToken) sol_obj_retain((SolObject) sol_token_create(value));
            free(value);
            return (SolObject) token;
        }
        case 0x5: {
            ++*data;
            double value;
            memcpy(&value, *data, sizeof(value));
            *data += sizeof(value);
            return sol_obj_retain((SolObject) sol_num_create(value));
        }
        case 0x6: {
            uint64_t length = *++*data;
            *data += sizeof(length);
            char* value = memcpy(malloc(sizeof(*value) * length + 1), *data, sizeof(*value) * length + 1);
            value[sizeof(*value) * length] = '\0';
            *data += sizeof(*value) * length;
            SolString string = (SolString) sol_obj_retain((SolObject) sol_string_create(value));
            free(value);
            return (SolObject) string;
        }
        case 0x7: {
            bool value = *++*data;
            ++*data;
            return sol_obj_retain((SolObject) sol_bool_create(value));
        }
        case 0x0:
            return NULL;
        default:
            fprintf(stderr, "Error loading sol bytecode: invalid object type %u.\n", **data);
            exit(EXIT_FAILURE);
    }
}
