#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define INT_MAX 560000
typedef struct Vertice{
char Name[32],IP[16];
int port;//the real one
int* dv;//[0/1,dv]
//int th_id;

}Vertice;

typedef struct Graph{
struct Vertice* v;
int ** Connections;
char** assigns;
int length;
}Graph;

int getnumber(char** arr,char* a,int leng);//gives the index number location 'a' in arr,length is arr length
Vertice* getverticbychar(struct Vertice* v,char* ch,int len);//gives the struct node according to char*

void NewInput(char* loc,struct Graph* g);//function for reading data from a file
int isnotin(char* a,char** arr);//0 if a is in arr,-1 if it exist 

void printchar(char** arr,int leng);
void printint(int* arr,int leng);


void* Thread_Sender ( void *ptr );
void* Thread_Reciever ( void *ptr );
void* ThreadCalculator ( void *ptr );

static pthread_cond_t weightCond = PTHREAD_COND_INITIALIZER;	//dv condition
static pthread_mutex_t weightMutex = PTHREAD_MUTEX_INITIALIZER; //lock for accessing weight[]

static pthread_cond_t calcCond = PTHREAD_COND_INITIALIZER;	//calculator condition
static pthread_mutex_t calcMutex = PTHREAD_MUTEX_INITIALIZER; //lock for waking calculator thread


void printchar(char** arr,int leng){
	int i;
	
	for(i=0;i<leng;i++){
	printf("%s \t",arr[i]);	
	}printf("\n");
}

void printint(int* arr,int leng){
	int i;
	for(i=0;i<leng;i++){
		printf("%d \t",arr[i]);
	}
	printf("\n");
}

int getnumber(char** arr,char* a,int leng){
	if(arr==NULL || arr[0]==NULL) {
		printf("g->assign is null! function: getnumber\n");
		exit(1);
	}
		

	int len=leng;
	int i;
	for(i=0;i<len;i++){
    	if(strcmp(arr[i],a)==0) 
    		return i;
	}
	return -1;
}


int isnotin(char* a,char** arr){//char to char*
	if(a==NULL) {printf("a on the left is null\n!"); exit(1);}

	if(arr==NULL){printf("arr is NULL\n"); exit(1);}

	char **p;
	for(p=arr;*p!=NULL;p++){
        if(strcmp(*p,a)==0) return -1;
	}
	return 0;
}

//reading Data From File
void NewInput(char* loc,struct Graph* g){//Copying from a file to Computer

	g->assigns=malloc(sizeof(char*));
	FILE *reader,*reader2;
	int line_number=1,letter3;
	char letter[32],letter2[32];

	g->v=malloc(sizeof(Vertice));//all nodes

	int len;
	reader=fopen(loc,"r");
	if(reader==NULL){
        printf("Error opinning a file motherfucker\n");
		exit(1);
	}
	
	reader2=fopen(loc,"r");
	if(reader==NULL){
		printf("Error opinning a file motherfucker\n");
		exit(1);
	}

	//Assigning
	len=0;
	size_t buffer_size = 0;
	char *buffer = NULL;

	//reading length -> as it assumed first line is length
	if(-1 != getline(&buffer, &buffer_size, reader)){
		g->length=buffer[0]-'0';
	}
	printf("in input, length %d \n",g->length);

	g->assigns[0]=NULL;

	//start reading input from file
	while(-1 != getline(&buffer, &buffer_size, reader))
    {
    		if(line_number>g->length){  printf("Breaked\n"); break;}

    		printf("%d: %s", line_number, buffer);
    		//<Node> <node-ip> <port>  
    		sscanf(buffer, "%s %s %d\n",letter,letter2,&letter3);

    		//building data for current nodes - check if already exist
    		if(isnotin(letter,g->assigns)==0)
    		{

    			//saving data nodes ig our graph
        		strcpy(g->v[len].Name,letter);
        		strcpy(g->v[len].IP,letter2);
        		g->v[len].port=letter3;

        		//initializing a distance vector for every node
        		g->v[len].dv=malloc(sizeof(int));
        		g->v[len].dv[0]=0;
        		g->assigns[len]=malloc(sizeof(char*));
        		strcpy(g->assigns[len],letter);
    			len++;
    			g->assigns[len]=NULL;
  			}
  		++line_number;	
	}

	//building the connection graph .. looking at the links
	int i,j;
   	g->Connections=(int**)malloc(len*sizeof(int*));
   	for(i=0;i<len;i++)
   	{
        g->Connections[i]=malloc(len*sizeof(int));
   	
   		for(j=0;j<len;j++){
   				//start with infinity
        		g->Connections[i][j]=INT_MAX;
        		//if same node make 0
    	    	if(i==j) g->Connections[i][j]=0;
    	}
	}
	//next line in file - read connections
	line_number=0;
	
	while(-1 != getline(&buffer, &buffer_size, reader2))
	{
		if(line_number>g->length)
		{
			printf("%d: %s",line_number, buffer);
			//reading <node> <neighbour node> distance
    		sscanf(buffer, "%s %s %d\n",letter,letter2,&letter3);
    		g->Connections[getnumber(g->assigns,letter,g->length)][getnumber(g->assigns,letter2,g->length)]=letter3;
    		g->Connections[getnumber(g->assigns,letter2,g->length)][getnumber(g->assigns,letter,g->length)]=letter3;
		}
		++line_number;
	}

	printf("END OF NewInput\n");
	printf("\n\n");
	//fclose(reader);
}

