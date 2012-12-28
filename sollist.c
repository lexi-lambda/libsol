
#include <string.h>

#include "sol.h"
#include "sollist.h"


const sol_list DEFAULT_LIST = {
    {
        TYPE_SOL_LIST, NULL, NULL, NULL
    }, NULL, NULL, 0, NULL, 0
};


SolList sol_list_create() {
    SolList list_value = malloc(sizeof(*list_value));
    memcpy(list_value, &DEFAULT_LIST, sizeof(*list_value));
    return list_value;
}

void sol_list_add_obj(SolList list, SolObject obj) {
    sol_list_node* new_node = malloc(sizeof(*new_node));
    new_node->value = obj;
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

SolList sol_list_get_sublist(SolList list, int start, int end) {
    // copy list
    sol_list list_value = *list;
    sol_list new_value = list_value;
    SolList new_list = &new_value;
    
    // advance start pointer
    for (int i = 0; i < start; i++) {
        new_list->first = new_list->first->next;
    }
    
    // advance end pointer
    for (int i = new_list->length; i > end; i--) {
        new_list->last = new_list->last->prev;
    }
    
    // update length
    new_list->length -= start + (new_list->length - end);
    
    return new_list;
}

SolList sol_list_get_sublist_s(SolList list, int start) {
    // copy list
    SolList new_list = malloc(sizeof(*new_list));
    memcpy(new_list, list, sizeof(*new_list));
    
    // advance start pointer
    for (int i = 0; i < start; i++) {
        new_list->first = new_list->first->next;
    }
    
    // update length
    new_list->length -= start;
    
    return new_list;
}

SolList sol_list_get_sublist_e(SolList list, int end) {
    // copy list
    sol_list list_value = *list;
    sol_list new_value = list_value;
    SolList new_list = &new_value;
    
    // advance end pointer
    for (int i = new_list->length; i > end; i--) {
        new_list->last = new_list->last->prev;
    }
    
    // update length
    new_list->length -= new_list->length - end;
    
    return new_list;
}
