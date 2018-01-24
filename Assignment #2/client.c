/* symperilipsi aparaititwn vivlio8ikwn */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "order.h"
#include "pizzaria.h"

#define MAX_READ_BYTES 128

#define MAX_PIZZES 3

void usage() 
{
	printf("Usage: client <num_of_margarita> <num_of_peperoni> <num_of_special> <distance>\n");
}

int main(int argc, char *argv[])
{
	int s, t, len, connection_established;
	struct sockaddr_un remote;
	char buf[ MAX_READ_BYTES ];

	order o;

	if (argc == 5) {
		o.pizzes[MARGARITA] = atoi(argv[1]);
		o.pizzes[PEPPERONI] = atoi(argv[2]);
		o.pizzes[SPECIAL] = atoi(argv[3]);

		o.apostasi = atoi(argv[4]); 
		if ( (o.pizzes[0] + o.pizzes[1] + o.pizzes[2]) > MAX_PIZZES ) {
			printf("Megistos arithmos pitswn: %d\n", MAX_PIZZES);
			exit(-1);
		}
	} else {
		o.pizzes[MARGARITA] = 2;
		o.pizzes[PEPPERONI] = 1;
		o.pizzes[SPECIAL] = 0;
		o.apostasi = 3; 
	}

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		perror("Client socket failure: ");
		exit(1);
	}

	printf("Trying to connect...\n");

	remote.sun_family = AF_UNIX;
	/* Antigrafi tis diey8ynsis tou socket sti diey8ynsi tou server */
	strcpy(remote.sun_path, PIZZARIA_PATH);
	/* Ka8orismos tou synolikou mikous diey8ynsis */
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	/* Me tin klisi tou systhmatos connect() syndeetai o client me ton server.
	   Se anti8eti periptwsi ektypwnetai minima la8ous */
	connection_established = connect(s, (struct sockaddr *)&remote, len);
	if (connection_established < 0) {
		perror("Client connect failure : ");
		exit(1);
	} else {
		printf("Connected!\n");
	}

	memcpy(buf, &o, sizeof(o));
	/* me ti write o client grafei kapoio string(buf) ston server kai 
	   se anti8eti periptwsi ektypwnetai mhnyma la8ous */
	if (write(s, buf, sizeof(o)) == -1) {
		perror("write");
		exit(1);
	}

	/* me ti read o client diavazei kapoio string(buf) apo ton server kai 
	   se anti8eti periptwsi ektypwnetai mhnyma la8ous  */
	if ((t = read(s, buf, MAX_READ_BYTES)) > 0) {
		buf[t] = '\0';
		printf("%d: Server> %s\n", getpid(), buf);
	} else {
		if (t < 0) perror("read");
		else printf("Server closed connection\n");
		exit(1);
	}
	close(s);

	return 0;
}

