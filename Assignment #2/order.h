#ifndef __order__h__
#define __order__h__

#include <sys/time.h>

#define MAX_CATEGORIES 3

#define MARGARITA 0
#define PEPPERONI 1
#define SPECIAL 2

typedef struct _order 
{
	char pizzes[MAX_CATEGORIES];
	char apostasi;
	int  order_pid;
	char free_beer;
	struct timeval start;
}order;

#endif 