//by its title: given node (char) return vertice
Vertice* getverticbychar(struct Vertice* v,char* ch,int len){
   int i;
   struct Vertice* ret;
   ret=v;
   for(i=0;i<len;i++){
        if(strcmp(ret[i].Name,ch)==0) return &ret[i];

    }
    return NULL;
}


static char** dvNebours;//assigning
static int** NebourConn;
static int indv;		//numbers of main nebours
static int nb_zero[1];		//number of neighbours done calculating their DV
static int nb_received[1];//nebours recieved
static int LENGTH,chance;
static struct Graph *g;
static struct Vertice *SOURCE;


//Vertices and Connection has the same index pointing to same char
//as described in assigns

/*************************************************************************/
//		MAIN 							/
/**************************************************************************/


int main(int argc,char **argv){
//10 times
    if(argc<3){//.a.out file.txt R1 10
	printf("insert 3 aruments\n");
	exit(1);
	}
	
	printf("file:%s \tSource:%s \tchances:%s\n",argv[1],argv[2],argv[3]);
	
	chance=atoi(argv[3]);//for sender
	g=malloc(sizeof(Graph)*1);
	NewInput(argv[1],g);
	int len=g->length;
	LENGTH=len;
	
	if( isnotin(argv[2],g->assigns)==0){
		printf("char not exist\n");
		exit(1);
	}
	nb_received[0]=0;
	nb_zero[0]=1;
	//initilizing own dv
	SOURCE=malloc(sizeof(Vertice)*1);
	
	SOURCE->dv=malloc(sizeof(int)); 
	
	strcpy(SOURCE->Name,argv[2]);
	
	
	dvNebours=malloc(sizeof(char*));//assigns				
	indv=0;//num of nebours
	
	int i,j,row,counter=0;
	row=getnumber(g->assigns,SOURCE->Name,g->length);
	//inting source dv
	SOURCE->dv[0]=0;
	
	
	
	for(i=0;i<g->length;i++){
		if(g->Connections[row][i]==0 || g->Connections[row][i]==INT_MAX) counter++;
		SOURCE->dv[i+1]=g->Connections[row][i];
	}

	if(counter==g->length){//node has no nebour
		printf("Node had not nebours\n");
		exit(0);	
	}
	SOURCE->dv[0]=0;//dv was changed?!
	SOURCE->port=g->v[row].port;
	//building neghbours dv-assigining
	
	for(i=0;i<g->length;i++)
	{
		if(g->Connections[row][i]!=0 && g->Connections[row][i]!=INT_MAX)
		{
			//it is a nebour:
			dvNebours[indv]=malloc(sizeof(char*));
			strcpy(dvNebours[indv],g->assigns[i]);
			++indv;
		}
	
	}
	
	
	NebourConn=malloc(sizeof(int*)*indv);//nebour connection belongs to SOURCE
	//intiting NebourConn
	for(i=0;i<indv;i++)
	{
		NebourConn[i]=malloc(sizeof(int)*g->length);
		for(j=0;j<g->length;j++)
		{
			NebourConn[i][j]=INT_MAX;
		}
	}
	
	//creating threads
	pthread_t senders[indv];
	pthread_t recievers[indv];
	pthread_t calcu;
	
	int ti=0;
	for(j=0;j<LENGTH;j++){
	if(g->Connections[row][j]!=0 && g->Connections[row][j]!=INT_MAX){//building threads for only connected nodes
	
	if(	(pthread_create(&senders[ti],NULL, Thread_Sender, (void*)&g->v[j]) == -1) ||	//thread that sends
		((pthread_create(&recievers[ti],NULL, Thread_Reciever, (void*)&g->v[j]) == -1) ))//thread that reads
		{
			perror("creating threads,first proces");
			return 1;
		}
		++ti;
	    }
	}
	 //calculator
	if(pthread_create(&calcu, NULL, ThreadCalculator, (void*)NULL) == -1){
		perror("creating calculator thread");
	}
	
	//joining Thrads - you know 'main' preceed
	for(i=0; i < indv; i++){
		if(	(pthread_join(senders[i], NULL) == -1) ||
			(pthread_join(recievers[i], NULL) == -1) ){
			perror("pthread_join");
			//return 1;
		}
	}
	if(pthread_join(calcu, NULL) == -1){
		perror("pthread_join");
		//return 1;
	}
	
	
	
	return 0;
/*
Main:
Read file, initialized data structure.
Initialized own DV and via
Create threads
Join threads

*/
}
void* Thread_Sender ( void *ptr ){//ptr is neghbour port
	printf("in sender\n");
	//pthread_exit(NULL);
	int fd, portno, n,i;
    struct sockaddr_in srv;
    struct hostent *server;
    memset(&server, 0, sizeof(server));
    Vertice* ver=(Vertice*) ptr;
    
    portno = ver->port;//port to connect
    n=strlen(ver->Name);
    for(i=0; i < n; i++){
		portno +=SOURCE->Name[i];
	}
    printf("port to send to: %d Router:%s\n",portno,ver->Name);
    fd = socket(AF_INET, SOCK_STREAM, 0);//opinning a connection sesion
    if (fd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }
    int reuse=1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1){
 		printf("Reuse port Error : %s\n", strerror(errno));
	}
    
    
    bzero((char *) &srv, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = INADDR_ANY;/*inet_addr("ver->IP");*/
    //bcopy((char *)server->h_addr,(char *)&srv.sin_addr.s_addr,server->h_length);
    srv.sin_port = htons(portno);//connection figuritive
    
           
    n= connect(fd,(struct sockaddr*)&srv,sizeof(srv));//making a connection
    if ( n < 0)	//try to connect
    {
        while(chance>0 ){
            n= connect(fd,(struct sockaddr*)&srv,sizeof(srv));
            if(n==0) break;
            chance--;
            sleep(1000);
        }
        if(chance==0 && n<0){
            perror("error connecting\n");
            pthread_exit(NULL);
        }
    }////connecting succeeded
    int number_zero=0;
    while(1){
           
            printf("Thread Sender waiting for barrier\n");
           
            pthread_mutex_lock(&weightMutex);
            
            
            if(SOURCE->dv[0]==0){
            	if(*nb_zero>=indv)break;
            
            	printf("Thread sender is waiting\n");
            	pthread_cond_wait(&weightCond, & weightMutex);printf("Thread sender awaken\n");
            }
            if(SOURCE->dv[0]==0){
               //printf("row 410 when SOURCE->dv=%d\n",SOURCE->dv[0]);
               //printf(" 411 row\n") ;
               n=send(fd, (char *) &number_zero, sizeof(int), 0);//sending dv to neighbour
               if(n<0){
                    perror("error writing data");
                    pthread_exit(NULL);
               }
            }
              
           if(SOURCE->dv[0]==1){
           		//printf("row 420 when SOURCE->dv[0]=%d\n",SOURCE->dv[0]);
           		//sending dv to neighbour
               	n=send(fd,(char *) SOURCE->dv, sizeof(int)*(LENGTH+1), 0);
                if(n<0){
                    perror("error writing data");
                    pthread_exit(NULL);
                }
            } 
            
            SOURCE->dv[0]=0;//inting again
            pthread_mutex_unlock(&weightMutex);
            printf("Thread sender -- after barrier SOURCE->dv[0]:%d\n",SOURCE->dv[0]);

        //if the same dv was not changed
            //if the dv calculation is done then finalize reciever
            //otherwise wait in a conditioninal variable
        //if dv was changed {send 1dv}
        //else send 0
    }
    
    printf("Sender Finished\n");
    close(fd);
    pthread_exit(NULL);

}

