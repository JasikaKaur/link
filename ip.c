#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include "ip.h"

/**
 * 	This function receives the name of the interface that you want to show the IP ADDRESS,
 * and returns the string containing the ip address of that interface, in case the interface
 * is found. It returns NULL otherwise (or segmentation fault).
 * 
 * example of calling:   printf("%s\n", getIpAddr((char*)"eth0"));
 */
char* getIpAddr(char* interface)
{
	int fd, size;
	struct if_nameindex *curif, *ifs;
	struct ifreq req;
	char* address, temp[13];

	if((fd = socket(PF_INET, SOCK_DGRAM, 0)) != -1) 
	{
		ifs = if_nameindex();
		if(ifs) 
		{
			for(curif = ifs; curif && curif->if_name; curif++) 
			{
				strncpy(req.ifr_name, curif->if_name, IFNAMSIZ);
				req.ifr_name[IFNAMSIZ] = 0;
				ioctl(fd, SIOCGIFADDR, &req);
				if (strcmp(curif->if_name, interface) == 0)
					strcpy(temp,inet_ntoa(((struct sockaddr_in*) &req.ifr_addr)->sin_addr));
			}
		if_freenameindex(ifs);
		if(close(fd)!=0)
			perror("close");
		} 
		else
			perror("if_nameindex");
	} 
	else
		perror("socket");
		
	size = strlen(temp);
	if (size>0)
	{
		address = (char*) malloc(size * sizeof(char));
		strcpy(address, temp);
		return address;
	}
	else
		return NULL;
}
