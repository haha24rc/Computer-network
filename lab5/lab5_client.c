/* time_client.c - main */

#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>                                                                            
#include <netinet/in.h>
#include <arpa/inet.h>
                                                                                
#include <netdb.h>


#define	BUFSIZE	100

#define	MSG		"Any Message \n"


/*------------------------------------------------------------------------
 * main - UDP client for TIME service that prints the resulting time
 *------------------------------------------------------------------------
 */
 
 struct pdu {
 	char type;
 	char data[BUFSIZE];
 }spdu,cpdu;
 
 
int main(int argc, char **argv)
{
	char	*host = "localhost";
	char	buf[100];
	int	port = 3000;
	char	now[100];			/* 32-bit integer to hold time	*/ 
	struct hostent	*phe;		/* pointer to host information entry	*/
	struct sockaddr_in sin;		/* an Internet endpoint address		*/
	int	s, n, type;				/* socket descriptor and socket type	*/

	switch (argc) {
	case 1:
		break;
	case 2:
		host = argv[1];
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "usage: UDPtime [host [port]]\n");
		exit(1);
	}

	memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;                                                                
        sin.sin_port = htons(port);
                                                                                        
    /* Map host name to IP address, allowing for dotted decimal */
        if ( phe = gethostbyname(host) ){
                memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
        }
        else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		fprintf(stderr, "Can't get host entry \n");
                                                                                
    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
		fprintf(stderr, "Can't create socket \n");
	
	//--------------------------------------------------------------------------------                                                                         
    /* Connect the socket */
   	FILE * fp;
   	 
   	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "Can't connect to %s %s \n", host, "Time");
		
	while(1) {
		printf("Enter a file name or 'exit' to quit:\n");
		n = read(0, spdu.data, BUFSIZE);
		spdu.data[n-1] = '\0';
		
		if (strcmp(spdu.data, "exit") == 0){
			break;
		}
		else{
			spdu.type = 'C';
			write(s, &spdu, n+1);		//send file name
			
			fp = fopen(spdu.data, "w");
			while(1){
				n = recv(s, &cpdu, BUFSIZE+1,0);
				cpdu.data[BUFSIZE] = '\0';
				
				if(cpdu.type == 'E'){
					printf("Error: %s\n",cpdu.data);
					remove(spdu.data);
					break;
				}
				fprintf(fp, "%s",cpdu.data);
				if(cpdu.type == 'F'){
					fclose(fp);
					printf("File recieved\n");
					break;
				}				
			}	
			bzero(&cpdu,BUFSIZE+1);
		}
	}
	exit(0);
}
