/* time_server.c - main */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

# define BUFLEN 100
# define NAMELEN 10
# define LISTLEN 20
# define INLEN (NAMELEN*2) + BUFLEN + 1
# define OUTLEN BUFLEN + 1
/*------------------------------------------------------------------------
 * main - Iterative UDP server for TIME service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
	struct  sockaddr_in fsin;	/* the from address of a client	*/
	char *listRow;
	char	buf[100], fileName[100], data[100];		/* "input" buffer; any size > 0	*/
	char    *pts;
	int	sock;			/* server socket		*/
	time_t	now;			/* current time			*/
	int	alen;			/* from-address length		*/
	struct  sockaddr_in sin; /* an Internet endpoint address         */
        int     s, type, size;        /* socket descriptor and socket type    */
        off_t   fBytes;
	int 	port=3000;
	FILE    *fp;
	struct pduPeer {
		char type;
		char contentName[NAMELEN];
		char peerName[NAMELEN];
		char address[BUFLEN];
	};
	
	struct pduServer {
		char type;
		char data[BUFLEN];
	};
	
	
	struct pduPeer pduIn;
	struct pduServer pduOut;
	struct stat stats;
	
	char contentName [LISTLEN][NAMELEN];
	char peerName [LISTLEN][NAMELEN];
	char address [LISTLEN][BUFLEN];
	
	int i, j, pduOutData_size, sp = 0;
                                                                        

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
                                                                                
    /* Bind the socket */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "can't bind to %d port\n",port);
        listen(s, 5);	
	alen = sizeof(fsin);
	
	while (1) {
		//receive UDP message
		if (recvfrom(s, &pduIn, INLEN, 0,(struct sockaddr *)&fsin, &alen) < 0)	
			fprintf(stderr, "recvfrom error\n");
			
		//If PDU = R, register content
		if(pduIn.type == 'R'){
			//prepare acknowledgement message
			pduOut.type = 'A';
			strcpy(pduOut.data, "Content registered successfully");
			//Check if peer and content name are taken
			for(i = 0; i < sp; i++){
				if(strcmp(pduIn.contentName, contentName[i]) == 0 && strcmp(pduIn.peerName, peerName[i]) == 0){
				//prepare error message
					pduOut.type = 'E';
					strcpy(pduOut.data, "Error: Content and peer name taken. Please register with a different peer name.");
				}
			}
			
			//If acknowledge message is prepared, register content
			if(pduOut.type = 'A'){
				if(sp <= LISTLEN){
					strcpy(contentName[sp], pduIn.contentName);
					strcpy(peerName[sp], pduIn.peerName);
					strcpy(address[sp], pduIn.address);
					sp++;
				}
				else{
					//content index full, error
					pduOut.type = 'E';
					strcpy(pduOut.data, "Error: Content Index list full. Content Registration failed.");

				}
			}
			//Send response to peer
			(void) sendto(s, &pduOut, OUTLEN, 0, (struct sockaddr *)&fsin, sizeof(fsin));
				
			
		}
		
		//If pdu type = S, search content
		else if(pduIn.type == 'S'){
			//prepare error message
			pduOut.type = 'E';
			strcpy(pduOut.data, "Error: Content not Found. Search Failed.");
			// check if content is registered
			for(i = 0; i < sp; i++){
				if(strcmp(pduIn.contentName, contentName[i]) == 0 && strcmp(pduIn.peerName, peerName[i]) == 0){
				//content exists, prepare Content message
					pduOut.type = 'C';
					strcpy(pduOut.data, address[i]);
					break;
					
				}
			}
			//Send response to peer
			(void) sendto(s, &pduOut, OUTLEN, 0, (struct sockaddr *)&fsin, sizeof(fsin));
				
		}
		
		//If pdu type = T, deregister content
		else if(pduIn.type == 'T'){
			//prepare error message
			pduOut.type = 'E';
			strcpy(pduOut.data, "Error: Content not Found. Deregistration Failed.");
			
			// search for content to deregister
			for(i = 0; i < sp; i++){
			
				if(strcmp(pduIn.contentName, contentName[i]) == 0 && strcmp(pduIn.peerName, peerName[i]) == 0){
					//content found, deregister
					
					//if deregistering content on top of the stack, decrement stack pointer
					if(i == (sp)-1){
						sp--;
					}
					else{
						//else shift content left to deregister 
						for(j = i; j < sp; j++){
							strcpy(contentName[j], contentName[j + 1]);
							strcpy(peerName[j], peerName[j + 1]);
							strcpy(address[j], address[j + 1]);
							sp--;
					
						}
					}
					
					pduOut.type = 'A';
					strcpy(pduOut.data, "Content de-registered successfully.");
		
					break;
				
				}
			
			}
			//Send response to peer
			(void) sendto(s, &pduOut, OUTLEN, 0, (struct sockaddr *)&fsin, sizeof(fsin));
				
		}
		
		//if pdu type = O, list content
		else if(pduIn.type == 'O'){
			//Initialize list message
			pduOut.type = 'C';
			strcpy(pduOut.data, "");
			pduOutData_size = 0;
			for(i = 0; i < sp; i++){
				//format row of list: "peerName   contentName"
				strcpy(listRow, peerName[i]);
				strcat(listRow, "   ");
				strcat(listRow, contentName[i]);
				strcat(listRow, "\n");
				
				//find updated pduOut.data size with new row
				pduOutData_size = pduOutData_size +strlen(listRow);
				
				// if pduOut.data size is less than BUFLEN
				if(pduOutData_size <= BUFLEN){
					//concatentate row
					strcat(pduOut.data, listRow);
	
				}
				else{
					//else new row exceeds pduOut.data size
					//Send response without new row
					(void) sendto(s, &pduOut, OUTLEN, 0, (struct sockaddr *)&fsin, sizeof(fsin));
				
					//Start next packet new row listRow
					pduOutData_size = strlen(listRow);
					strcpy(pduOut.data, listRow);
				}
				
			
				//Send final listresponse without new row
				(void) sendto(s, &pduOut, OUTLEN, 0, (struct sockaddr *)&fsin, sizeof(fsin));
				
			}
		}
		
        	
		
		
	}
}