void* Thread_Reciever ( void *ptr ){
	printf("in reciever\n");
	//pthread_exit(NULL);

	int socket_desc , client_sock , c,rec;
    struct sockaddr_in server , client;
    struct Vertice* ver=(Vertice*) ptr;
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }

	int connect_port = SOURCE->port; //source port
	int nb_name_len = strlen(ver->Name);
	int i;
	
	for(i=0; i < nb_name_len; i++){
		connect_port += ver->Name[i]; //port + neighbour name
	}
	printf("Port to recieve from:%d Router:%s\n",connect_port,ver->Name);
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( connect_port);

   //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error in reciver");
        exit(1);
    }

    //Listen
    if(listen(socket_desc , 10)==-1){
    		perror("listening");
		pthread_exit(NULL);
    
    }

    c = sizeof(struct sockaddr_in);


    //Accept incoming connection
    puts("Waiting for incoming connections...");
    client_sock = accept(socket_desc, (struct sockaddr *)&server, (socklen_t*)&c);
    if(client_sock<0){
        perror("error accepting");
        pthread_exit(NULL);
    }
    
    
    while(1){

        if(*nb_zero >= indv) //if calculation of the DV is finalized
			break; //exit
	
		int weight_buf[LENGTH+1];
		printf("reciever waiting for buffer recv\n");

        if((rec=recv(client_sock, weight_buf, sizeof(int)*(LENGTH+1), 0)) == -1)
        {
			perror("recieving");
			break;
		}
		printf("nd recieved %d\n",++nb_received[0]);
		if(rec==0){ printf("Connection closed by Router:%s on port:%d\n",ver->Name,connect_port); break;}
		/**********************************************WE HAVE */
		printf("printing received buffer\n");
		for(i=0;i<LENGTH+1;i++) printf("%d\t",weight_buf[i]);
		printf("\n");
	
		if(weight_buf[0] == 0){
			*nb_zero=*nb_zero+1;
		}
	
		if(weight_buf[0] == 1){
			//receive nb DV
			
			//copy only neighbour buffer to weight buffer
			int i,row;
			
			row=getnumber(dvNebours,ver->Name,indv+1);//takes the nebour name,then finds its index at the "nebours dv" for updating
			NebourConn[row][0]=0;
			for (i = 1; i < LENGTH+1; i++) {//first index 0 is not important
				 
				NebourConn[row][i] = weight_buf[i];
				printf("nb:%d\t",NebourConn[row][i]);
				
			}printf("\n");
		}
		
		if(	(weight_buf[0] == 1/*Got DV*/ || 
			(weight_buf[0] == 0) ||
			(nb_received[0] <= indv)) 
			)
		{
			
			//wake calculator
			pthread_mutex_lock(&calcMutex);
			pthread_cond_signal(&calcCond);
			pthread_mutex_unlock(&calcMutex);
		}
            
    }
    //finish while
    
    printf("Receiver finshing\n");
    close(socket_desc);
    pthread_exit(NULL);
}

