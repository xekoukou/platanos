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

#include"zk_sync.h"

void
comp_res_init (comp_res_t ** comp_res, char *comp_name)
{

    (*comp_res) = malloc (sizeof (comp_res));
    (*comp_res)->res_list = zlist_new ();

    assert (strlen (comp_name) < 8);
    strcpy ((*comp_res)->name, comp_name);

    zlist_autofree ((*comp_res)->res_list);

}

void
comp_res_add (comp_res_t * comp_res, char *res_name)
{

    zlist_append (comp_res->res_list, res_name);

}

int
comp_res_remove (comp_res_t * comp_res, char *res_name)
{

    char *iter = zlist_first (comp_res->res_list);

    while (iter) {

        if (strcmp (res_name, iter) == 0) {
            zlist_remove (comp_res->res_list, iter);
            return 1;
        }

        iter = zlist_next (comp_res->res_list);
    }

    return 0;
}


void
comp_res_destroy (comp_res_t ** comp_res)
{

    zlist_destroy (&((*comp_res)->res_list));
    free (*comp_res);

    *comp_res = NULL;
}

void
sync_init (sync_t ** sync, char *key, oz_updater_t * updater, int old)
{

    (*sync) = malloc (sizeof (sync_t));
    assert (strlen (key) < 16);
    strcpy ((*sync)->key, key);
    (*sync)->comp_res_list = zlist_new ();
    (*sync)->old = old;

    int i;
    for (i = 0; i < updater->computers.count; i++) {
        comp_res_t *comp_res;
        comp_res_init (&comp_res, updater->computers.data[i]);
        zlist_append ((*sync)->comp_res_list, comp_res);

        int j;
        for (j = 0; j < updater->w_resources[i].count; j++) {
            comp_res_add (comp_res, updater->w_resources[i].data[j]);
        }
    }
}

int
sync_remove_comp (sync_t * sync, char *comp_name)
{

    comp_res_t *iter = zlist_first (sync->comp_res_list);
    while (iter) {
        if (strcmp (iter->name, comp_name) == 0) {
            zlist_remove (sync->comp_res_list, iter);
            comp_res_destroy (&iter);
            return 1;
        }
        iter = zlist_next (sync->comp_res_list);
    }
    return 0;
}

int
sync_remove_res (sync_t * sync, char *comp_name, char *res_name)
{

    int found_it = 0;
    comp_res_t *iter = zlist_first (sync->comp_res_list);
    while (iter) {
        if (strcmp (iter->name, comp_name) == 0) {
            found_it = 1;
            break;
        }
        iter = zlist_next (sync->comp_res_list);
    }

    if (found_it == 1) {

        int result = comp_res_remove (iter, res_name);
        if (zlist_size (iter->res_list) == 0) {
            zlist_remove (sync->comp_res_list, iter);
            comp_res_destroy (&iter);
        }
        return result;
    }
    else {
        return 0;
    }
}

int
sync_ready (sync_t * sync)
{
    if (zlist_size (sync->comp_res_list) == 0) {
        return 1;
    }
    return 0;
}
