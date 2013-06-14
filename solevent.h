//
//  solevent.h
//  libsol
//
//  Created by Jake King on 2/11/13.
//
//

#ifndef SOLEVENT_H
#define SOLEVENT_H

#include <uv.h>
#include "sol.h"
#include "solfunc.h"
#include "solop.h"

typedef SolObject SolEvent;
extern SolEvent Event;

struct sol_event_listener_list {
    SolFunction callback;
    struct sol_event_listener_list* next;
};
struct sol_event_listener {
    char* type;
    struct sol_event_listener_list* listeners;
    struct sol_event_listener_list* listeners_end;
    UT_hash_handle hh;
};

bool sol_event_has_work(void);

void sol_event_loop_run(void);
void sol_event_loop_stop(void);

void sol_event_listener_add(SolObject object, char* type, SolFunction callback);
void sol_event_listener_remove(SolObject object, char* type, SolFunction callback);
void sol_event_listener_dispatch(SolObject object, char* type, SolEvent event);

#endif
