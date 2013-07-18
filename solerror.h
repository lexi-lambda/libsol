
#ifndef SOLERROR_H
#define SOLERROR_H

#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdbool.h>

/* try, catch, finally */

#define SOL_TRY \
    for (int _sol_error_jmp_code = -1; _sol_error_jmp_code == -1;)                                  \
        if ((_sol_error_jmp_code = setjmp(_sol_error_try_frames[++_sol_error_try_frame_top])) == 0) \
            for (; 1; longjmp(_sol_error_try_frames[_sol_error_try_frame_top], -1))

#define SOL_CATCH(type, err) \
    else if (sol_error_get_current_error() && sol_error_extends_type(sol_error_get_current_error(), &(type)))           \
        for (sol_error* (err) = sol_error_get_current_error(); (err) != NULL;                                           \
            sol_error_free_current_error(), (err) = NULL, longjmp(_sol_error_try_frames[_sol_error_try_frame_top], -1))

#define SOL_FINALLY \
    else for (int _sol_error_run_once = 1; _sol_error_run_once; \
        _sol_error_run_once = 0,                                \
        (_sol_error_jmp_code != -1                              \
            ? rethrow()                                         \
            : --_sol_error_try_frame_top),                      \
        _sol_error_jmp_code = 0)

#define SOL_THROW(err) \
    sol_error_throw_error((err))

#define SOL_THROW_MESSAGE(type, format, ...) \
    throw (sol_error_create(&(type), (format), ##__VA_ARGS__))

#define SOL_RETHROW() \
    sol_error_rethrow_current_error()

#define try                          SOL_TRY
#define catch(type, err)             SOL_CATCH(type, err)
#define finally                      SOL_FINALLY
#define throw(err)                   SOL_THROW(err)
#define throw_msg(type, format, ...) SOL_THROW_MESSAGE(type, format, ##__VA_ARGS__)
#define rethrow()                    SOL_RETHROW()

/* exception type declarations */

typedef struct sol_error_type {
    struct sol_error_type* parent;
    char* name;
    int error_code;
} sol_error_type;

typedef struct sol_error {
    sol_error_type* type;
    char* message;
    void* data;
} sol_error;

#define SOL_ERROR_DECLARE(name) extern sol_error_type (name)
#define SOL_ERROR_DEFINE(name, super) sol_error_type (name) = {(super), #name, __COUNTER__ + 1}

/* globals */

extern int _sol_error_try_frame_top;
extern jmp_buf _sol_error_try_frames[];

/* functions */

sol_error* sol_error_create(sol_error_type* type, char* format, ...);
void sol_error_free(sol_error* error);
void sol_error_throw_error(sol_error* error);

sol_error* sol_error_get_current_error();
void sol_error_rethrow_current_error();
void sol_error_free_current_error();

bool sol_error_extends_type(sol_error* error, sol_error_type* type);
bool sol_error_type_extends_type(sol_error_type* error_type, sol_error_type* type);

/* error types */

SOL_ERROR_DECLARE(Error);
SOL_ERROR_DECLARE(BytecodeError);
SOL_ERROR_DECLARE(TypeError);

/* helper macros */

#define SOL_REQUIRE_TYPE(obj, type) \
    if (!(obj) || ((SolObject) obj)->type_id != (type))                                                                            \
        for (; 1; throw_msg (TypeError, "expected %s, found %s", sol_type_string((type)), sol_obj_type_string((SolObject) (obj))))

#define SOL_REQUIRE_DATATYPE(obj, type) \
    if (!(obj) || ((SolObject) obj)->type_id != TYPE_SOL_DATATYPE || ((SolDatatype) (obj))->type_id != (type))                         \
        for (; 1; throw_msg (TypeError, "expected %s, found %s", sol_datatype_string((type)), sol_obj_type_string((SolObject) (obj))))

#endif
