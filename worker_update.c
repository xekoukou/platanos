#include<czmq.h>
#include"worker_update.h"
#include"vertex.h"
#include"hash/khash.h"
#include"btree/kbtree.h"
#include"aknowledgements.h"

//TODO this is arbitrary
#define ONGOING_TIMEOUT 10000

int
update_init (update_t ** update, void *dealer, router_t * router,
	     balance_t * balance)
{
    *update = (update_t *) malloc (sizeof (update_t));
    (*update)->id = 0;
    (*update)->dealer = dealer;
    (*update)->router = router;
    (*update)->balance = balance;
    (*update)->compute = compute;
}

int
on_give_init (on_give_t ** on_give, event_t * event, int un_id)
{
    *on_give = (on_give_t *) malloc (sizeof (on_give_t));
    (*on_give)->event = event;
    (*on_give)->unc_vertices = kb_init (vertices, KB_DEFAULT_SIZE);
    (*on_give)->counter = 0;
    (*on_give)->rec_counter = 0;
    (*on_give)->un_id = un_id;
    (*on_give)->last_time = zclock_time ();
}


//destroy this after you have removed the event from the events list
//this will free the event
int
on_give_destroy (on_give_t * on_give)
{
    free (on_give->event);
    assert (on_give->unc_vertices != NULL);
    kb_destroy (vertices, on_give->unc_vertices);
    free (on_give);
}

int
on_receive_init (on_receive_t ** on_receive, zmsg_t * msg)
{
    *on_receive = (on_receive_t *) malloc (sizeof (on_receive_t));

    zframe_t *frame = zmsg_pop (msg);
    memcpy (&((*on_receive)->un_id), zframe_data (frame), sizeof (int));
    zframe_destroy (&frame);
    action_init (&((*on_receive)->action), msg);

    (*on_receive)->m_counters = kb_init (counters, KB_DEFAULT_SIZE);

}

//destroy this after you have inserted the action to the actions list
//or removed the corresponding event
int
on_receive_destroy (on_receive_t * on_receive)
{
    assert (on_receive->m_counters != NULL);
    kb_destroy (counters, on_receive->m_counters);
    free (on_receive);
}





int
balance_init (balance_t ** balance, kbtree_t (vertices) * tree,
	      void *router_bl, void *self_bl)
{

    *balance = (balance_t *) malloc (sizeof (balance_t));
    (*balance)->tree = tree;
    (*balance)->router_bl = router_bl;
    (*balance)->self_bl = self_bl;
    intervals_init (&((*balance)->intervals));
    (*balance)->events = zlist_new ();
    (*balance)->actions = zlist_new ();
    (*balance)->on_gives = zlist_new ();
    (*balance)->on_receives = zlist_new ();
    (*balance)->id = 0;


}
