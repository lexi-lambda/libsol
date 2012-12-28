/* 
 * File:   soltoken.h
 * Author: Jake
 *
 * Created on November 18, 2012, 9:37 PM
 */

#ifndef SOLTOKEN_H
#define	SOLTOKEN_H

#include "sol.h"
#include "uthash.h"


typedef struct sol_token {
    sol_obj super; // extend sol_obj
    char* identifier;
} sol_token;
typedef sol_token* SolToken;

typedef struct token_pool_entry {
    char* identifier;
    SolObject* value;
    UT_hash_handle hh;
} token_pool_entry;
typedef token_pool_entry* TokenPoolEntry;
typedef token_pool_entry* TokenMap;

typedef struct token_pool {
    TokenMap pool;
    struct token_pool* next;
} token_pool;
typedef token_pool* TokenPool;


/**
 * The current (top), most local token pool.
 */
extern TokenPool local_token_pool;


/**
 * Creates a new token object with the given identifier.
 * @param identifier
 * @return new token
 */
SolToken sol_token_create(char* identifier);

/**
 * Creates a new scope level.
 */
void sol_token_pool_push();

/**
 * Pushes a pre-existing map onto the pool as a scope level.
 * @param pool
 */
void sol_token_pool_push_m(TokenMap pool);

/**
 * Destroys the current scope level.
 */
void sol_token_pool_pop();

/**
 * Gets a "snapshot" of the current token pool, a single map of all token
 * pointer values.
 * @return token map
 */
TokenMap sol_token_pool_snapshot();

/**
 * Registers a token in the current pool.
 * @param token
 * @param obj
 */
void sol_token_register(char* token, SolObject obj);

/**
 * Sets the most local token binding to point to obj.
 * @param token
 * @param obj
 */
void sol_token_update(char* token, SolObject obj);

/**
 * Resolves a token to a value.
 * @param token
 * @return value
 */
SolObject sol_token_resolve(char* token);

#endif	/* SOLTOKEN_H */

