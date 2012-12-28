
#include <string.h>

#include "soltypes.h"


const sol_num DEFAULT_NUM = {
    {
        { TYPE_SOL_DATATYPE, NULL, NULL, NULL }, DATA_TYPE_NUM
    }, 0
};

const sol_string DEFAULT_STRING = {
    {
        { TYPE_SOL_DATATYPE, NULL, NULL, NULL }, DATA_TYPE_STR
    }, NULL
};

static sol_bool BOOL_FALSE = {
    {
        { TYPE_SOL_DATATYPE, NULL, NULL, NULL }, DATA_TYPE_BOOL
    }, 0
};
static sol_bool BOOL_TRUE = {
    {
        { TYPE_SOL_DATATYPE, NULL, NULL, NULL }, DATA_TYPE_BOOL
    }, 1
};

SolNumber sol_num_create(double value) {
    SolNumber new_num = malloc(sizeof(*new_num));
    memcpy(new_num, &DEFAULT_NUM, sizeof(*new_num));
    new_num->value = value;
    return new_num;
}

SolString sol_string_create(char* value) {
    SolString new_string = malloc(sizeof(*new_string));
    memcpy(new_string, &DEFAULT_STRING, sizeof(*new_string));
    new_string->value = value;
    return new_string;
}

SolBoolean sol_bool_create(int value) {
    return value == 0 ? &BOOL_FALSE : &BOOL_TRUE;
}

SolBoolean sol_bool_value_of(SolObject obj) {
    switch (obj->type_id) {
        case TYPE_SOL_DATATYPE:
            if (((SolDatatype) obj)->type_id == DATA_TYPE_BOOL) {
                return (SolBoolean) obj;
            } else {
                return sol_bool_create(1);
            }
        default:
            return sol_bool_create(1);
    }
}
