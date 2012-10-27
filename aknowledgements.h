#ifndef OCTOPUS_AKNOWLEDGEMENTS_H_
#define OCTOPUS_AKNOWLEDGEMENTS_H_


#include <stdint.h>
#include <stddef.h>
#include "tree/tree.h"
#include"router.h"
#include"czmq.h"

//TODO We dont need an rb tree here
//but it works anyway

//intervals are disjoint by default
//we sort them by the end
struct interval_t
{


    struct _hkey_t start;
    struct _hkey_t end;
      RB_ENTRY (interval_t) field;

}

RB_HEAD (intervals_t, interval_t);
RB_PROTOTYPE (intervals_t, interval_t, field, cmp_interval_t);

struct event_t
{
    struct _hkey_t start;
    struct _hkey_t end;
    int give;
    char key[100];
}
struct action_t
{				//actions are only vertices that are received
    struct _hkey_t start;
    struct _hkey_t end;
    char key[100];
}


typedef struct event_t event_t;
typedef struct action_t action_t;


typedef struct interval_t interval_t;
typedef struct intervals_t intervals_t;

int intervals_init (intervals_t * intervals);
int
interval_init (interval_t ** interval, struct _hkey_t *start,
	       struct _hkey_t *end);

int interval_add (intervals_t * intervals, interval_t * interval);

//returns true if it is contained inside one integral
interval_t *interval_contained (intervals_t * intervals,
				interval_t * interval);

//removes and free interval
//returns true if there was a change
int interval_remove (intervals_t * intervals, interval_t * interval);

//return true if it belongs to one of the intervals
int interval_belongs (intervals_t * intervals, uint64_t key);

event_t *events_search (zlist_t * events, action_t * action);

//this is used when a node dies and we need to clean the previous events
int events_remove (zlist_t * events, node_t * node);

//returns 1 if there was an event that was erased by this action
int events_update (zlist_t * events, action_t * action);




//returns the first event that can occur with the current nodes that we possess or NULL
event_t *events_possible (zlist_t * events, intervals_t * intevals);



int event_possible (event_t * event, intervals_t * intervals);

action_t *actions_search (zlist_t * actions, event_t * event);

//returns 1 if there was an action that was erased by this event
int actions_update (zlist_t * actions, event_t * event);

//action is only created by a received msg
int action_init (action_t ** action, zmsg_t * msg);

#endif
