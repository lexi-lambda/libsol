
#include <stdio.h>
#include "solevent.h"
#include "soltypes.h"
#include "uthash.h"

static struct event_base* base;
static struct sol_event_initializer* initializers = NULL;
static struct sol_event_listener* listeners = NULL;

bool sol_event_has_work(void) {
    return listeners != NULL;
}

void sol_event_loop_create(void) {
    base = event_base_new();
}

void sol_event_loop_run(void) {
    event_base_dispatch(base);
}

void sol_event_loop_stop(void) {
    event_base_loopexit(base, NULL);
}

void sol_event_loop_add(struct sol_event* event) {
    struct event* ev = event_new(base, event->fd, event->flags, event->callback, event->arg);
    event_add(ev, event->timeout);
}

void sol_event_initializer_register(char* type, sol_event_initializer_fn initializer) {
    struct sol_event_initializer* new_initializer;
    // check if already exists
    HASH_FIND_STR(initializers, type, new_initializer);
    if (new_initializer != NULL) {
        fprintf(stderr, "Fatal error: conflict in registering custom event type '%s'.", type);
        exit(EXIT_FAILURE);
    }
    new_initializer = malloc(sizeof(*new_initializer));
    new_initializer->type = strdup(type);
    new_initializer->intializer = initializer;
    HASH_ADD_KEYPTR(hh, initializers, new_initializer->type, strlen(new_initializer->type), new_initializer);
}

void sol_event_listener_add(char* type, SolFunction callback) {
    struct sol_event_listener* new_listener;
    // check if already exists
    HASH_FIND_STR(listeners, type, new_listener);
    if (new_listener == NULL) {
        new_listener = malloc(sizeof(*new_listener));
        new_listener->type = strdup(type);
        new_listener->listeners = NULL;
        HASH_ADD_KEYPTR(hh, listeners, new_listener->type, strlen(new_listener->type), new_listener);
    }
    if (new_listener->listeners == NULL) {
        new_listener->listeners = new_listener->listeners_end = malloc(sizeof(*new_listener->listeners_end));
    } else {
        new_listener->listeners_end->next = malloc(sizeof(*new_listener->listeners_end));
        new_listener->listeners_end = new_listener->listeners_end->next;
    }
    new_listener->listeners_end->callback = (SolFunction) sol_obj_retain((SolObject) callback);
    new_listener->listeners_end->next = NULL;
    // handle custom event initializers
    struct sol_event_initializer* initializer;
    HASH_FIND_STR(initializers, type, initializer);
    if (initializer != NULL) {
        struct sol_event event = initializer->intializer();
        sol_event_loop_add(&event);
        HASH_DEL(initializers, initializer);
        free(initializer->type);
        free(initializer);
    }
}

void sol_event_listener_remove(char* type, SolFunction callback) {
    struct sol_event_listener* listener;
    HASH_FIND_STR(listeners, type, listener);
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

void sol_event_listener_dispatch(char* type, SolEvent event) {
    struct sol_event_listener* listener;
    HASH_FIND_STR(listeners, type, listener);
    if (listener == NULL) return;
    
    SolList args = (SolList) sol_obj_retain((SolObject) sol_list_create(false));
    sol_list_add_obj(args, event);
    
    for (struct sol_event_listener_list* listener_list = listener->listeners; listener_list != NULL; listener_list = listener_list->next) {
        sol_obj_release(sol_func_execute(listener_list->callback, args, nil));
    }
    
    sol_obj_release((SolObject) args);
}
