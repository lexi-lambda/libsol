
#include <stdio.h>
#include "solutils.h"
#include "solerror.h"

int _sol_error_try_frame_top = -1;
jmp_buf _sol_error_try_frames[512];

static sol_error* current_error = NULL;

SOL_ERROR_DEFINE(Error, NULL);
SOL_ERROR_DEFINE(BoundsError, &Error);
SOL_ERROR_DEFINE(BytecodeError, &Error);
SOL_ERROR_DEFINE(TypeError, &Error);

sol_error* sol_error_create(sol_error_type* type, char* format, ...) {
    va_list args;
    va_start(args, format);
    
    sol_error* error = malloc(sizeof(*error));
    error->type = type;
    vasprintf(&error->message, format, args);
    error->data = NULL;
    
    va_end(args);
    return error;
}

void sol_error_free(sol_error* error) {
    free(error->message);
    free(error);
}

void sol_error_throw_error(sol_error* error) {
    if (_sol_error_try_frame_top < 0) {
        fprintf(stderr, "  Terminating due to uncaught error\n  %s: %s\n", error->type->name, error->message);
        exit(EXIT_FAILURE);
    }
    current_error = error;
    longjmp(_sol_error_try_frames[_sol_error_try_frame_top], error->type->error_code);
}

sol_error* sol_error_get_current_error() {
    return current_error;
}

void sol_error_rethrow_current_error() {
    _sol_error_try_frame_top--;
    sol_error_throw_error(current_error);
}

void sol_error_free_current_error() {
    sol_error_free(current_error);
    current_error = NULL;
}

bool sol_error_extends_type(sol_error* error, sol_error_type* type) {
    return sol_error_type_extends_type(error->type, type);
}

bool sol_error_type_extends_type(sol_error_type* error_type, sol_error_type* type) {
    do {
        if (type->error_code == error_type->error_code) return true;
    } while ((error_type = error_type->parent) != NULL);
    return false;
}
