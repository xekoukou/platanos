#include <stdint.h>
#include <stddef.h>
#include "tree/tree.h"
#include"router.h"
#include"aknowledgements.h"

int
cmp_hkey_t (struct _hkey_t *first, struct _hkey_t *second)
{

    if (first->prefix > second->prefix) {
	return 1;
    }
    else {
	if (first->prefix < second->prefix) {
	    return -1;
	}
	else {
	    if (first->suffix > second->suffix) {
		return 1;
	    }
	    else {
		if (first->suffix < second->suffix) {
		    return -1;
		}
		else {
		    return 0;
		}
	    }
	}
    }

}



int
cmp_interval_t (struct interval_t *first, struct interval_t *second)
{

    cmp_hkey (&(first->end), &(second->end));

}

RB_GENERATE (intervals_t, interval_t, field, cmp_interval_t);

int intervals_init(intervals_t ** intervas){
RB_INIT (*intervals);
}


int
interval_init (interval_t ** interval, struct _hkey_t *start,
	       struct _hkey_t *end)
{
    *interval = (interval_t *) malloc (sizeof (interval_t));
    memcpy ((*interval)->start, start, sizeof (struct _hkey_t));
    memcpy ((*interval)->end, end, sizeof (struct _hkey_t));

}

int
interval_add (intervals_t * intervals, interval_t * interval)
{

    interval_t *interval_above = NULL;
    interval_t *interval_below = NULL;
    struct _hkey_t temp;


    memcpy (temp, interval->end, sizeof (struct _hkey_t));
    memcpy (interval->end, interval->start, sizeof (struct _hkey_t));

    interval_above = RB_FIND (intervals_t, intervals, interval);


    memcpy (interval->end, temp, sizeof (struct _hkey_t));

    if (interval_above) {

	memcpy (interval->start, interval_above->start,
		sizeof (struct _hkey_t));
	RB_REMOVE (intervals_t, intervals, interval_above);
	free (interval_above);
    }

    interval_below = RB_NFIND (intervals_t, intervals, interval);

    if (interval_below
	&& memcmp (interval_below->start, interval->end,
		   sizeof (struct _hkey_t))) {
	memcpy (interval->end, interval_below->end, sizeof (struct _hkey_t));
	RB_REMOVE (intervals_t, intervals, interval_below);
	free (interval_below);
    }

    RB_INSERT (intervals_t, intervals, interval);


}

//returns true if it is contained inside one integral
interval_t *
interval_contained (intervals_t * intervals, interval_t * interval)
{

    interval_t *inside = NULL;
    struct _hkey_t temp;


    memcpy (temp, interval->end, sizeof (struct _hkey_t));
    memcpy (interval->end, interval->start, sizeof (struct _hkey_t));

    inside = RB_NFIND (intervals_t, intervals, interval);


    memcpy (interval->end, temp, sizeof (struct _hkey_t));

    if ((cmp_hkey (&(interval->start), &(inside->start)) > 0)
	&& (cmp_hkey (&(inside->end), &(interval->end)) > 0)) {
	return inside;

    }
    return NULL;


}

//returns true if an interval was removed
int
interval_remove (intervals_t * intervals, interval_t * interval)
{
    interval_t *inside = interval_contained (intervals, interval);

    if (inside) {
	interval_t *up = NULL;
	interval_t *down = NULL;

	if (cmp_hkey (&(interval->start), &(inside->start)) != 0) {
	    interval_init (&up, &(inside->start), &(interval->start));
	}
	if (cmp_hkey (&(interval->end), &(inside->end)) != 0) {
	    interval_init (&down, &(interval->end), &(inside->end));
	}

	RB_REMOVE (intervals_t, intervals, inside);
	free (inside);

	if (up) {
	    RB_INSERT (intervals_t, intervals, up);
	}
	if (down) {
	    RB_INSERT (intervals_t, intervals, down);
	}

	return 1;
    }
    return 0;

}


//return true if it belongs to one of the intervals
int
interval_belongs (intervals_t * intervals, uint64_t key)
{

    struct interval_t search;
    struct interval_t *result;

    MurmurHash3_x64_128 ((void *) &key, sizeof (uint64_t), 0,
			 (void *) &(search.end));

    result = RB_NFIND (intervals_t, intervals, &search);
    if (result) {
	if (cmp_hkey (&(search.end), &(result->start)) > 0) {
	    return 1;

	}

    }

    return 0;
}


//
//
//
// EVENT OBJECT
//
//
//

int
cmp_ev_ac (event_t * event, action_t * action)
{
    if (event->give == 1) {
	return 0;
    }

    if (cmp_hkey (&(event->start), &(action->start)) != 0) {
	return 0;
    }
    if (cmp_hkey (&(event->end), &(action->end)) != 0) {
	return 0;
    }

    if (strcmp (event->key, action->key) != 0) {
	return 0;
    }

    return 1;
}

event_t *
events_search (zlist_t * events, action_t * action)
{
    event_t *iter = zlist_first (events);
    while (iter) {
	if (cmp_ev_ac (iter, action)) {
	    return iter;
	}
	iter = zlist_next (events);
    }
    return NULL;

}


//returns 1 if there was an event that was erased by this action
int
events_update (zlist_t * events, action_t * action)
{
    event_t *result = events_search (events, action);
    if (result) {
	zlist_remove (events, result);

	free (result);

	return 1;
    }
    return 0;


}

//returns the first event that can occur with the current nodes that we possess or NULL
event_t *
events_send (zlist_t * events, intervals_t * intevals)
{
    interval_t *interval;
    interval_init (&interval, NULL, NULL);

    event_t *iter = zlist_first (events);
    while (iter) {
	if (iter->give) {
	    memcpy (interval->start, iter->start, sizeof (struct _hkey_t));
	    memcpy (interval->end, iter->end, sizeof (struct _hkey_t));

	    if (interval_contained (intervals, interval)) {
		return iter;
	    }

	}
	iter = zlist_next (events);
    }

    return NULL;

}

action_t *
actions_search (zlist_t * actions, event_t * event)
{
    action_t *iter = zlist_first (actions);
    while (iter) {
	if (cmp_ev_ac (event, iter)) {
	    return iter;
	}
	iter = zlist_next (actions);
    }
    return NULL;

}

//returns 1 if there was an event that was erased by this action
int
actions_update (zlist_t * actions, event_t * event)
{
    action_t *result = actions_search (actions, event);
    if (result) {
	zlist_remove (actions, result);

	free (result);

	return 1;
    }
    return 0;


}
