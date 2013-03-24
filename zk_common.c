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

#include"zk_common.h"

//returned pointer to the same memory
char *
last_path (const char *path)
{

    int size = strlen (path);
    int iter;
    for (iter = size - 1; iter >= 0; iter--) {
        if (path[iter] == '/') {
            return (char *) &(path[iter + 1]);
        }
    }
    return NULL;

}

// the result , ie start doesnt have a null at the end
//location 1 means first from the last
void
part_path (char *path, int location, char **start, int *siz)
{

    int st = strlen (path);
    int end;
    int en = 0;

    int size = strlen (path);
    int iter;
    for (iter = size - 1; iter >= 0; iter--) {
        if (path[iter] == '/') {
            end = st;
            st = iter;
            en++;
        }
        if (en == location) {

            break;
        }
    }

    *start = &(path[st + 1]);
    *siz = -st + end - 1;
}

//the initial struct is given
void
duplicate_String_vector (struct String_vector * duplicate, struct String_vector * vector)
{

    allocate_String_vector (duplicate, vector->count);

    int i;
    for (i = 0; i < vector->count; i++) {
        duplicate->data[i] = calloc (strlen (vector->data[i]) + 1, 1);
        memcpy (duplicate->data[i], vector->data[i],
                strlen (vector->data[i]) + 1);
    }
}
