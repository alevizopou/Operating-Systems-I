/* symperilpsi aparaititwn vivlio8ikwn */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "order.h"
#include "pizzaria.h"

#define MAX_READ_BYTES 128

#define PSISTES 10
#define DELIVERADES 10

#define MAX_ORDERS 100

#define T_SHORT     3
#define T_LONG	    5
#define T_VERYLONG 10 

// plirofories pou moirazontai ola ta threads
static int available_psistes = 10;
static int available_deliver_boys = 10;

// xronoi se seconds
int bake_times[3] = { 1, 2, 3 };

// elegxei an i paraggelai exei argisei. Vazei to apotelesma sto buf
int check_order(order *o, char *buf)
{
	struct timeval end;
	// elegxoume an exoume xeperasei to megisto xrono paraggelias. An nai kanoume doro mia mpyra (free as in beer)
	gettimeofday(&end, NULL);
	int secs = (end.tv_sec - o->start.tv_sec);
	int usecs = (end.tv_usec - o->start.tv_usec);

	int total = secs + usecs/1000000;

	memset(buf, 0, MAX_READ_BYTES);
	// an argisame dinoume mia mpira sto pelati 
	if (total > T_VERYLONG) {
		o->free_beer = 1;
		strcpy(buf, "PIZZA READY. Delivery was late. Sorry. Here get a free beer!\0");
	} else {
		// allios apla ti stelnoume
		strcpy(buf, "PIZZA READY. Hope to see you again!\0");
	}
	return total;
}

void deliver_order(order *o, int sock)
{
	char buf[128];

	int total = check_order(o, buf);


	// pleon exoume teleiosei me ola. Kalo dromo stin(s) pizza(es)
	sleep( ((int)o->apostasi) );

	printf("=================================================\n");
	printf("\t\torder: %d %d %d || distance : %d\n", o->pizzes[MARGARITA], o->pizzes[PEPPERONI], 
			o->pizzes[SPECIAL], o->apostasi); 
	printf("\t\tTotal order time: %d s.\n", total);
	printf("=================================================\n");

	// kai stelnoume piso
	write(sock, buf, strlen(buf));
}

pthread_cond_t bake_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t deliver_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t bake_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t deliver_mutex = PTHREAD_MUTEX_INITIALIZER;

// thread gia psisimo
void *bake_thread(void *arg)
{
	int *sleep_time = (int *)arg;

	// sleep as long as needed to bake
	sleep(*sleep_time);

	// bake finished
	pthread_mutex_lock(&bake_mutex);

	// ara exoume diathesimo psisti. auxanoume ti metavliti kai stelnoume signal
	available_psistes ++; 
	pthread_cond_signal(&bake_cond);

	pthread_mutex_unlock(&bake_mutex);

	return NULL;
}

#define MAX_PIZZAS 3
void *process_order(void *ptr)
{
	int *sock = (int *)ptr;

	int i, j;
	char buf[MAX_READ_BYTES];
	order o;
	
	pthread_t bake_threads[MAX_PIZZAS];
	int bake_threads_idx = 0;

	size_t ret = read(*sock, buf, MAX_READ_BYTES);

	if (ret != sizeof(o)) {
		// prepei na lambanoume osa bytes einai i struct mas
		printf("thread: %zu size error (%zu)\n", pthread_self(), ret);
		close(*sock);
		free(sock);
		return NULL;
	}

	// kanoume copy to raw buffer sti domi mas
	memcpy(&o, buf, sizeof(o));
	// kratame to pote mpike i paraggelia sto sistima
	gettimeofday(&o.start, NULL);

	// time to start cooking. Edo perimenoume ospou na exoume diathesimo psisti
	pthread_mutex_lock(&bake_mutex);
	for(i=0; i<MAX_CATEGORIES; i++) {
		for(j=0; j < o.pizzes[i]; j++) {
			// an den exoume psisti perimenoume os ton proto diathesimo
			if (available_psistes == 0) {
				printf("@@");
				pthread_cond_wait(&bake_cond, &bake_mutex);
			}
			// elatonoume tous diathesimous psistes. 
			// gia kathe pizza trexoume ena thread gia na psithei
			available_psistes--;
			pthread_create( &bake_threads[bake_threads_idx++],
					NULL, bake_thread, &bake_times[i]);
		}
	}
	pthread_mutex_unlock(&bake_mutex);
	

	// perimenoume na teleiosoun ola ta threads pou psinoun
	for(i=0; i<bake_threads_idx; i++)
		pthread_join(bake_threads[i], NULL);

	pthread_mutex_lock(&deliver_mutex);
	// anazitoume diathesimo delivery boy. An den yparxei anamenoume gia ton 1o
	if (available_deliver_boys == 0) {
		printf("##");
		pthread_cond_wait(&deliver_cond, &deliver_mutex);
	}
	available_deliver_boys--;
	pthread_mutex_unlock(&deliver_mutex);

	// pleon ola einai etoima. Steile tin pizza. I deliver_order tha ipologisei
	// kai to sinoliko xrono kai an prepei na dosei mpyra
	deliver_order(&o, *sock);	

	pthread_mutex_lock(&deliver_mutex);

	// devilvery finished. Simainei diathesimo delivery boy => signal
	pthread_cond_signal(&deliver_cond);
	available_deliver_boys++;

	pthread_mutex_unlock(&deliver_mutex);

	// kleinoume kai katharizoume
	close(*sock);
	// i sock itan enas pointer pou kratouse to sock. Prepei na to eleutherosoume
	free(sock);

	return NULL;
}

