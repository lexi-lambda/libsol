
#include <string.h>
#include "sol.h"
#include "sollist.h"

SolList sol_list_create(bool object_mode) {
    return sol_obj_clone_type((SolObject) List, &(struct sol_list_raw){
            object_mode,
            NULL,
            NULL,
            0,
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

SolObject sol_list_remove_obj(SolList list, int index) {
    if (index == 0) {
        sol_list_node* removed_node = list->first;
        list->first = list->first->next;
        removed_node->next->prev = NULL;
        SolObject ret = removed_node->value;
        free(removed_node);
        return ret;
        
    }
    if (index == list->length - 1) {
        sol_list_node* removed_node = list->last;
        list->last = list->last->prev;
        removed_node->prev->next = NULL;
        SolObject ret = removed_node->value;
        free(removed_node);
        return ret;
    }
    // use fastest iteration direction
    sol_list_node* current;
    if (index < list->length / 2) {
        int current_index = 0;
        current = list->last;
        while (current_index < index) {
            current_index++;
            current = current->next;
        }
    } else {
        int current_index = list->length - 1;
        current = list->first;
        while (current_index > index) {
            current_index--;
            current = current->prev;
        }
    }
    sol_list_node* removed_node = current;
    removed_node->prev->next = removed_node->next;
    removed_node->next->prev = removed_node->prev;
    SolObject ret = removed_node->value;
    free(removed_node);
    return ret;
}

void sol_list_insert_object(SolList list, SolObject obj, int index) {
    if (list->first == NULL || list->last == NULL || index == list->length) {
        sol_list_add_obj(list, obj);
        return;
    }
    sol_list_node* new_node = malloc(sizeof(*new_node));
    new_node->value = sol_obj_retain(obj);
    if (index == 0) {
        new_node->next = list->first;
        new_node->prev = NULL;
        list->first = new_node;
    } else {
        for (sol_list_node* current = list->first; index > 0; index--, current = current->next) {
            new_node->next = current->next;
            new_node->prev = current;
            current->next = new_node;
        }
    }
    list->length++;
}

SolObject sol_list_get_obj(SolList list, int index) {
    // use fastest iteration direction
    sol_list_node* current;
    if (index < list->length / 2) {
        int current_index = 0;
        current = list->first;
        while (current_index < index) {
            current_index++;
            current = current->next;
        }
    } else {
        int current_index = list->length - 1;
        current = list->last;
        while (current_index > index) {
            current_index--;
            current = current->prev;
        }
    }
    return sol_obj_retain(current->value);
}

int sol_list_index_of(SolList list, SolObject obj) {
    SOL_LIST_ITR(list, current, i) {
        if (sol_obj_equals(current->value, obj))
            return i;
    }
    return -1;
}

SolList sol_list_slice(SolList list, int start, int end) {
    // create new list
    SolList new_list = (SolList) sol_obj_retain((SolObject) sol_list_create(list->object_mode));
    
    // walk to starting position
    sol_list_node* node = list->first;
    int i = 0;
    for (; i < start; i++) {
        node = node->next;
    }
    
    // copy nodes
    for (; i < end; i++) {
        sol_list_add_obj(new_list, node->value);
        node = node->next;
    }
    
    return new_list;
}

SolList sol_list_slice_s(SolList list, int start) {
    // create new list
    SolList new_list = (SolList) sol_obj_retain((SolObject) sol_list_create(list->object_mode));
    
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
    SolList new_list = (SolList) sol_obj_retain((SolObject) sol_list_create(list->object_mode));
    
    // copy nodes
    sol_list_node* node = list->first;
    for (int i = 0; i < end; i++) {
        sol_list_add_obj(new_list, node->value);
        node = node->next;
    }
    
    return new_list;
}
