#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int main(int argc, char **argv)
{
	int x, z;
	char *host_ip = "192.168.1.84"; /*Here we have used localhost address for host machine */
	short host_port = 9991; 	/* It's the port of host machine */
	char *remote_ip = "192.168.1.5"; /* Ip address of remote machine*/
	short remote_port = 9999; 	/* port of remote machine */
	int i;
	int host_sock, remote_sock;
	char datagram[512];
	int remote_len;
	struct sockaddr_in host_add;
	struct sockaddr_in remote_add;
	host_sock = socket(AF_INET, SOCK_DGRAM, 0);
	host_add.sin_family = AF_INET;
	host_add.sin_port = htons(host_port);
	host_add.sin_addr.s_addr = inet_addr(host_ip);

	z = bind(host_sock, (struct sockaddr *)&host_add, sizeof(host_add));

	while(1){
		puts("Waiting for response.........");
		bzero(datagram, 512);

		/* Here we are waiting for recieving a message from remote machine */
		z = recvfrom(host_sock, datagram, 512, 0, (struct sockaddr *)&remote_add, &remote_len);
		datagram[z] = 0;
		printf("Shayam:%s", datagram);
		printf("Ram: ");
		bzero(datagram, 512);
		fgets(datagram, 512, stdin);

		remote_sock = socket(AF_INET, SOCK_DGRAM, 0);
		remote_add.sin_family = AF_INET;
		remote_add.sin_port = htons(remote_port);
		remote_add.sin_addr.s_addr = inet_addr(remote_ip);

		/* Here we are sending the udp message as datagram to remote machine */
		x = sendto(remote_sock, datagram, strlen(datagram), 0, (struct sockaddr *)&remote_add, sizeof(remote_add));
		if(x != -1){
			printf("Message sent. ");
		}
		close(remote_sock);
	}
	close(host_sock);
	return 0;
}
