
#include <string.h>

#include "sol.h"
#include "sollist.h"

SolList sol_list_create(bool object_mode) {
    return sol_obj_clone_type((SolObject) List, &(struct sol_list_raw){
            object_mode,
            NULL,
            NULL,
            0,
            NULL,
            0
        }, sizeof(sol_list));
}

void sol_list_add_obj(SolList list, SolObject obj) {
    sol_list_node* new_node = malloc(sizeof(*new_node));
    new_node->value = sol_obj_retain(obj);
    new_node->next = NULL;
    if (list->first == NULL || list->last == NULL) {
        new_node->prev = NULL;
        list->first = new_node;
        list->last = new_node;
    } else {
        new_node->prev = list->last;
        list->last->next = new_node;
        list->last = new_node;
    }
    list->length++;
}

SolObject sol_list_get_obj(SolList list, int index) {
    // use fastest iteration direction
    if (index < list->length / 2) {
        list->current_index = 0;
        list->current = list->last;
        while (list->current_index < index) {
            list->current_index++;
            list->current = list->current->next;
        }
    } else {
        list->current_index = list->length - 1;
        list->current = list->first;
        while (list->current_index > index) {
            list->current_index--;
            list->current = list->current->prev;
        }
    }
    return list->current->value;
}

SolList sol_list_slice(SolList list, int start, int end) {
    // create new list
    SolList new_list = sol_list_create(list->object_mode);
    
    // walk to starting position
    sol_list_node* node = list->first;
    for (int i = 0; i < start; i++) {
        node = node->next;
    }
    
    // copy nodes
    for (int i = 0; i < end; i++) {
        sol_list_add_obj(new_list, node->value);
        node = node->next;
    }
    
    return new_list;
}

SolList sol_list_slice_s(SolList list, int start) {
    // create new list
    SolList new_list = sol_list_create(list->object_mode);
    
    // walk to starting position
    sol_list_node* node = list->first;
    for (int i = 0; i < start; i++) {
        node = node->next;
    }
    
    // copy nodes
    while (node != NULL) {
        sol_list_add_obj(new_list, node->value);
        node = node->next;
    }
    
    return new_list;
}

SolList sol_list_slice_e(SolList list, int end) {
    // create new list
    SolList new_list = sol_list_create(list->object_mode);
    
    // copy nodes
    sol_list_node* node = list->first;
    for (int i = 0; i < end; i++) {
        sol_list_add_obj(new_list, node->value);
        node = node->next;
    }
    
    return new_list;
}
