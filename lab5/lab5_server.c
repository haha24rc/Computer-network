/* time_server.c - main */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>



#define	BUFSIZE	100
/*------------------------------------------------------------------------
 * main - Iterative UDP server for TIME service
 *------------------------------------------------------------------------
 */
struct pdu {
	char type;
	char data[BUFSIZE];
}cpdu,spdu;

 
int main(int argc, char *argv[])
{
	struct  sockaddr_in fsin;		/* the from address of a client	*/
	char	buf[100];				/* "input" buffer; any size > 0	*/
	char    *pts;
	int	sock;						/* server socket		*/
	time_t	now;					/* current time			*/
	int	alen;						/* from-address length		*/
	struct  sockaddr_in sin; 		/* an Internet endpoint address         */
        int     s, type, n;       	/* socket descriptor and socket type    */
	int 	port=3000;
	char	fileNotFoundMsg[] = "FILE NOT FOUND";
                                              

	switch(argc){
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(port);
                                                                                                 
    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
		fprintf(stderr, "can't creat socket\n");
         
    //------------------------------------------------------------------------------------------
    FILE * fp;   
	struct stat st;
	int sent,fileSize;  
	char fileData[BUFSIZE] = {0};
                                                                             
    /* Bind the socket */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "can't bind to %d port\n",port);
    	listen(s, 5);	
	alen = sizeof(fsin);

	while (1) {
		if ((n = recvfrom(s, &cpdu, sizeof(cpdu), 0, (struct sockaddr *)&fsin, &alen)) < 0)
			fprintf(stderr, "recvfrom error\n");
		
		fp = fopen(cpdu.data, "r");	
		if (fp == NULL) {					//File does not exist		
			spdu.type = 'E';
			strcpy(spdu.data, "The file was not found");
			
			if(sendto(s, &spdu, BUFSIZE+1, 0, (struct sockaddr *)&fsin, sizeof(fsin)) < 0){
				//send error message: The file was not found
		  		fprintf(stderr, "Error sending data\n");
		  		exit(1);
			}
		}
		else {									// File exists
			struct stat st;
			int sent = 0,fileSize = 0;  
			char fileData[BUFSIZE] = {0};
			stat(cpdu.data, &st);
			fileSize = st.st_size;		
			while((n = fread(fileData, sizeof(char), BUFSIZE, fp)) > 0) {
				if( (sent + BUFSIZE) >= fileSize){		
					spdu.type = 'F';
				}
				else{
					spdu.type = 'D';
				}
				memcpy(spdu.data, fileData, BUFSIZE);
				if((sent = sendto(s, &spdu, BUFSIZE+1, 0, (struct sockaddr *)&fsin,
				 sizeof(fsin))) < 0){
		  			fprintf(stderr, "Error sending data\n");
		  			exit(1);
				}
				sent += n;	
				bzero(fileData, BUFSIZE);
			}
			fclose(fp);
		}
	}
}