int main(int argc, char *argv[])
{
	/* Dilwsi file descriptors pou epistrefontai ap tin klisi tis synartisis socket
	   kai accept antistoixa */
	int s, new_sd = -1;
	/* Dilwsi megethous buffer opou apo8hkeyontai xarakthres gia xrisi stis synarthseis
	   sistimatos read kai write */
	/* Dilwsi diey8ynsewn tou server kai tou client antistoixa */
	struct sockaddr_un local, remote;
	/* Dilwsi mikous diey8ynsewn tou server kai tou client antistoixa */
	int len;
	socklen_t clilen;
	/* Dilwsi buffer opou apo8hkeyontai xarakthres gia xrisi stis synarthseis sistimatos
	   read kai write */
	//char buf[ MAX_READ_BYTES ];

	/* Dhmiourgia tou end point tou server */
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		printf("Server socket failure  %d\n", errno);
		perror("Server: ");
		exit(1);
	}

	printf("To socket dhmiourgh8hke! Epityxia!\n");

	local.sun_family = AF_UNIX; // ka8orismos tou typou tou socket se local (Unix Domain)
	strcpy(local.sun_path, PIZZARIA_PATH); // ka8orismos tou onomatos autou tou socket
	unlink(local.sun_path); // svisimo opoioudipote proigoumenou socket me to idio filename

	/* Synoliko mhkos diey8ynsews tou server */
	len = strlen(local.sun_path) + sizeof(local.sun_family);

	printf("adr len : %d\n", len);

	/* Elegxos sindesis socket descriptor me ena local port kai ektiposi
	   minimatos sfalmatos se periptwsi sfalmatos */
	if (bind(s, (struct sockaddr *)&local, len) < 0)
	{
		perror("Bind failed:");
		close(s);
		exit(1);
	}

	/* Dhmiourgia mias listas aitisewn gia tous clients me mikos 5 */
	if (listen(s, 5) < 0) {
		perror("Server: ");
		close(s);
		exit(1);
	}

	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	int *sock;
	/* Atermonos vrogxos pou periexei ton kwdika me ton opoio ginetai i syndesi me ton
	   client kai h exyphretisi tou */
	while( 1 ) {
		clilen = sizeof(remote); // Ka8orismos mege8ous diey8ynsis tou client

		/* Antigrafi tis epomenis aitisis apo tin oura aitisewn sti metavliti new_sd
		   kai diagrafi tis apo tin oura */
		if ( ( new_sd = accept(s, (struct sockaddr*)&remote, &clilen)) < 0 ) {
			perror("Accept failed!");
			sleep(1);
			continue;
		}
		
		// epeidi to socket to pairname sta threads, kathe socket to vazoume se
		// diaforetiki thesi mnimis gia na apofigoume overwrites.
		sock = malloc(sizeof(int));
		*sock = new_sd;

		pthread_t child;
		if ( pthread_create( &child, NULL, process_order, sock) != 0 )
			perror("can't create thread:");

		// to thread einai detached => tha eleutherothoun ta resources xoris 
		// na xreiazetai join
		pthread_detach( child );
	}

	// kai telos kai to socket kai ora na kleisei i pitsaria
	close(s);
	return 0;
}
