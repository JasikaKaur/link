
 #include <stdio.h>

 #include <string.h>

 #include <stdlib.h>

 #include <sys/types.h>

 #include <sys/socket.h>

 #include <netinet/in.h>

 #include <arpa/inet.h>

 #include <unistd.h>

 #include <errno.h>

 	

 #define BUFSIZE 1024

 		

 void send_recv(int i, int sockfd)

 {

 	char send_buf[BUFSIZE];

 	char recv_buf[BUFSIZE];

 	int nbyte_recvd;

 	

 	if (i == 0)
{

 		fgets(send_buf, BUFSIZE, stdin);

 		if (strcmp(send_buf , "quit\n") == 0) 
		{

 			exit(0);

 		}
else
{
 			send(sockfd, send_buf, strlen(send_buf), 0);

 	}
else {

 		nbyte_recvd = recv(sockfd, recv_buf, BUFSIZE, 0);

 		recv_buf[nbyte_recvd] = '\0';

 		printf("%s\n" , recv_buf);

 		fflush(stdout);

 	}

 }

 		

