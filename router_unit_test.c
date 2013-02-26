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




#include"router.h"
#include<string.h>
#include<stdio.h>
#include<stdlib.h>

int
main ()
{

    int passed = 1;


    node_t *node1;
    node_t *node2;
    node_t *node3;
    node_t *node4;
    node_t *node5;
    node_t *node6;
    node_init (&node1, "this1 key", 4, 6);
    node_init (&node2, "this2 key", 2, 3);
    node_init (&node3, "this3 key", 4, 5);
    node_init (&node4, "this4 key", 6, 2);
    node_init (&node5, "this5 key", 1, 8);
    node_init (&node6, "this6 key", 3, 9);
    router_t *router;
    router_init (&router);

    router_set_repl (router, 4);

    router_add (router, node1);
    node_set_alive (node1, 1);
    router_add (router, node2);
    node_set_alive (node2, 1);
    router_add (router, node3);
    node_set_alive (node3, 1);
    router_add (router, node4);
    node_set_alive (node4, 1);
    router_add (router, node5);
    node_set_alive (node5, 1);
    router_add (router, node6);
    node_set_alive (node6, 1);

    int repl;
    router_get_repl (router, &repl);

    printf ("\n repl is :%d", repl);

    char **rkey = (char **) malloc (repl);

    int iter = 0;

    for (iter = 0; iter < repl; iter++) {
        rkey[iter] = (char *) malloc (100);
    }
    int nreturned;
    router_dbroute (router, "a radom key", rkey, &nreturned);

    printf ("\n it returned %d", nreturned);

    while (nreturned) {
        printf ("\n key '%s'", rkey[nreturned - 1]);
        nreturned--;
    }

    node_t *result;
    router_fnode (router, "this1 key", &result);

    router_delete (router, result);
    router_destroy (router);

    if (passed) {
        printf ("\nTest passed");
    }

}
