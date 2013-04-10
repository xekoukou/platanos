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


#ifndef OCTOPUS_INTERVALS_H_
#define OCTOPUS_INTERVALS_H_

#include "interval.h"

RB_HEAD (intervals_rb_t, interval_t);
RB_PROTOTYPE (intervals_rb_t, interval_t, field, cmp_interval_t);

struct intervals_t
{
    struct intervals_rb_t intervals_rb;

    int circle;
};


typedef struct intervals_t intervals_t;
typedef struct intervals_rb_t intervals_rb_t;


void intervals_init (intervals_t ** intervals);

void intervals_add (intervals_t * intervals, interval_t * interval);

//returns the interval if it is contained inside one interval or NULL
interval_t *intervals_contained (intervals_t * intervals,
                                 interval_t * interval, int *circle);

//removes and free interval
//returns true if there was a change
int intervals_remove (intervals_t * intervals, interval_t * interval);

int intervals_belongs_h (intervals_t * intervals, struct _hkey_t *hkey);

//return true if it belongs to one of the intervals
int intervals_belongs (intervals_t * intervals, uint64_t key);

void intervals_print (intervals_t * intervals);

intervals_t *intervals_dup (intervals_t * intervals);

intervals_t *intervals_difference (intervals_t * intervals,
                                   intervals_t * rintervals);


#endif
