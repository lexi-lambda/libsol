
#include "sol.h"
#include "soltoken.h"


void sol_runtime_init() {
    Object = malloc(sizeof(*Object));
    nil = sol_obj_clone(Object);
    
    local_token_pool = calloc(1, sizeof(*local_token_pool));
}

void sol_runtime_destroy() {
}
