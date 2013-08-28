
#include <uv.h>
#include <stdio.h>
#include "solevent.h"
#include "soltypes.h"
#include "uthash.h"

static unsigned int listener_count = 0;

bool sol_event_has_work(void) {
    return listener_count > 0;
}

void sol_event_loop_run(void) {
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void sol_event_loop_stop(void) {
    uv_stop(uv_default_loop());
}

void sol_event_listener_add(SolObject object, char* type, SolFunction callback) {
    listener_count++;
    struct sol_event_listener* new_listener;
    // check if already exists
    HASH_FIND_STR(object->listeners, type, new_listener);
    if (new_listener == NULL) {
        new_listener = malloc(sizeof(*new_listener));
        new_listener->type = strdup(type);
        new_listener->listeners = NULL;
        HASH_ADD_KEYPTR(hh, object->listeners, new_listener->type, strlen(new_listener->type), new_listener);
    }
    if (new_listener->listeners == NULL) {
        new_listener->listeners = new_listener->listeners_end = malloc(sizeof(*new_listener->listeners_end));
    } else {
        new_listener->listeners_end->next = malloc(sizeof(*new_listener->listeners_end));
        new_listener->listeners_end = new_listener->listeners_end->next;
    }
    new_listener->listeners_end->callback = (SolFunction) sol_obj_retain((SolObject) callback);
    new_listener->listeners_end->next = NULL;
}

void sol_event_listener_remove(SolObject object, char* type, SolFunction callback) {
    listener_count--;
    struct sol_event_listener* listener;
    HASH_FIND_STR(object->listeners, type, listener);
    if (listener == NULL) return;
    
    struct sol_event_listener_list* listener_list_prev = NULL;
    for (struct sol_event_listener_list* listener_list = listener->listeners; listener_list != NULL; listener_list = listener_list->next) {
        if (listener_list->callback == callback) {
            if (listener_list_prev != NULL) {
                listener_list_prev->next = listener_list->next;
            } else {
                listener->listeners = listener_list->next;
            }
            if (listener_list->next == NULL) {
                listener->listeners_end = listener_list_prev;
            }
            sol_obj_release((SolObject) listener_list->callback);
            free(listener_list);
            return;
        }
        listener_list_prev = listener_list;
    }
}

void sol_event_listener_dispatch(SolObject object, char* type, SolEvent event) {
    struct sol_event_listener* listener;
    HASH_FIND_STR(object->listeners, type, listener);
    if (listener == NULL) return;
    
    SolListFrozen args = (SolListFrozen) sol_obj_retain((SolObject) sol_list_create_frozen(false));
    sol_list_add_obj(args->value, event);
    
    for (struct sol_event_listener_list* listener_list = listener->listeners; listener_list != NULL; listener_list = listener_list->next) {
        sol_obj_release(sol_func_execute(listener_list->callback, args->value, nil));
    }
    
    sol_obj_release((SolObject) args);
}
