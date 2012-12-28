/* 
 * File:   soltypes.h
 * Author: Jake
 *
 * Created on November 19, 2012, 8:17 PM
 */

#ifndef SOLTYPES_H
#define	SOLTYPES_H

#include "sol.h"

typedef enum data_type {
    DATA_TYPE_NUM,
    DATA_TYPE_STR,
    DATA_TYPE_BOOL
} data_type;

typedef struct sol_datatype {
    sol_obj super; // extend sol_obj
    data_type type_id;
} sol_datatype;
typedef sol_datatype* SolDatatype;

typedef struct sol_num {
    sol_datatype super; // extend sol_datatype
    double value;
} sol_num;
typedef sol_num* SolNumber;

typedef struct sol_string {
    sol_datatype super; // extend sol_datatype
    char* value;
} sol_string;
typedef sol_string* SolString;

typedef struct sol_bool {
    sol_datatype super; // extend sol_datatype
    int value;
} sol_bool;
typedef sol_bool* SolBoolean;


SolNumber sol_num_create(double value);

SolString sol_string_create(char* value);

SolBoolean sol_bool_create(int value);
SolBoolean sol_bool_value_of(SolObject obj);

#endif	/* SOLTYPES_H */

