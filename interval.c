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


#include "interval.h"


int
cmp_interval_t (struct interval_t *first, interval_t * second)
{

    return cmp_hkey_t (&(first->end), &(second->end));

}


void
interval_init (interval_t ** interval, struct _hkey_t *start,
               struct _hkey_t *end)
{
    *interval = malloc (sizeof (interval_t));
    if (start) {
        memcpy (&((*interval)->start), start, sizeof (struct _hkey_t));
    }
    if (end) {
        memcpy (&((*interval)->end), end, sizeof (struct _hkey_t));
    }

}

void
interval_minit (interval_t ** interval, zmsg_t * msg)
{

    *interval = malloc (sizeof (interval_t));

    zframe_t *frame = zmsg_first (msg);
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
