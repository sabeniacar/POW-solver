/* 
login id: sabeniacar, student no: 838392
A simple server based proof of work solver using SSTP
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include "uint256.h"
#include "sha256.h"
#include <time.h>
#include <arpa/inet.h>
#include <sys/queue.h>
#include <pthread.h>
#include <inttypes.h>


int found=0;
uint64_t max =  9223372036854775807;
//given seed as byte array,nonce as uint64_t and difficulty as uint32_t  returns if seed and nonce concatenated together 
//gives a hash smaller than target which is derived from diff
int hashSatisfied(BYTE* seed,uint64_t nnonce,uint32_t diff){
  
char dif[9];
sprintf(dif, "%08x", diff);   //convert difficulty to hex and store as string 

char nonce[17];
sprintf(nonce, "%016lx", nnonce);  //convert nonce to hex and store as string

   BYTE beta[32];     //stores beta derived from difficulty
   BYTE target[32];   //stores target value derived from alpha and beta
   BYTE concat[40];   //stores concatenated seed and nonce

   uint256_init (beta);
   uint256_init (target);

   BYTE buf[32];
   BYTE buff[32];

   SHA256_CTX ctx;
   
   char alphaHex[2], *ah = alphaHex;  //alpha in difficulty formula
   char betaHex[6], *bh = betaHex;    //beta in difficulty formula

   char* dum = dif;
	//take first byte(8 bits,first 2 characters of dif string) of difficulty to derive alpha 
	for(int i=0; i<2; i++){
		sscanf(dum, "%c", &alphaHex[i]);
		dum+=1;
	}
	//take last 3 bytes(24 bits,last 6 characters of dif string) of difficulty to derive beta 
	for(int i=0; i<6; i++){
		sscanf(dum, "%c", &betaHex[i]);
		dum+=1;
	}
         	unsigned a = strtol(ah,0,16);
	
	for(int i=0; i<3; i++){
		sscanf(bh, "%2hhx", &beta[31-i]);
		bh += 2;
	}
	int l = 8*(a-3);

	//shift beta left l times to get desired target value
         	uint256_sl (target, beta, l);
		
	//put seed in concatenated seed|nonce byte array
        for(int i = 0; i < 32; i++) {
		concat[i]=seed[i];
	}

	//put nonce in concatenated seed|nonce byte array 
	dum = nonce;
        for(int i = 32; i < 40; i++) {
		sscanf(dum, "%2hhx", &concat[i]);
		dum += 2;
	}
 
	//hash concatenated byte array, then hash the result and compare with target value
	sha256_init(&ctx);
	sha256_update(&ctx, concat, 40);
	sha256_final(&ctx, buf);
	sha256_init(&ctx);
	sha256_update(&ctx, buf, 32);
	sha256_final(&ctx, buff);
	
	return sha256_compare(target,buff);


}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//work structure, stores seed,nonce,target,number of threads and file descriptor of work
typedef struct work{
   uint64_t nonce;
   BYTE seed[32];
   uint32_t target;
   int th;
   int id;

}work;

//work structure, stores seed,nonce,target,number of threads and file descriptor of work
typedef struct thinp{
   work* w;
   uint64_t num;
   pthread_t ids[255];
}thinp;


TAILQ_HEAD(tailhead, entry) head;

	//queue entry, each entry has a work
	struct entry {
	  work *w;
	  TAILQ_ENTRY(entry) entries;
	};


	//given work adds work to the queue
	void* addWorkQueue(void* arg){

	struct work *w = arg;

	  struct entry *elem;
	  elem = malloc(sizeof(struct entry));
	  if (elem) {
	    elem->w = w;
	  }
	  //lock when adding an element

		pthread_mutex_lock(&lock);
	  TAILQ_INSERT_HEAD(&head, elem, entries);
		pthread_mutex_unlock(&lock);

	pthread_exit(0);

	}
uint64_t sol=0;
       //given work checks finds a solution
       void* checkSolution(void* arg){
           
	 
	   struct thinp *th = arg;
	   uint64_t n = th->num;
	   struct work *ww = th->w;
	   int ff = 0;
		while(1){

		  if(found == 1){

         
			pthread_exit(0);
		   }


		ff = hashSatisfied(ww->seed,n,ww->target);
		//printf("ff is %d and n is %lu\n",ff,n);
		

			if(ff==1){


			sol = n;
			found=1;

			}
			n = n+1;
		

		}


	     pthread_exit(0);
       }

	//thread runs without ending and processes one work at a time from queue
	void* processWorkQueue(){

	     
	  while(1){

                //if queue is not empty take head and process it
	        struct entry *wo;
		if(!TAILQ_EMPTY(&head)){
		pthread_mutex_lock(&lock);
		//TAILQ_FOREACH(wo,&head,entries){
	
			wo = TAILQ_FIRST(&head);
			struct work *ww = wo->w;
			TAILQ_REMOVE(&head, wo, entries);
			//pthread_mutex_unlock(&lock);
			
			//create desired number of threads to work on finding a solution
				pthread_t tids[ww->th];
				for(int i=0; i<ww->th; i++){

					thinp* newTh = malloc(sizeof(thinp));
					newTh->w =ww;
					newTh->num=(ww->nonce)+i*((max-(ww->nonce))/(ww->th));		
					//(newTh->ids)[i]=tids[i];			
					pthread_attr_t attr;
					pthread_attr_init(&attr);
					pthread_create(&tids[i], &attr, checkSolution, newTh );

				}


			    	for(int i=0; i<ww->th; i++){

					pthread_join(tids[i],NULL);
							
				}   
				

			
		found = 0; 
		char message[128]; //when a solution is found it will be put in message buffer
		char tar[9];
		char sed[65];
		char no[17];	

			
			sprintf(tar, "%08x", ww->target);
			sprintf(no, "%016lx", sol);

			for(int x = 0; x < 32; x++)
			    sprintf(sed+(x*2), "%02x", (ww->seed)[x]);
			sed[64] = '\0';

			//put strings to be printed into the message buffer
			strcpy(message,"SOLN ");
			strcat(message, tar);
			strcat(message," ");
			strcat(message, sed);
			strcat(message," ");
			strcat(message, no);
			strcat(message,"\r\n");
		
                        int b=0;
			b = write(ww->id,message  , 97);


			if(b<0){
				perror("ERROR writing to socket");
				exit(1);
			}

	     

	     }
		pthread_mutex_unlock(&lock);

	 
	 }
	     pthread_exit(0);	
	}


int main(int argc, char **argv)
{

	TAILQ_INIT(&head);

	//initialize the thread responsible for processing the work queue
        pthread_t tidQ;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&tidQ, &attr, processWorkQueue,NULL);

	int sockfd, newsockfd, portno, maxClients = 100, sockClients[100], sd, activity;
	char buffer[1024];

	char* pch;
	char* ttime;
	struct sockaddr_in serv_addr;
	socklen_t clilen;
	int n;
	int max;


	FILE *fp;
	
	time_t rawtime;
        struct tm* timeinfo;



	fd_set readfds;

	if (argc < 2) 
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	//initialise sockClients[] to zero
	for(int i=0; i<maxClients; i++){
		sockClients[i] = 0;
	}

	 /* Create TCP socket */
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) 
	{
		perror("ERROR opening socket");
		exit(1);
	}

	
	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);
	
	/* Create address we're going to listen on (given port number)
	 - converted to network byte order & any IP address for 
	 this machine */
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);  // store in machine-neutral format

	 /* Bind address to the socket */
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0) 
	{
		perror("ERROR on binding");
		exit(1);
	}
	
	/* Listen on socket - means we're ready to accept connections - 
	 incoming connection requests will be queued */
	
	
	
	if(listen(sockfd,5)<0){

		perror("listen");
		exit(1);
	}
		
	clilen = sizeof(serv_addr);



	while(1){

		FD_ZERO(&readfds);
		FD_SET(sockfd,&readfds);
		max = sockfd;
		//find maximum file descriptor
		for(int i=0; i<maxClients ; i++){
	
			sd = sockClients[i];
			if(sd > 0)
			   FD_SET(sd, &readfds);	
			if(sd > max)
			   max = sd;
	
		}

		activity = select(max+1, &readfds, NULL, NULL, NULL);

		if((activity<0) && (errno!=EINTR)){
			printf("select error");
		}
		
		if(FD_ISSET(sockfd, &readfds)){
		//new connection
	
			newsockfd = accept(sockfd, (struct sockaddr *) &serv_addr, 
							&clilen);

			if (newsockfd < 0) 
			{
				perror("ERROR on accept");
				exit(1);
			}
			//add new client to client array
			for(int i=0; i<maxClients; i++){
				if(sockClients[i] == 0){
					sockClients[i] = newsockfd;
					break;			
				}
			}
		}
		//I/O with some client
		for(int i=0; i<maxClients; i++){
	                bzero(buffer,1024);
			sd = sockClients[i];

		
			if(FD_ISSET(sd,&readfds)){
			     time(&rawtime);
			     timeinfo = localtime(&rawtime);
			     ttime = strtok(asctime(timeinfo), "\n");
                             getpeername(sd , (struct sockaddr*)&serv_addr ,  (socklen_t*)&clilen);  

				fp = fopen("log.txt","a");
	
				n = read(sd,buffer,1023);
printf("task is %s",buffer);
				//write client input to log.txt
				fprintf(fp,"%s  %s  %d  %s\n",ttime, inet_ntoa(serv_addr.sin_addr), sd  ,buffer);
				fclose(fp);
				//if n is zero close connection
				if(n == 0){
					close(sd);
					sockClients[i]=0;
				}else{

					if (n < 0) 
					{
						perror("ERROR reading from socket");
						exit(1);
					}

					if(strcmp(buffer,"PING\r\n") == 0){

					    n = write(sd,"PONG\r\n",6);

                                        if (n < 0)
                                        {
                                                perror("ERROR writing to socket");
                                                exit(1);
                                        }

					}else if(strcmp(buffer,"PONG\r\n") == 0){

					   n = write(sd,"ERRO:PONG is reserved for server\r\n",34);

                                        if (n < 0)
                                        {
                                                perror("ERROR writing to socket");
                                                exit(1);
                                        }
					}else if(strcmp(buffer,"OKAY\r\n") == 0){
					  
				         n = write(sd,"ERRO:It's not okay to send OKAY\r\n",33);
					 if (n < 0)
                                         {
                                                perror("ERROR writing to socket");
                                                exit(1);
                                         }
					}else if(strcmp(buffer,"ERRO\r\n") == 0){
				          
 					  n = write(sd,"ERRO:This can't be send to server\r\n",35);
					  if (n < 0)
                                          {
                                                perror("ERROR writing to socket");
                                                exit(1);
                                          }
					}else{
						pch = strtok(buffer," ");
						if(!strcmp(pch,"SOLN")){
							BYTE s[32];
				                        pch = strtok(NULL," ");
							uint32_t t= strtol(pch,0,16);							
							pch = strtok(NULL," ");
		
							for(int i = 0; i < 32; i++) {
								sscanf(pch, "%2hhx", &s[i]);
								pch += 2;
							}

							pch = strtok(NULL," ");
							uint64_t n;
							n = strtol(pch,0,16);
							int m = hashSatisfied(s,n,t);
							
							if(m == 1){
								n = write(sd,"OKAY\r\n",6);
							}else{
								n = write(sd,"ERRO:Not valid proof of work!\r\n",31);
							}
							 if (n < 0)
                                       		         {
                                              		        perror("ERROR writing to socket");
                                                		exit(1);
                                       			 }		
						}else if(!strcmp(pch,"WORK")){
							uint32_t t;
							work* newWork = malloc(sizeof(work));
							pch = strtok(NULL," ");
							t = strtol(pch,0,16);    //read target to a uint32_t variable
							newWork->target = t;

							pch = strtok(NULL," ");  //fill byte array of created work
							for(int i = 0; i < 32; i++) {
								sscanf(pch, "%2hhx", &(newWork->seed)[i]);
								pch += 2;
							}

							pch = strtok(NULL," ");
							uint64_t n;
							n = strtol(pch,0,16);
							newWork->nonce = n;   //read nonce to a uint64_t variable
							pch = strtok(NULL," ");
							int tNum = strtol(pch,0,16); 
							newWork->th = tNum;   //read number of threads to int varible
							newWork->id = sd;     
							pthread_t tid;	     
							//add work to queue
							pthread_attr_t attr;
							pthread_attr_init(&attr);
							pthread_create(&tid, &attr, addWorkQueue,newWork);



						}else{

							n = write(sd,"ERRO:ERROR OCCURRED\r\n",21);
	
						if (n < 0)
                                        	{
                                                perror("ERROR writing to socket");
                                                exit(1);
                                        	}
			

					}	


			
				}
			}
		   }
			
		}
	}

	return 0; 
}
