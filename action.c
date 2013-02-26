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

#include "action.h"

//action is only created by a received msg
void
action_minit (action_t ** action, zmsg_t * msg)
{

    *action = malloc (sizeof (action_t));

    zframe_t *frame = zmsg_first (msg);
    frame = zmsg_next (msg);
    memcpy ((*action)->key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&((*action)->start), zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&((*action)->end), zframe_data (frame), zframe_size (frame));


}
