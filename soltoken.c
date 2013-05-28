
#include <stdio.h>
#include <string.h>
#include "soltoken.h"

TokenPoolEntry sol_token_resolve_entry(char* token);

TokenPool local_token_pool = NULL;

SolToken sol_token_create(char* identifier) {
    return sol_obj_clone_type((SolObject) Token, &(struct sol_token_raw){
            strdup(identifier)
        }, sizeof(sol_token));
}

void sol_token_pool_push() {
    TokenPool old_token_pool = local_token_pool;
    local_token_pool = malloc(sizeof(*local_token_pool));
    local_token_pool->next = old_token_pool;
    local_token_pool->pool = NULL;
}

void sol_token_pool_push_m(TokenMap pool) {
    TokenPool old_token_pool = local_token_pool;
    local_token_pool = malloc(sizeof(*local_token_pool));
    local_token_pool->next = old_token_pool;
    local_token_pool->pool = pool;
}

void sol_token_pool_pop() {
    // repoint local_token_pool
    TokenPool old_token_pool = local_token_pool;
    local_token_pool = local_token_pool->next;
    
    // clean up old pool and contents
    if (old_token_pool->pool != NULL) {
        TokenPoolEntry current_token, tmp;
        HASH_ITER(hh, old_token_pool->pool, current_token, tmp) {
            HASH_DEL(old_token_pool->pool, current_token);
            sol_obj_release(current_token->binding->value);
            if (--current_token->binding->retain_count <= 0) free(current_token->binding);
            free(current_token->identifier);
            free(current_token);
        }
    }
    free(old_token_pool);
}

TokenMap sol_token_pool_pop_m() {
    // repoint local_token_pool
    TokenPool old_token_pool = local_token_pool;
    TokenMap old_token_map = old_token_pool->pool;
    local_token_pool = local_token_pool->next;
    free(old_token_pool);
    
    // return current pool
    return old_token_map;
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
                // create a new entry (we need a new hash handle)
                TokenPoolEntry new_token = malloc(sizeof(*new_token));
                new_token->identifier = strdup(current_token->identifier);
                new_token->binding = current_token->binding;
                new_token->binding->retain_count++;
                sol_obj_retain(new_token->binding->value);
                HASH_ADD_KEYPTR(hh, snapshot, new_token->identifier, strlen(new_token->identifier), new_token);
            }
        }
    }
    return snapshot;
}

SolObject sol_token_register(char* token, SolObject obj) {
    TokenPoolEntry new_token;
    sol_obj_retain(obj);
    // check if entry already exists
    HASH_FIND_STR(local_token_pool->pool, token, new_token);
    if (new_token == NULL) {
        new_token = calloc(1, sizeof(*new_token));
        new_token->identifier = strdup(token);
        HASH_ADD_KEYPTR(hh, local_token_pool->pool, new_token->identifier, strlen(new_token->identifier), new_token);
    } else {
        sol_obj_release(new_token->binding->value);
        if (--new_token->binding->retain_count <= 0) free(new_token->binding);
    }
    // bind to existing binding if the object is a token
    if (obj->type_id == TYPE_SOL_TOKEN && !strcmp(((SolToken) obj)->identifier, "undefined")) {
        sol_obj_release(obj);
        obj = NULL;
    }
    if (obj != NULL && obj->type_id == TYPE_SOL_TOKEN) {
        TokenPoolEntry resolved_object = sol_token_resolve_entry(((SolToken) obj)->identifier);
        if (resolved_object != NULL) {
            new_token->binding = resolved_object->binding;
            new_token->binding->retain_count++;
            sol_obj_retain(new_token->binding->value);
            sol_obj_release(obj);
            return new_token->binding->value;
        }
    }
    new_token->binding = malloc(sizeof(*new_token->binding));
    new_token->binding->value = obj;
    new_token->binding->retain_count = 1;
    return obj;
}

SolObject sol_token_update(char* token, SolObject obj) {
    // loop through token pools to find the token
    TokenPool current_pool = local_token_pool;
    do {
        TokenPoolEntry resolved_token;
        HASH_FIND_STR(current_pool->pool, token, resolved_token);
        if (resolved_token != NULL) {
            for (int i = 0; i < resolved_token->binding->retain_count; i++) {
                sol_obj_retain(obj);
                sol_obj_release(resolved_token->binding->value);
            }
            resolved_token->binding->value = obj;
            return obj;
        }
    } while ((current_pool = current_pool->next) != NULL);
    return NULL;
}

TokenPoolEntry sol_token_resolve_entry(char* token) {
    if (!strcmp(token, "undefined")) return NULL;
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
    return resolved_token == NULL ? NULL : sol_obj_retain(resolved_token->binding->value);
}
