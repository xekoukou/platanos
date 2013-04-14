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


#include "db_balance.h"

void
db_balance_init (db_balance_t ** balance, dbo_t * dbo,
                 void *router_bl, void *self_bl, char *key)
{

    *balance = malloc (sizeof (db_balance_t));
    (*balance)->dbo = dbo;
    (*balance)->router_bl = router_bl;
    (*balance)->self_bl = self_bl;
    memset ((*balance)->self_key, 0, 16);
    strcpy ((*balance)->self_key, key);
    intervals_init (&((*balance)->intervals));
    intervals_init (&((*balance)->locked_intervals));

}
