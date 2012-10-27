#include"sleep.h"

RB_GENERATE (smsg_rb_t, smsg_t, field, cmp_smsg_t);



int
main ()
{

    sleep_t *sleep;
    unsigned short wb;

    sleep_init (&sleep);

    zmsg_t *msg = zmsg_new ();

    sleep_add (sleep, msg, 10, 1);
    sleep_add (sleep, msg, 15, 0);
    int iter = 0;
    while (sleep_awake (sleep, &wb)) {
	iter++;
    }
    printf ("\nnumber of msgs:%d", iter);

    zclock_sleep (11);

    while (sleep_awake (sleep, &wb)) {
	iter++;
    }
    printf ("\nnumber of msgs:%d", iter);


    zclock_sleep (10);

    while (sleep_awake (sleep, &wb)) {
	iter++;
    }
    printf ("\nnumber of msgs:%d", iter);

}