void* ThreadCalculator ( void *ptr ){

	printf("in caculator, nb_zero: %d indv: %d\n",*nb_zero,indv);

	int wieght,i,j,row;
	while(*nb_zero < indv){ //while not got all 0s - as we process first element in DV=0? start calculate: still changing

		pthread_mutex_lock(&calcMutex);

		if(	(nb_received[0] < indv) || //not got all DVs
			(*nb_zero< indv) )
		{	  //not got all 0s	
				printf("calculator is waiting, nb_zero=%d indv=%d\n",*nb_zero,indv);
				pthread_cond_wait(&calcCond, &calcMutex); //wait
				printf("calculator is awaken\n");
		}

		//relax
		SOURCE->dv[0]=0;
		printf("in caculator, nb_zero: %d indv: %d\n",*nb_zero,indv);
		nb_received[0]=0;
		for(j=0;j<indv;j++){
			wieght=g->Connections[getnumber(g->assigns,SOURCE->Name,LENGTH)][getnumber(g->assigns,dvNebours[j],LENGTH)];
			printf("nebourRouter:%s wieght:%d\n",dvNebours[j],wieght);

			//row=j;printf("calculator 569, row:%d\n",row);
			//Bellman Ford Showing Off
			for(i=0;i<LENGTH+1;i++)
			{
				if(NebourConn[row][i]!=0 && NebourConn[row][i]!=INT_MAX){
					if(wieght+NebourConn[row][i]<SOURCE->dv[i]){
						SOURCE->dv[i]=wieght+NebourConn[row][i];
						SOURCE->dv[0]=1;//source dv was changed, is not it!!?
					}
			
				}
			}
		
		}
		
		
		
		printf("In caculator All nebour connection right now:\n");
		printf("row1: "); printint(NebourConn[0],LENGTH+1);
		
		printf("In caculator All SOURCE connection right now:\n");
		printf("row1: "); printint(SOURCE->dv,LENGTH+1);
		printf("\n");
		
		pthread_cond_signal(&weightCond);	//wake sender

		pthread_mutex_unlock(&calcMutex);
	
	}
		
	
	printf("***********************MAIN ANSWER:\n\n");//code finished
	for(i=0;i<LENGTH+1;i++){
	if(SOURCE->dv[i+1]!=0 && SOURCE->dv[i+1]!=INT_MAX){
		printf("distance:%s  wieght:%d\n",g->assigns[i],SOURCE->dv[i+1]);
		}
	}
	printf("\n****************************\n");
	pthread_mutex_destroy(&weightMutex);
	pthread_cond_destroy(&weightCond);

	pthread_mutex_destroy(&calcMutex);
	pthread_cond_destroy(&calcCond);
	pthread_exit(NULL); //exit
}
