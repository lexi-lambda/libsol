
#include "soltoken.h"

TokenPoolEntry sol_token_resolve_entry(char* token);

TokenPool local_token_pool = NULL;

SolToken sol_token_create(char* identifier) {
    return sol_obj_clone_type((SolObject) Token, &(struct sol_token_raw){
            identifier
        }, sizeof(sol_token));
}

void sol_token_pool_push() {
    TokenPool old_token_pool = local_token_pool;
    local_token_pool = malloc(sizeof(token_pool_entry));
    local_token_pool->next = old_token_pool;
    local_token_pool->pool = NULL;
}

void sol_token_pool_push_m(TokenMap pool) {
    TokenPool old_token_pool = local_token_pool;
    local_token_pool = malloc(sizeof(token_pool_entry));
    local_token_pool->next = old_token_pool;
    local_token_pool->pool = pool;
}

void sol_token_pool_pop() {
    // repoint local_token_pool
    TokenPool old_token_pool = local_token_pool;
    local_token_pool = old_token_pool->next;
    
    // clean up old pool and contents
    TokenPoolEntry current_token, tmp;
    HASH_ITER(hh, local_token_pool->pool, current_token, tmp) {
        HASH_DEL(local_token_pool->pool, current_token);
    }
}

TokenMap sol_token_pool_snapshot() {
    // store the final map
    TokenMap snapshot = NULL;
    // loop through all levels of token pool
    for (TokenPool pool = local_token_pool; pool != NULL; pool = pool->next) {
        // get all the actual key-value mappings inside the pool
        TokenPoolEntry current_token, tmp;
        HASH_ITER(hh, pool->pool, current_token, tmp) {
            // if the snapshot doesn't have the mapping, add it
            TokenPoolEntry read_token;
            HASH_FIND_STR(snapshot, current_token->identifier, read_token);
            if (read_token == NULL) {
                HASH_ADD_KEYPTR(hh, snapshot, current_token->identifier, strlen(current_token->identifier), current_token);
            }
        }
    }
    return snapshot;
}

void sol_token_register(char* token, SolObject obj) {
    TokenPoolEntry new_token;
    // check if entry already exists
    HASH_FIND_STR(local_token_pool->pool, token, new_token);
    if (new_token == NULL) {
        new_token = malloc(sizeof(token_pool_entry));
        new_token->identifier = token;
        HASH_ADD_KEYPTR(hh, local_token_pool->pool, new_token->identifier, strlen(new_token->identifier), new_token);
    }
    // bind to existing binding if the object is a token
    if (obj->type_id == TYPE_SOL_TOKEN) {
        TokenPoolEntry resolved_object = sol_token_resolve_entry(((SolToken) obj)->identifier);
        if (resolved_object != NULL) {
            new_token->value = resolved_object->value;
            return;
        }
    }
    new_token->value = malloc(sizeof(*new_token->value));
    memcpy(new_token->value, &obj, sizeof(*new_token->value));
}

void sol_token_update(char* token, SolObject obj) {
    // loop through token pools to find the token
    TokenPool current_pool = local_token_pool;
    do {
        TokenPoolEntry resolved_token;
        HASH_FIND_STR(current_pool->pool, token, resolved_token);
        if (resolved_token != NULL) {
            // TODO: release object multiple times if multiple tokens are bound to it
            sol_obj_release(*resolved_token->value);
            *resolved_token->value = sol_obj_retain(obj);
            return;
        }
    } while ((current_pool = current_pool->next) != NULL);
}

TokenPoolEntry sol_token_resolve_entry(char* token) {
    // loop through token pools to find the token
    TokenPool current_pool = local_token_pool;
    do {
        TokenPoolEntry resolved_token;
        HASH_FIND_STR(current_pool->pool, token, resolved_token);
        if (resolved_token != NULL) {
            return resolved_token;
        }
    } while ((current_pool = current_pool->next) != NULL);
    
    // if it cannot be found, return NULL
    return NULL;
}

SolObject sol_token_resolve(char* token) {
    TokenPoolEntry resolved_token = sol_token_resolve_entry(token);
    return resolved_token == NULL ? NULL : *resolved_token->value;
}
