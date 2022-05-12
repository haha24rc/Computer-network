/* time_client.c - main */

#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>                                                                            
#include <netinet/in.h>

#include <arpa/inet.h>                                                                             
#include <netdb.h>
#include <errno.h>

#include <dirent.h>
#include <sys/types.h>

#define	NAMESIZE	10
#define	DATASIZE	100
#define	PACKETSIZE	101

#define	MSG		"Any Message \n"


/*------------------------------------------------------------------------
 * main - UDP client for TIME service that prints the resulting time
 *------------------------------------------------------------------------
 */
struct pdu {
 	char type;
	char peerName[NAMESIZE];
	char contentName[NAMESIZE];
	struct	sockaddr_in addr;
 };


 struct pdu2 {
 	char type;
 	char data[DATASIZE];
 };
 
void userOperation(int s, struct sockaddr_in reg_addr, char *peerName, int *list);
void clientOperation(int s);
void findLocalFile();
int contentServer(int s, char peerName[], char contentName[], struct sockaddr_in reg_addr);
int downloadContent(int s, char contentName[]);
void PDUSetup(struct pdu * cpdu, char type, char peerName[], char contentName[]);

int main(int argc, char **argv)
{
	char	*host = "localhost";
	int	port = 3000;
	char	now[100];			/* 32-bit integer to hold time	*/ 
	struct hostent	*phe;		/* pointer to host information entry	*/
	struct sockaddr_in sin, reg_addr, client;		/* an Internet endpoint address		*/
	int	s, n, type,sd;				/* socket descriptor and socket type	*/
	

	int alen, new_sd;
	char peerName[NAMESIZE];
	int clientlen, list = 0;
	int *listPtr = &list;

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
	                                                                         
    /* Connect the socket */
   	 
   	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "Can't connect to %s %s \n", host, "Time");


	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}
	bzero((char *)&reg_addr, sizeof(struct sockaddr_in));
	reg_addr.sin_family = AF_INET;
	reg_addr.sin_port = htons(0);
	//reg_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	reg_addr.sin_addr.s_addr = htonl(INADDR_ANY);							//
	if (bind(sd,(struct sockaddr *)&reg_addr, sizeof(reg_addr)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	alen = sizeof(struct sockaddr_in);
	getsockname(sd, (struct sockaddr *)&reg_addr, &alen); 					//

	listen(sd, 10);

	/* listen multiple sockets */
	fd_set rfds, afds;

	peerName[0] = '\0';
	printf("Choose a user name:\n");

	while(1){
		FD_ZERO(&afds);
		FD_SET(0, &afds);			/*Listening on stdin*/
		FD_SET(sd, &afds);			/*Listening on a TCP socket*/
		memcpy(&rfds, &afds,sizeof(rfds));
		select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
		

		if(FD_ISSET(sd, &rfds)){
			clientlen = sizeof(client);
			new_sd = accept(sd,(struct sockaddr*)&client,&clientlen);
			//new_sd = accept(sd,(struct sockaddr*)&client,&alen); //
			if(new_sd >= 0){
				clientOperation(new_sd);
				close(new_sd);
				printf("Command: \n");
			}

		}
		if(FD_ISSET(fileno(stdin), &rfds)){
			userOperation(s, reg_addr, peerName, listPtr);
			printf("Command: \n");
		}
	}

	close(sd);
	exit(0);
}

void userOperation(int s, struct sockaddr_in addr, char *peerName, int *list){
	struct pdu spdu;
	struct pdu2 cpdu;

	char contentName[NAMESIZE], download[NAMESIZE];
	char command;
	int content_sd, d;
	
	if(peerName[0] == '\0'){
		scanf("%s",peerName);
		peerName[NAMESIZE] = '\0';
		printf("Command:\n");
	}
	scanf(" %c",&command);

	/*command*/
	switch(command){
		case 'D':
			printf("Enter the name of the content:\n");
			scanf("%s",download);
			content_sd = contentServer(s, peerName, download, addr);
			if(content_sd < 0){
				printf("Content does not exist\n");
				break;
			}
			if ((d = downloadContent(content_sd, download)) < 0)
				break;
		case 'R':
			if(command == 'D'){
				PDUSetup(&spdu, 'R', peerName, download);
			}
			else{
				printf("Enter the name of the content\n");
				scanf("%s",contentName);
				PDUSetup(&spdu, 'R', peerName, contentName);
			}
			spdu.addr = addr;
			write(s, &spdu, sizeof(spdu));
			recv(s, &cpdu, DATASIZE + 1, 0);
			if(cpdu.type == 'E'){
				printf("Error: %s\n",cpdu.data);
			}
			if(cpdu.type == 'A'){
				printf("Registration: name: %s\n", spdu.contentName);
				printf("Registration: port: '%d'\n\n", spdu.addr.sin_port);
				*list += 1;
			}
			break;
		case 'O':
			spdu.type = 'O';
			write(s, &spdu, sizeof(spdu));
			recv(s,&cpdu, DATASIZE + 1, 0);
			printf("On-line content list: \n%s\n\n",cpdu.data);
			break;
		case 'Q':
		case 'T':
			if(*list <= 0){
				printf("No registered content\n\n");
			}
			else{
				printf("De-registration:");
				PDUSetup(&spdu, 'T', peerName, contentName);
				write(s, &spdu, sizeof(spdu));
				recv(s,&cpdu, DATASIZE+ 1, 0);
				if(cpdu.type == 'A'){
					*list -= 1;
					printf("Content %s is successfully de-Registered\n\n",spdu.contentName);
				}
			}
			if(command == 'Q'){
				exit(0);
			} 
			break;
		case 'L':
			findLocalFile();
			break;
		case '?':
			printf("R - Registration, T - De-Registration, D - Download, L - List Local Content,");
			printf(" O - List of On-Line Registered Content, Q - Quit\n\n");
			break;
		default:
			break;
	}
}

int contentServer(int s, char peerName[], char contentName[], struct sockaddr_in addr){
	struct pdu spdu, cpdu;
	struct pdu2 dpdu;
	int sd;
	
	PDUSetup(&spdu, 'S', peerName, contentName);
	spdu.addr = addr;
	printf("Search:\n");
	write(s, &spdu, sizeof(spdu));
	recv(s, &cpdu, DATASIZE + 1, 0);	// Receive content server address/port from index server
	
	if(cpdu.type == 'E'){	// If error, content doesn't exist
		return -1;
	}
	else if (cpdu.type == 'S'){
		PDUSetup(&spdu, 'D', peerName, contentName);
	}
	printf("Find content\n");
	printf("Download: Peer Port Number: '%d'\n", cpdu.addr.sin_port);
	
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    	fprintf(stderr, "Can't create a socket\n");
    	exit(1);
    }                                                               
    if (connect(sd, (struct sockaddr *)&cpdu.addr, sizeof(cpdu.addr)) < 0){
    	fprintf(stderr, "Can't connect to host\n");
    	exit(1);
    }
    return sd;
}

