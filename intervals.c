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


#include "intervals.h"



RB_GENERATE (intervals_rb_t, interval_t, field, cmp_interval_t);

void
intervals_init (intervals_t ** intervals)
{
    *intervals = malloc (sizeof (intervals_t));
    RB_INIT (&((*intervals)->intervals_rb));
    (*intervals)->circle = 0;
}

void
intervals_add (intervals_t * intervals, interval_t * interval)
{
    assert (intervals->circle == 0);

    interval_t *interval_above = NULL;
    interval_t *interval_below = NULL;
    struct _hkey_t temp;


    memcpy (&temp, &(interval->end), sizeof (struct _hkey_t));
    memcpy (&(interval->end), &(interval->start), sizeof (struct _hkey_t));

    interval_above =
        RB_FIND (intervals_rb_t, &(intervals->intervals_rb), interval);

    memcpy (&(interval->end), &temp, sizeof (struct _hkey_t));

    if (interval_above) {

        memcpy (&(interval->start), &(interval_above->start),
                sizeof (struct _hkey_t));
        RB_REMOVE (intervals_rb_t, &(intervals->intervals_rb), interval_above);
        free (interval_above);
    }

    interval_below =
        RB_NFIND (intervals_rb_t, &(intervals->intervals_rb), interval);

//in case the end of the interval is in the other side of the circle
    if (interval_below == NULL) {

        interval_below = RB_MIN (intervals_rb_t, &(intervals->intervals_rb));
    }

    if (interval_below) {
        if (memcmp (&(interval_below->start), &(interval->end),
                    sizeof (struct _hkey_t)) == 0) {
            memcpy (&(interval->end), &(interval_below->end),
                    sizeof (struct _hkey_t));
            RB_REMOVE (intervals_rb_t, &(intervals->intervals_rb),
                       interval_below);
            free (interval_below);
        }
        else {
            assert (interval_belongs_h (interval, &(interval_below->start)) ==
                    0);

        }
    }
    if (memcmp (&(interval->start), &(interval->end),
                sizeof (struct _hkey_t)) == 0) {
        intervals->circle = 1;
        free (interval);
    }
    else {

        RB_INSERT (intervals_rb_t, &(intervals->intervals_rb), interval);
    }

}





interval_t *
intervals_contained (intervals_t * intervals, interval_t * interval,
                     int *circle)
{

    *circle = intervals->circle;

    if (*circle) {
        return NULL;

    }


//check whether it is reversed
    int reversed = 0;
    int iter_reversed;

    if ((cmp_hkey_t (&(interval->start), &(interval->end)) > 0)) {
        reversed = 1;
    }

    interval_t *iter = NULL;
    RB_FOREACH (iter, intervals_rb_t, &(intervals->intervals_rb)) {
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
    int circle;
    interval_t *inside = intervals_contained (intervals, interval, &circle);

    if (circle) {
        interval_t *complement;
        interval_init (&complement, &(interval->end), &(interval->start));
        RB_INSERT (intervals_rb_t, &(intervals->intervals_rb), complement);
        free (interval);
        intervals->circle = 0;
        return 1;

    }

    interval_t *up = NULL;
    interval_t *down = NULL;


    if (inside) {

        if (cmp_hkey_t (&(interval->start), &(inside->start)) != 0) {
            interval_init (&up, &(inside->start), &(interval->start));
        }
        if (cmp_hkey_t (&(interval->end), &(inside->end)) != 0) {
            interval_init (&down, &(interval->end), &(inside->end));
        }

        RB_REMOVE (intervals_rb_t, &(intervals->intervals_rb), inside);
        free (inside);

        if (up) {
            RB_INSERT (intervals_rb_t, &(intervals->intervals_rb), up);
        }
        if (down) {
            RB_INSERT (intervals_rb_t, &(intervals->intervals_rb), down);
        }
        free (interval);

        return 1;
    }
    free (interval);
    return 0;

}



int
intervals_belongs_h (intervals_t * intervals, struct _hkey_t *hkey)
{


    struct interval_t search;
    struct interval_t *result;


    memcpy (&(search.end), hkey, sizeof (struct _hkey_t));

    result = RB_NFIND (intervals_rb_t, &(intervals->intervals_rb), &search);

    int side = 0;
    if (result == NULL) {
        result = RB_MIN (intervals_rb_t, &(intervals->intervals_rb));
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




void
intervals_print (intervals_t * intervals)
{

    fprintf (stderr, "\nMy intervals are:");

    interval_t *iter = NULL;
    RB_FOREACH (iter, intervals_rb_t, &(intervals->intervals_rb)) {

        fprintf (stderr, "\nstart: %lu %lu \nend: %lu %lu",
                 iter->start.prefix, iter->start.suffix, iter->end.prefix,
                 iter->end.suffix);


    }
}
