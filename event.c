

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


#include "event.h"

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
    (*event)->dead = 0;
    memcpy (&((*event)->start), &start, sizeof (struct _hkey_t));
    memcpy (&((*event)->end), &end, sizeof (struct _hkey_t));
    if (key) {
        strcpy ((*event)->key, key);
    }
    else {
        strcpy ((*event)->key, "\0");
    }
}

int
event_possible (event_t * event, intervals_t * intervals)
{
    assert (event->key);

    interval_t *interval;
    interval_init (&interval, &(event->start), &(event->end));

    if (event->give) {
        int circle;
        interval_t *is_contained =
            intervals_contained (intervals, interval, &circle);
        if (circle || is_contained) {
            return 1;
        }

    }
    free (interval);
    return 0;

}


void event_clean(event_t *event, khash_t (vertices) * hash){

interval_t * interval;
interval_init(&interval,&(event->start),&(event->end));

khint_t hiter;

for (hiter = kh_begin (balance->hash); hiter != kh_end (balance->hash);
             ++hiter) {
            if (!kh_exist (balance->hash, hiter))
                continue;

            key = kh_key (balance->hash, hiter);
            if (interval_belongs (on_give->interval, key)) {
                
                //delete it from the hash
                kh_del (vertices, balance->hash, hiter);


}}
}
