
#ifndef SOLLIST_H
#define	SOLLIST_H

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
    int length;
);
typedef sol_list* SolList;
extern SolList List;

// same as sol_obj_frozen, but typed for lists
STRUCT_EXTEND(sol_obj, sol_list_frozen,
    SolList value;
);
typedef sol_list_frozen* SolListFrozen;

#define SOL_LIST_ITR(list, el, i)                                                            \
    for (sol_list_node* el, * _once = (sol_list_node*) 1; _once; _once = (sol_list_node*) 0) \
    for (int i, _once = 1; _once; _once = 0)                                                 \
    for (                                                                                    \
        el = (list)->first, i = 0;                                                           \
        el && i < (list)->length;                                                            \
        el = el->next, i++                                                                   \
    )

#define SOL_LIST_ITR_PARALLEL(list1, el1, i1, list2, el2, i2)                                                                           \
    for (sol_list_node* el1, * _once = (sol_list_node*) 1; _once; _once = (sol_list_node*) 0) for (int i1, _once = 1; _once; _once = 0) \
    for (sol_list_node* el2, * _once = (sol_list_node*) 1; _once; _once = (sol_list_node*) 0) for (int i2, _once = 1; _once; _once = 0) \
    for (                                                                                                                               \
        el1 = (list1)->first, el2 = (list2)->first, i1 = i2 = 0;                                                                        \
        (el1 && i1 < (list1)->length) || (el2 && i2 < (list2)->length);                                                                 \
        el1 = el1 ? el1->next : NULL, el2 = el2 ? el2->next : NULL, i1++, i2++                                                          \
    )

/**
 * Creates a new, empty list object.
 * @param object_mode
 * @return new list
 */
SolList sol_list_create(bool object_mode);

/**
 * Creates a new, empty list object and returns it frozen.
 * @param object_mode
 * @param output for list, or NULL
 * @return new list
 */
SolListFrozen sol_list_create_frozen(bool object_mode);

/**
 * Adds an object to the end of this list.
 * @param list
 * @param obj
 */
void sol_list_add_obj(SolList list, SolObject obj);

/**
 * Removes an object at the specified index and return that object.
 * @param list
 * @param index
 * @return removed object
 */
SolObject sol_list_remove_obj(SolList list, int index);

/**
 * Adds an object at the given index in this list, pushing all the subsequent elements out of the way.
 * @param list
 * @param obj
 * @param index
 */
void sol_list_insert_object(SolList list, SolObject obj, int index);

/**
 * Gets an object from the list at an index. Note that this is an O(n) operation,
 * since the list must be iterated through to find the object.
 * @param list
 * @param index
 * @return object
 */
SolObject sol_list_get_obj(SolList list, int index);

/**
 * Searches the array for the element specified and returns the index of the first occurrence,
 * or -1 if it doesn't exist.
 * @param list
 * @param obj
 * @return index
 */
int sol_list_index_of(SolList list, SolObject obj);

/**
 * Creates a new list and copies a set of values from the original.
 * The values pointed to are the same, but the new list is a wholly
 * separate structure. The items are retained.
 * @param list
 * @param start
 * @param end
 * @return new list
 */
SolList sol_list_slice(SolList list, int start, int end);

/**
 * Creates a new list and copies a set of values from the original.
 * The values pointed to are the same, but the new list is a wholly
 * separate structure. The items are retained.
 * @param list
 * @param start
 * @return new list
 */
SolList sol_list_slice_s(SolList list, int start);

/**
 * Creates a new list and copies a set of values from the original.
 * The values pointed to are the same, but the new list is a wholly
 * separate structure. The items are retained.
 * @param list
 * @param end
 * @return new list
 */
SolList sol_list_slice_e(SolList list, int end);

#endif	/* SOLLIST_H */
