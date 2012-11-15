/*
    Copyright contributors as noted in the AUTHORS file.
                
    This file is part of PLATANOS.

    PLATANOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU Affero General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
            
    PLATANOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.
        
    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/






#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include "tree/tree.h"
#include"router.h"
#include"aknowledgements.h"
#include"czmq.h"
#include"MurmurHash/MurmurHash3.h"


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

    return cmp_hkey_t (&(first->end), &(second->end));

}

RB_GENERATE (intervals_t, interval_t, field, cmp_interval_t);

void
intervals_init (intervals_t ** intervals)
{
    *intervals = malloc (sizeof (intervals_t));
    RB_INIT (*intervals);
}


void
interval_init (interval_t ** interval, struct _hkey_t *start,
	       struct _hkey_t *end)
{
    *interval = malloc (sizeof (interval_t));
    memcpy (&((*interval)->start), start, sizeof (struct _hkey_t));
    memcpy (&((*interval)->end), end, sizeof (struct _hkey_t));

}

void
interval_minit (interval_t ** interval, zmsg_t * msg)
{

    *interval = malloc (sizeof (interval_t));

    zframe_t *frame = zmsg_first (msg);
    frame = zmsg_next (msg);
    frame = zmsg_next (msg);
    memcpy (&((*interval)->start), zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&((*interval)->end), zframe_data (frame), zframe_size (frame));


}



//check for the initial implementation(intervals_belong_h)
int
interval_belongs_h (interval_t * interval, struct _hkey_t *hkey)
{


    struct interval_t search;


    memcpy (&(search.end), hkey, sizeof (struct _hkey_t));


    int side = 0;
    if (cmp_hkey_t (&(interval->end), hkey) < 0) {
	side = 1;
    }
    else {
	if (cmp_hkey_t (&(interval->end), hkey) == 0) {
	    return 1;
	}
    }


    //check whether it is reversed
    int reversed = 0;

    if ((cmp_hkey_t (&(interval->start), &(interval->end)) > 0)) {
	reversed = 1;
    }
    if (!reversed && !side) {
	if (cmp_hkey_t (&(search.end), &(interval->start)) >= 0) {
	    return 1;

	}
    }

    if (reversed && !side) {
	return 1;

    }
    if (!reversed && side) {
	return 0;

    }
    if (reversed && side) {
	if (cmp_hkey_t (&(search.end), &(interval->start)) >= 0) {
	    return 1;

	}
    }

//in case there is no interval
    return 0;
}

int
interval_belongs (interval_t * interval, uint64_t key)
{

    struct _hkey_t hkey;

    MurmurHash3_x64_128 ((const void *) &key, (int) sizeof (uint64_t), 0,
			 (void *) &hkey);

    return interval_belongs_h (interval, &hkey);

}






void
intervals_add (intervals_t * intervals, interval_t * interval)
{

    interval_t *interval_above = NULL;
    interval_t *interval_below = NULL;
    struct _hkey_t temp;


    memcpy (&temp, &(interval->end), sizeof (struct _hkey_t));
    memcpy (&(interval->end), &(interval->start), sizeof (struct _hkey_t));

    interval_above = RB_FIND (intervals_t, intervals, interval);

    memcpy (&(interval->end), &temp, sizeof (struct _hkey_t));

    if (interval_above) {

	memcpy (&(interval->start), &(interval_above->start),
		sizeof (struct _hkey_t));
	RB_REMOVE (intervals_t, intervals, interval_above);
	free (interval_above);
    }

    interval_below = RB_NFIND (intervals_t, intervals, interval);

//in case the end of the interval is in the other side of the circle
    if (interval_below == NULL) {

	interval_below = RB_MIN (intervals_t, intervals);
    }

    if (interval_below) {
	if (memcmp (&(interval_below->start), &(interval->end),
		    sizeof (struct _hkey_t)) == 0) {
	    memcpy (&(interval->end), &(interval_below->end),
		    sizeof (struct _hkey_t));
	    RB_REMOVE (intervals_t, intervals, interval_below);
	    free (interval_below);
	}
	else {
	    assert (interval_belongs_h (interval, &(interval_below->start))==
		    0);

	}
    }

    RB_INSERT (intervals_t, intervals, interval);


}

interval_t *
intervals_contained (intervals_t * intervals, interval_t * interval)
{
//check whether it is reversed
    int reversed = 0;
    int iter_reversed;

    if ((cmp_hkey_t (&(interval->start), &(interval->end)) > 0)) {
	reversed = 1;
    }

    interval_t *iter = NULL;
    RB_FOREACH (iter, intervals_t, intervals) {
	iter_reversed = 0;
	if ((cmp_hkey_t (&(iter->start), &(iter->end)) > 0)) {
	    iter_reversed = 1;
	}
//4 cases

	if ((!reversed && !iter_reversed) || (reversed && iter_reversed)) {

	    if ((cmp_hkey_t (&(interval->start), &(iter->start)) >= 0)
		&& (cmp_hkey_t (&(iter->end), &(interval->end)) >= 0)) {
		return iter;

	    }


	}
	if ((!reversed && iter_reversed)) {

	    if ((cmp_hkey_t (&(iter->start), &(interval->start)) <= 0)) {
		return iter;

	    }
	    if ((cmp_hkey_t (&(interval->end), &(iter->end)) <= 0)
		) {
		return iter;

	    }



	}





    }

    return NULL;


}


//returns true if an interval was removed
int
intervals_remove (intervals_t * intervals, interval_t * interval)
{
    interval_t *inside = intervals_contained (intervals, interval);
    interval_t *up = NULL;
    interval_t *down = NULL;


    if (inside) {

	if (cmp_hkey_t (&(interval->start), &(inside->start)) != 0) {
	    interval_init (&up, &(inside->start), &(interval->start));
	}
	if (cmp_hkey_t (&(interval->end), &(inside->end)) != 0) {
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
	free (interval);

	return 1;
    }
    free (interval);
    return 0;

}


//return true if it belongs to one of the intervals
int
intervals_belongs_h (intervals_t * intervals, struct _hkey_t *hkey)
{


    struct interval_t search;
    struct interval_t *result;


    memcpy (&(search.end), hkey, sizeof (struct _hkey_t));

    result = RB_NFIND (intervals_t, intervals, &search);

    int side = 0;
    if (result == NULL) {
	result = RB_MIN (intervals_t, intervals);
	side = 1;
    }


    if (result) {

	if (cmp_hkey_t (&(result->end), hkey) == 0) {
	    return 1;
	}


	//check whether it is reversed
	int reversed = 0;

	if ((cmp_hkey_t (&(result->start), &(result->end)) > 0)) {
	    reversed = 1;
	}
	if (!reversed && !side) {
	    if (cmp_hkey_t (&(search.end), &(result->start)) >= 0) {
		return 1;

	    }
	}

	if (reversed && !side) {
	    return 1;

	}
	if (!reversed && side) {
	    return 0;

	}
	if (reversed && side) {
	    if (cmp_hkey_t (&(search.end), &(result->start)) >= 0) {
		return 1;

	    }
	}








    }
//in case there is no interval
    return 0;
}


//return true if it belongs to one of the intervals
int
intervals_belongs (intervals_t * intervals, uint64_t key)
{

    struct _hkey_t hkey;

    MurmurHash3_x64_128 ((const void *) &key, sizeof (uint64_t), 0,
			 (void *) &hkey);

    return intervals_belongs_h (intervals, &hkey);

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

    if (cmp_hkey_t (&(event->start), &(action->start)) != 0) {
	return 0;
    }
    if (cmp_hkey_t (&(event->end), &(action->end)) != 0) {
	return 0;
    }

    if (strcmp (event->key, action->key) != 0) {
	return 0;
    }

    return 1;
}

void
event_init (event_t ** event, struct _hkey_t start, struct _hkey_t end,
	    int give, char *key)
{
    assert (memcmp (&start, &end, sizeof (struct _hkey_t)) != 0);

    *event = malloc (sizeof (event_t));
    (*event)->give = give;
    memcpy (&((*event)->start), &start, sizeof (struct _hkey_t));
    memcpy (&((*event)->end), &end, sizeof (struct _hkey_t));
    if (key) {
	strcpy ((*event)->key, key);
    }
    else {
	strcpy ((*event)->key, "\0");
    }
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

void
events_remove (zlist_t * events, node_t * node)
{

    event_t *iter = zlist_first (events);

    while (iter) {

	if (strcmp (node->key, iter->key) == 0) {
	    zlist_remove (events, iter);
	    free (iter);
	}


	iter = zlist_next (events);
    }



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
events_possible (zlist_t * events, intervals_t * intervals)
{
    interval_t *interval;
    interval_init (&interval, NULL, NULL);

    event_t *iter = zlist_first (events);
    while (iter) {
	if (iter->give) {
	    memcpy (&(interval->start), &(iter->start),
		    sizeof (struct _hkey_t));
	    memcpy (&(interval->end), &(iter->end), sizeof (struct _hkey_t));

	    if (intervals_contained (intervals, interval)) {
		return iter;
	    }

	}
	iter = zlist_next (events);
    }
    free (interval);
    return NULL;

}


int
event_possible (event_t * event, intervals_t * intervals)
{
    assert (event->key);

    interval_t *interval;
    interval_init (&interval, &(event->start), &(event->end));

    if (event->give) {

	if (intervals_contained (intervals, interval)) {
	    return 1;
	}

    }
    free (interval);
    return 0;

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

//action is only created by a received msg
void
action_minit (action_t ** action, zmsg_t * msg)
{

    *action = malloc (sizeof (action_t));

    zframe_t *frame = zmsg_first (msg);
    memcpy ((*action)->key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&((*action)->start), zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&((*action)->end), zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);

}
