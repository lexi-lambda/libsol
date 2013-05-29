
#include <string.h>

#include "soltypes.h"

SolNumber sol_num_create(double value) {
    return (SolNumber) sol_obj_clone_type((SolObject) Number, &(struct sol_num_raw){
            DATA_TYPE_NUM,
            value
        }, sizeof(sol_num));
}

SolString sol_string_create(char* value) {
    return (SolString) sol_obj_clone_type((SolObject) String, &(struct sol_string_raw){
            DATA_TYPE_STR,
            strdup(value)
        }, sizeof(sol_string));
}

SolBoolean sol_bool_create(bool value) {
    static SolBoolean SOL_TRUE = NULL;
    if (SOL_TRUE == NULL) SOL_TRUE = (SolBoolean) sol_obj_clone_type((SolObject) Boolean, &(struct sol_bool_raw){
            DATA_TYPE_BOOL,
            true
        }, sizeof(sol_bool));
    static SolBoolean SOL_FALSE = NULL;
    if (SOL_FALSE == NULL) SOL_FALSE = (SolBoolean) sol_obj_clone_type((SolObject) Boolean, &(struct sol_bool_raw){
            DATA_TYPE_BOOL,
            false
        }, sizeof(sol_bool));
    return value ? SOL_TRUE : SOL_FALSE;
}

SolBoolean sol_bool_value_of(SolObject obj) {
    if (obj == NULL) return (SolBoolean) sol_obj_retain((SolObject) sol_bool_create(false));
    switch (obj->type_id) {
        case TYPE_SOL_DATATYPE:
            if (((SolDatatype) obj)->type_id == DATA_TYPE_BOOL) {
                return (SolBoolean) sol_obj_retain(obj);
            } else {
                return (SolBoolean) sol_obj_retain((SolObject) sol_bool_create(true));
            }
        default:
            return (SolBoolean) sol_obj_retain((SolObject) sol_bool_create(true));
    }
}
