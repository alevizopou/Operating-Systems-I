/* symperilpsi aparaititwn vivlio8ikwn */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "order.h"
#include "pizzaria.h"

#define MAX_READ_BYTES 128

#define PSISTES 10
#define DELIVERADES 10

#define PSISTES_SEM_NAME "psistes"
#define DELIVERIES_SEM_NAME "deliveries"

#define T_VERYLONG 50*1000 // se micro seconds gia na to xrisimopoioume me to timeval

// semaphores. 
sem_t * psistes_semaphore;
sem_t * delivery_semaphore;

// theloume na moirastoume mnimi gia paraggelies

#define SHM_KEY_PIZZES       100
#define SHM_KEY_ORDERS_READY 102

void terminate(int sig);

// xronoi se microseconds
int bake_times[3] = { 10*1000, 12*1000, 15*1000 };

// elegxei an i paraggelai exei argisei. Vazei to apotelesma sto buf
int check_order(order *o, char *buf)
{
	struct timeval end;
	// elegxoume an exoume xeperasei to megisto xrono paraggeleias. An nai kanoume doro mia mpyra (free as in beer)
	gettimeofday(&end, NULL);
	int secs = (end.tv_sec - o->start.tv_sec);
	int usecs = (end.tv_usec - o->start.tv_usec);

	int total = (secs*1000000) + usecs;

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

void deliver_order(order *o, int order_socket)
{
	char buf[128];
	// an exoume delivera proxorame. Allios perimenoume ospou na erhei enas
	sem_wait(delivery_semaphore);

	// ipologizoume to sinoliko xrono afou exoume vrei kai diathesimo delivera alla xoris to xrono paradosis tis pitsas.
	int total = check_order(o, buf);
	int order_pid = getpid();

	printf("%d: Total order time: %d ms\n", order_pid, total/1000);

	// pleon exoume teleiosei me ola. Kalo dromo stin(s) pizza(es)
	usleep( ((int)o->apostasi) * 1000 );

	printf("%d: done. Client is %d ms far.\n", order_pid, o->apostasi);
	printf("%d -> %s\n", order_pid, buf);

	// tin paradosame. Apeleutheronoume to semaphore
	sem_post(delivery_semaphore);

	// kai stelnoume piso
	write(order_socket, buf, strlen(buf));
}

void prepare_order(order *o)
{
	int i, j;
	// trexoume oles tis pizzes
	for (i=0; i< MAX_CATEGORIES; i++) {
		for(j=0; j < o->pizzes[i]; j++) {

			sem_wait(psistes_semaphore);
			// diathesimos psistis. ora na kanoume fork

			// kanoume ena fork gia kathe pizza afou exoume diathesimo psisti 
			if ( fork() == 0 ) {
				//printf("\t%d time to bake...", order_pid);
				usleep(bake_times[i]);
				//printf("done\n");fflush(stdout);

				// increase to semaphore aka diathesimos psistis
				sem_post(psistes_semaphore);
				// apeleutherose to semaphore sto sistima (afou to child edo pethainei)
				sem_unlink(PSISTES_SEM_NAME);
				exit(0);
			} else {
			}
		}
	}

	// perimenoume gia ola ta paidia na teleiosoun
	while (waitpid(-1, NULL, 0)) {
		if (errno == ECHILD)
			break;
	}
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
	char buf[ MAX_READ_BYTES ];

	// semaphore gia psistes. Arxiki timi einai PSISTES etsi oste na kleidonoun otan den iparxoun diathesimoi
	psistes_semaphore = sem_open(PSISTES_SEM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, PSISTES);
	// antistoixa gia deliveries
	delivery_semaphore = sem_open(DELIVERIES_SEM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, DELIVERADES);

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
		printf("Server listen failure %d\n", errno);
		perror("Server: ");
		exit(1);
	}

	signal(SIGCHLD, SIG_IGN);

	signal(SIGINT, terminate);
	signal(SIGTERM, terminate);
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

		//printf("Connected! New socket : %d\n", new_sd);

		if (fork() == 0) {
			/* child process */
			close(s); /* child does not need it */

			// diavazoume tis leptomereies tis paraggelies.
			read(new_sd, buf, MAX_READ_BYTES);

			int order_pid = getpid();
			order o;
			// kanoume copy to raw buffer sti domi mas
			memcpy(&o, buf, sizeof(o));
			printf("%d order: %d %d %d %d %d\n", order_pid, o.pizzes[MARGARITA], o.pizzes[PEPPERONI], o.pizzes[SPECIAL], o.apostasi, o.order_pid); 

			// xreiazomaste na xeroume pote xekinise i paraggelia gia na ipologisoume sto telos an argisame
			gettimeofday(&o.start, NULL);

			// psiiiise!
			prepare_order(&o);

			// kai steile!
			deliver_order(&o, new_sd);

			// katharizoume ton pagko :P
			sem_unlink(DELIVERIES_SEM_NAME);
			close(new_sd);

			exit(0);
		} else {
			//printf("closing child socket %d\n", close(new_sd));
			close(new_sd);
		}
	}

	// kleinoume tous semaphores
	sem_close(psistes_semaphore);
	sem_close(delivery_semaphore);
	// kai telos kai to socket kai ora na kleisei i pitsaria
	close(s);
	return 0;
}

void terminate(int sig)
{
	sem_close(psistes_semaphore);
	sem_close(delivery_semaphore);
	exit(0);
}

