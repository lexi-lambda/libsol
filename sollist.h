/* 
 * File:   sollist.h
 * Author: Jake
 *
 * Created on November 18, 2012, 8:56 PM
 */

#ifndef SOLLIST_H
#define	SOLLIST_H

#include <stdbool.h>
#include "sol.h"

typedef struct sol_list_node {
    struct sol_list_node *next;
    struct sol_list_node *prev;
    SolObject value;
} sol_list_node;

STRUCT_EXTEND(sol_obj, sol_list,
    bool object_mode;
    sol_list_node *first;
    sol_list_node *last;
    int current_index;
    sol_list_node *current;
    int length;
    int freezeCount;
);
typedef sol_list* SolList;
extern SolList List;


#define SOL_LIST_ITR_BEGIN(list) (list)->current = (list)->first;     \
                                 (list)->current_index = 0;           \
                                 do {

#define SOL_LIST_ITR_END(list)   (list)->current_index++;             \
                                 } while (((list)->current = (list)->current->next) != NULL && (list)->current_index < (list)->length);

/**
 * Creates a new, empty list object.
 * @return new list
 */
SolList sol_list_create();

/**
 * Adds an object to the end of this list. This should only be used to populate
 * a list; at runtime, lists should be immutable.
 * @param list
 * @param obj
 */
void sol_list_add_obj(SolList list, SolObject obj);

/**
 * Gets an object from the list at an index. Note that this is an O(n) operation,
 * since the list must be iterated through to find the object.
 * @param list
 * @param index
 * @return object
 */
SolObject sol_list_get_obj(SolList list, int index);

/**
 * Creates a new list containing only a portion of the original. The returned
 * list's values are not copied, and remain the same from the original list.
 * @param list
 * @param start
 * @param end
 * @return new list
 */
SolList sol_list_get_sublist(SolList list, int start, int end);

/**
 * Creates a new list containing only a potion of the origin. This method will
 * yield the same result as list_get_sublist with the list's length as the second parameter.
 * @param list
 * @param start
 * @return new list
 */
SolList sol_list_get_sublist_s(SolList list, int start);

/**
 * Creates a new list containing only a potion of the origin. This method will
 * yield the same result as list_get_sublist with 0 as the first parameter.
 * @param list
 * @param end
 * @return new list
 */
SolList sol_list_get_sublist_e(SolList list, int end);

#endif	/* SOLLIST_H */
