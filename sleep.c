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





#include"sleep.h"
#include<czmq.h>
#include<stdlib.h>
#include"tree/tree.h"


RB_GENERATE (smsg_rb_t, smsg_t, field, cmp_smsg_t);


int
cmp_smsg_t (struct smsg_t *first, struct smsg_t *second)
{
    if (first->expiry > second->expiry) {
	return 1;
    }
    else if (first->expiry < second->expiry) {
	return -1;
    }
    return 0;
}


void
sleep_init (sleep_t ** sleep)
{
    *sleep = malloc (sizeof (sleep_t));
    RB_INIT (&((*sleep)->smsg_rb));
    (*sleep)->timeout = -1;
    (*sleep)->pr_time = zclock_time ();
    (*sleep)->min = NULL;
}

void
sleep_add (sleep_t * sleep, zmsg_t * msg, int64_t delay, unsigned short wb)
{
    struct smsg_t *smsg = (struct smsg_t *) malloc (sizeof (struct smsg_t));
    smsg->expiry = delay + zclock_time ();
    smsg->msg = msg;
    smsg->wb = wb;

    //update min
    if (sleep->min == NULL) {
	sleep->min = smsg;
    }
    if (cmp_smsg_t (sleep->min, smsg) > 0) {
	sleep->min = smsg;
    }
//insert into rbtree
    RB_INSERT (smsg_rb_t, &(sleep->smsg_rb), smsg);



    //update timeout if the new msgs needs to be sent sooner than the previous sooner msg

    sleep->timeout = sleep->timeout + sleep->pr_time - zclock_time ();
    sleep->pr_time = zclock_time ();

    if (sleep->timeout > delay) {
	sleep->timeout = delay;
    }
    else {
	if (sleep->timeout < 0) {
	    sleep->timeout = 0;
	}
    }
}


//awake and pop one msg
//this should be put into a loop
//if the msg is null it means it cannot pop more msgs
zmsg_t *
sleep_awake (sleep_t * sleep, unsigned short *wb)
{


//check if the timeout has expired, we need to send the msg of min
//update the timeout
//if there is no other msg set the timeout to -1;

    zmsg_t *msg = NULL;

    if (sleep->min != NULL) {
	if ((sleep->min->expiry - zclock_time ()) < 0) {

	    struct smsg_t *temp;

	    temp = sleep->min;
	    //update the min
	    sleep->min = RB_PARENT (sleep->min, field);
	    msg = temp->msg;
	    *wb = temp->wb;
	    //free the previous min smsg
	    free (temp);

	}
    }

//update the timeout 

    if (sleep->min == NULL) {
	sleep->timeout = -1;
    }
    else {
	sleep->timeout = sleep->min->expiry - zclock_time ();
	sleep->pr_time = zclock_time ();

	if (sleep->timeout < 0) {
	    sleep->timeout = 0;
	}


    }
    return msg;



}
