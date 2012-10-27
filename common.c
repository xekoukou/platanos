#include<string.h>

//returned pointer to the same memory
char *
last_path (char *path)
{

    int size = strlen (path);
    int iter;
    for (iter = size - 1; iter >= 0; iter--) {
	if (path[iter] == '/') {
	    return &(path[iter + 1]);
	}
    }

}

// the result , ie start doesnt have a null at the end
//location 1 means first from the last
void
part_path (char *path, int location, char **start, int *siz)
{

    int st = strlen (path) - 1;
    int end = -1;
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
    *siz = st - end - 1;
}
