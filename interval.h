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


#ifndef OCTOPUS_INTERVAL_H_
#define OCTOPUS_INTERVAL_H_

#include "hkey.h"
#include "tree/tree.h"
#include <czmq.h>
#include"MurmurHash/MurmurHash3.h"

//TODO We dont need an rb tree here
//but it works anyway

//intervals are disjoint by default
//we sort them by the end

//an interval should never have end and start equal
struct interval_t
{


    struct _hkey_t start;
    struct _hkey_t end;
      RB_ENTRY (interval_t) field;

};

typedef struct interval_t interval_t;

void
interval_init (interval_t ** interval, struct _hkey_t *start,
               struct _hkey_t *end);

//the msg is not deleted, it will be used to create action_t later
void interval_minit (interval_t ** interval, zmsg_t * msg);

//the interval is closed
int interval_belongs_h (interval_t * interval, struct _hkey_t *hkey);

int interval_belongs (interval_t * interval, uint64_t key);

int interval_identical(interval_t *first, interval_t *second);

//this is used for the red-black tree
int cmp_interval_t (struct interval_t *first, struct interval_t *second);

interval_t *interval_dup (interval_t * interval);

#endif
