/* 
 * File:   soltypes.h
 * Author: Jake
 *
 * Created on November 19, 2012, 8:17 PM
 */

#ifndef SOLTYPES_H
#define	SOLTYPES_H

#include <stdbool.h>
#include "sol.h"

typedef enum data_type {
    DATA_TYPE_NUM,
    DATA_TYPE_STR,
    DATA_TYPE_BOOL
} data_type;

#define DEFINE_DATATYPE(name, body) STRUCT_EXTEND(sol_obj, name, data_type type_id; body)

STRUCT_EXTEND(sol_obj, sol_datatype,
    data_type type_id;
);
typedef sol_datatype* SolDatatype;

DEFINE_DATATYPE(sol_num,
    double value;
);
typedef sol_num* SolNumber;
extern SolNumber Number;

DEFINE_DATATYPE(sol_string,
    char* value;
);
typedef sol_string* SolString;
extern SolString String;

DEFINE_DATATYPE(sol_bool,
    bool value;
);
typedef sol_bool* SolBoolean;
extern SolBoolean Boolean;


SolNumber sol_num_create(double value);

SolString sol_string_create(char* value);

SolBoolean sol_bool_create(bool value);
SolBoolean sol_bool_value_of(SolObject obj);

/**
 * Creates a string literal that represents a datatype.
 * @param type
 * @return
 */
char* sol_datatype_string(data_type type);

#endif	/* SOLTYPES_H */

