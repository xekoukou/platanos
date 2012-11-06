#include"tree/tree.h"
#include<czmq.h>

//TODO needs to reduce the size of time or not
struct smsg_t
{
    int64_t expiry;
    unsigned short wb;		//boolean 1 or 0
    zmsg_t *msg;
      RB_ENTRY (smsg_t) field;
};


int cmp_smsg_t (struct smsg_t *first, struct smsg_t *second);

RB_HEAD (smsg_rb_t, smsg_t);
RB_PROTOTYPE (smsg_rb_t, smsg_t, field, cmp_smsg_t);


typedef struct
{
    struct smsg_rb_t smsg_rb;
    struct smsg_t *min;		//used to reduce latency in finding the minimum item
    int64_t pr_time;		//previous_time
    int64_t timeout;		//time till next expiry

} sleep_t;


void sleep_init (sleep_t ** sleep);

void sleep_add (sleep_t * sleep, zmsg_t * msg, int64_t delay,
		unsigned short wb);

//returns null when there are no more msgs to give
//dont give null msgs, it will brake it
zmsg_t *sleep_awake (sleep_t * sleep, unsigned short *wb);