int downloadContent(int s, char contentName[]){
	struct pdu spdu;
	struct pdu2 dpdu;
	FILE * fp;
	int n;
	int d;
	
	spdu.type = 'D';
	strcpy(spdu.contentName , contentName);
	spdu.contentName[NAMESIZE] = '\0';
	d = write(s,&spdu, sizeof(spdu));
	
	fp = fopen(contentName, "w");
	while(1){
		n = recv(s, &dpdu, DATASIZE+1,0);
		dpdu.data[DATASIZE] = '\0';
		if(dpdu.type == 'E'){
			printf("Error: %s\n",dpdu.data);
			remove(contentName);
			return -1;
		}
		fprintf(fp, "%s",dpdu.data);
		if(dpdu.type == 'F'){
			break;
		}				
	}
	fclose(fp);
	return 0;
}

void findLocalFile(){
	DIR * folder;
	struct dirent *entry;

	folder = opendir(".");

	if(folder == NULL){
		printf("No content found in local file\n\n");
	}
	else{
		while ((entry = readdir(folder))){
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			if (entry->d_type == DT_REG){
				const char *ex = strrchr(entry->d_name,'.');
				if (ex != NULL && strcmp(ex, ".c") != 0){
					printf("%s\n", entry->d_name);
				}
			}
		}
		closedir(folder);
	}

}

void clientOperation(int s){
	struct pdu spdu;
	struct pdu2 cpdu,dpdu;
	char fileName[NAMESIZE];
	int n;
	FILE *fp;
	int sent = 0;  
	char fileData[DATASIZE] = {0};
	
	if((n = recv(s, &spdu, DATASIZE+1 ,0)) < 0 ){
		fprintf(stderr, "recv error\n");
	}

	if(spdu.type == 'D'){
		strcpy(fileName, spdu.contentName);
		char filePath[NAMESIZE+2];	// Add current directory to file name
		snprintf(filePath, sizeof(filePath), "%s%s", "./", fileName);
		fp = fopen(filePath, "r");
	
		if(fp == NULL){
			cpdu.type = 'E';
			strcpy(cpdu.data, "File is not found");
			write(s, &cpdu, sizeof(cpdu));
		}
		else{
			struct stat st;
			int fileSize = 0;
			stat(fileName, &st);
			fileSize = st.st_size;		
			while((n = fread(fileData, sizeof(char), DATASIZE, fp)) > 0) {
				if( (sent + DATASIZE) >= fileSize){		
					cpdu.type = 'F';
				}
				else{
					cpdu.type = 'C';
				}
				memcpy(cpdu.data, fileData, DATASIZE);
				if((send(s, &cpdu,sizeof(cpdu), 0)) < 0){
					fprintf(stderr, "Error sending data\n");
					exit(1);
				}
				sent += n;	
				bzero(fileData, DATASIZE);
			}
			fclose(fp);
		}
	}
}

void PDUSetup(struct pdu * cpdu, char type, char peerName[], char contentName[]){
	cpdu -> type = type;
	strcpy(cpdu -> peerName, peerName);
	cpdu -> peerName[NAMESIZE] = '\0';
	strcpy(cpdu->contentName, contentName);
	cpdu -> contentName[NAMESIZE] = '\0';
}
