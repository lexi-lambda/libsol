
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <arpa/inet.h>
#include "sol.h"
#include "soltoken.h"
#include "soltypes.h"
#include "sollist.h"
#include "solfunc.h"
#include "solop.h"
#include "solevent.h"

SolObject Object;
SolObject RawObject;
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
    sol_obj_retain(Object);
    
    RawObject = malloc(sizeof(*RawObject));
    memcpy(RawObject, &DEFAULT_OBJECT, sizeof(*RawObject));
    sol_obj_retain(RawObject);
    
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
    
    // automatically load the solcore library, if it exists
    FILE* f = fopen("/usr/local/lib/sol/solcore.solar/descriptor.yml", "r");
    if (f) {
        fclose(f);
        SolList arguments = (SolList) sol_obj_retain((SolObject) sol_list_create(false));
        sol_list_add_obj(arguments, (SolObject) sol_string_create("solcore"));
        OP_REQUIRE(arguments, nil);
        sol_obj_release((SolObject) arguments);
    }
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
    REGISTER_OP(bound?, BOUND);
    REGISTER_OP(set, SET);
    REGISTER_OP(define, DEFINE);
    REGISTER_OP(evaluate, EVALUATE);
    REGISTER_OP(freeze, FREEZE);
    sol_obj_set_prop((SolObject) OBJ_FREEZE, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    sol_obj_set_prop((SolObject) OBJ_FREEZE, "$evaluate-lists", (SolObject) sol_bool_create(false));
    REGISTER_OP(^, LAMBDA);
    sol_obj_set_prop((SolObject) OBJ_LAMBDA, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    sol_obj_set_prop((SolObject) OBJ_LAMBDA, "$evaluate-lists", (SolObject) sol_bool_create(false));
    REGISTER_OP(->token, TO_TOKEN);
    REGISTER_OP(print, PRINT);
    REGISTER_OP(not, NOT);
    REGISTER_OP(and, AND);
    sol_obj_set_prop((SolObject) OBJ_AND, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    sol_obj_set_prop((SolObject) OBJ_AND, "$evaluate-lists", (SolObject) sol_bool_create(false));
    REGISTER_OP(or, OR);
    sol_obj_set_prop((SolObject) OBJ_OR, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    sol_obj_set_prop((SolObject) OBJ_OR, "$evaluate-lists", (SolObject) sol_bool_create(false));
    REGISTER_OP(=, EQUALITY);
    REGISTER_OP(<, LESS_THAN);
    REGISTER_OP(>, GREATER_THAN);
    REGISTER_OP(<=, LESS_THAN_EQUALITY);
    REGISTER_OP(>=, GREATER_THAN_EQUALITY);
    REGISTER_OP(?, CONDITIONAL);
    sol_obj_set_prop((SolObject) OBJ_CONDITIONAL, "$evaluate-tokens", (SolObject) sol_bool_create(false));
    sol_obj_set_prop((SolObject) OBJ_CONDITIONAL, "$evaluate-lists", (SolObject) sol_bool_create(false));
    REGISTER_OP(if, IF);
    REGISTER_OP(loop, LOOP);
    REGISTER_OP(cat, CAT);
    
    REGISTER_METHOD(Object, get, OBJECT_GET);
    REGISTER_METHOD(Object, set, OBJECT_SET);
    REGISTER_METHOD(Object, get-metadata, OBJECT_GET_METADATA);
    REGISTER_METHOD(Object, set-metadata, OBJECT_SET_METADATA);
    REGISTER_METHOD(Object, @get, PROTOTYPE_GET);
    REGISTER_METHOD(Object, @set, PROTOTYPE_SET);
    REGISTER_METHOD(Object, @get-metadata, PROTOTYPE_GET_METADATA);
    REGISTER_METHOD(Object, @set-metadata, PROTOTYPE_SET_METADATA);
    REGISTER_METHOD(Object, clone, OBJECT_CLONE);
    REGISTER_METHOD(Object, ->string, OBJECT_TO_STRING);
    REGISTER_METHOD(Object, listen, OBJECT_LISTEN);
    REGISTER_METHOD(Object, dispatch, OBJECT_DISPATCH);
    
    sol_obj_set_proto(RawObject, "get", (SolObject) OBJ_OBJECT_GET);
    sol_obj_set_proto(RawObject, "set", (SolObject) OBJ_OBJECT_SET);
}

void sol_runtime_destroy() {
}

uint64_t htonll(uint64_t value);
uint64_t ntohll(uint64_t value);
uint64_t htonll(uint64_t value) {
    uint16_t num = 1;
    if (*(char *)&num == 1) {
        uint32_t high_part = htonl((uint32_t) (value >> 32));
        uint32_t low_part = htonl((uint32_t) (value & 0xFFFFFFFFLL));
        return (((uint64_t) low_part) << 32) | high_part;
    } else {
        return value;
    }
}
uint64_t ntohll(uint64_t value) {
    uint16_t num = 1;
    if (*(char *)&num == 1) {
        uint32_t high_part = ntohl((uint32_t) (value >> 32));
        uint32_t low_part = ntohl((uint32_t) (value & 0xFFFFFFFFLL));
        return (((uint64_t) low_part) << 32) | high_part;
    } else {
        return value;
    }
}

static unsigned char* data_start;
static SolObject sol_runtime_execute_get_object(unsigned char** data);
static uint64_t sol_runtime_execute_decode_length(unsigned char** data);
SolObject sol_runtime_execute(unsigned char* data) {
    data_start = data;
    // ensure magic number is correct
    if (data[0] != 'S' || data[1] != 'O' || data[2] != 'L' || data[3] != 'B' || data[4] != 'I' || data[5] != 'N') {
        fprintf(stderr, "Error executing sol bytecode: invalid magic number.\n");
        exit(EXIT_FAILURE);
    }
    
    // advance data pointer
    data += 6;
    
    // execute top-level objects
    SolObject obj, ans = nil;
    while ((obj = sol_runtime_execute_get_object(&data)) != NULL) {
        sol_obj_release(ans);
        ans = sol_obj_evaluate(obj);
        sol_obj_release(obj);
    }
    
    // wait for events to complete
    sol_event_loop_run();
    
    return ans;
}
static SolObject sol_runtime_execute_get_object(unsigned char** data) {
    switch (**data) {
        case 0x1: {
            ++*data;
            uint64_t type_length = sol_runtime_execute_decode_length(data);
            SolObject object;
            if (type_length > 0) {
                char* type_name = memcpy(malloc(sizeof(*type_name) * type_length + 1), *data, sizeof(*type_name) * type_length);
                type_name[type_length] = '\0';
                *data += sizeof(*type_name) * type_length;
                SolObject parent = sol_token_resolve(type_name);
                free(type_name);
                object = sol_obj_clone(parent);
            } else {
                object = sol_obj_create_raw();
            }
            uint64_t object_length = sol_runtime_execute_decode_length(data);
            for (int i = 0; i < object_length; i++) {
                uint64_t key_length = sol_runtime_execute_decode_length(data);
                char* key = memcpy(malloc(sizeof(*key) * key_length + 1), *data, sizeof(*key) * key_length);
                key[key_length] = '\0';
                *data += sizeof(*key) * key_length;
                SolObject value = sol_runtime_execute_get_object(data);
                SolObject evaluated = sol_obj_evaluate(value);
                sol_obj_set_prop(object, key, evaluated);
                free(key);
                sol_obj_release(evaluated);
                sol_obj_release(value);
            }
            return object;
        }
        case 0x2: {
            SolList list = (SolList) sol_obj_retain((SolObject) sol_list_create((bool) *++*data));
            bool literal = (bool) *++*data;
            ++*data;
            uint64_t length = sol_runtime_execute_decode_length(data);
            for (; length > 0; length--) {
                SolObject value = sol_runtime_execute_get_object(data);
                if (literal) {
                    SolObject evaluated = sol_obj_evaluate(value);
                    sol_list_add_obj(list, evaluated);
                    sol_obj_release(evaluated);
                } else {
                    sol_list_add_obj(list, value);
                }
                sol_obj_release(value);
            }
            if (literal) {
                SolObjectFrozen frozen = (SolObjectFrozen) sol_obj_retain((SolObject) sol_obj_freeze((SolObject) list));
                sol_obj_release((SolObject) list);
                return (SolObject) frozen;
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
            return sol_obj_retain((SolObject) function);
        }
        case 0x4: {
            ++*data;
            uint64_t length = sol_runtime_execute_decode_length(data);
            char* value = memcpy(malloc(sizeof(*value) * length + 1), *data, sizeof(*value) * length + 1);
            value[length] = '\0';
            *data += sizeof(*value) * length;
            SolToken token = (SolToken) sol_obj_retain((SolObject) sol_token_create(value));
            free(value);
            return (SolObject) token;
        }
        case 0x5: {
            ++*data;
            char sign = **data;
            ++*data;
            uint64_t base_data;
            memcpy(&base_data, *data, sizeof(uint64_t));
            base_data = ntohll(base_data);
            *data += sizeof(uint64_t);
            uint64_t exp_data = sol_runtime_execute_decode_length(data);
            int exp = (sign & 0x1) ? exp_data : exp_data * -1;
            double base = (double) base_data / pow(FLT_RADIX, DBL_MANT_DIG);
            double value = ldexp(base, exp);
            return sol_obj_retain((SolObject) sol_num_create((sign & 0x2) ? value : value * -1));
        }
        case 0x6: {
            ++*data;
            uint64_t length = sol_runtime_execute_decode_length(data);
            char* value = memcpy(malloc(sizeof(*value) * length + 1), *data, sizeof(*value) * length + 1);
            value[length] = '\0';
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
        case 0x8: {
            ++*data;
            SolObject value = sol_runtime_execute_get_object(data);
            return sol_obj_retain((SolObject) sol_obj_freeze(value));
        }
        case 0x0:
            return NULL;
        default:
            fprintf(stderr, "Error loading sol bytecode: invalid object type %u.\n", **data);
            exit(EXIT_FAILURE);
    }
}
static uint64_t sol_runtime_execute_decode_length(unsigned char** data) {
    char length_type = (**data & 0xF0) >> 4;
    uint64_t length;
    switch (length_type) {
        case 0x1:
            length = **data & 0xF;
            ++*data;
            break;
        case 0x2: {
            uint16_t length_data;
            memcpy(&length_data, *data, sizeof(length_data));
            length = ntohs(length_data);
            length &= 0xFFF;
            *data += sizeof(uint16_t);
            break;
        }
        case 0x3: {
            uint32_t length_data;
            memcpy(&length_data, *data, sizeof(length_data));
            length = ntohl(length_data);
            length &= 0xFFFFF;
            *data += sizeof(uint32_t);
            break;
        }
        case 0x4: {
            memcpy(&length, *data, sizeof(length));
            length = ntohll(length);
            length &= 0xFFFFFFF;
            *data += sizeof(uint64_t);
            break;
        }
        default:
            fprintf(stderr, "Error loading sol bytecode: invalid length type %x (0x%x) at offset 0x%lx.\n", length_type, **data, *data - data_start);
            exit(EXIT_FAILURE);
    }
    return length;
}
