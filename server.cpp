#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <libvirt/libvirt.h>

/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
}thData;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);

int getProp_Ident(char msg_recive[1024],char Prop_rez[256],char Ident_rez[256]);

int getPropFromVM(char Prop[256],char Ident[256],char Rez[20][256]);


int main ()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	
  int nr;		//mesajul primit de trimis la client 
  int sd;		//descriptorul de socket 
  int pid;
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
	int i=0;
  

  /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }
  /* utilizarea optiunii SO_REUSEADDR */
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));
  
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;	
  /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
    server.sin_port = htons (PORT);
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 2) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }
  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
    {
      int client;
      thData * td; //parametru functia executata de thread     
      int length = sizeof (from);

      printf ("[server]Asteptam la portul %d...\n",PORT);
      fflush (stdout);

      // client= malloc(sizeof(int));
      /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
      if ( (client = accept (sd, (struct sockaddr *) &from,(socklen_t*)&length)) < 0)
	{
	  perror ("[server]Eroare la accept().\n");
	  continue;
	}
	
        /* s-a realizat conexiunea, se astepta mesajul */
    
	// int idThread; //id-ul threadului
	// int cl; //descriptorul intors de accept

	td=(struct thData*)malloc(sizeof(struct thData));	
	td->idThread=i++;
	td->cl=client;

	pthread_create(&th[i], NULL, &treat, td);	      
				
	}//while    
};				
static void *treat(void * arg)
{		
		struct thData tdL; 
		tdL= *((struct thData*)arg);	
		printf ("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
		fflush (stdout);		 
		pthread_detach(pthread_self());		
		raspunde((struct thData*)arg);
		/* am terminat cu acest client, inchidem conexiunea */
		close ((intptr_t)arg);
		return(NULL);	
  		
};


void raspunde(void *arg)
{
	struct thData tdL; 
	tdL= *((struct thData*)arg);
	
  int is_open=1;
  char msg_send[1024];int size_msg_send;
  char msg_recive[1024];int size_msg_recive;
  while(is_open)
  {
    bzero(&size_msg_recive,sizeof(int));///cleaning recive vars
    bzero(msg_recive,1024*sizeof(char));

    if(read(tdL.cl,&size_msg_recive,sizeof(int))<=0)
    {
      perror("[server]Error at read()\n");
    }
    if(read(tdL.cl,msg_recive,size_msg_recive)<=0)
    {
      perror("[server]Error at read()\n");
    }

    printf("[server]Got %s of size %d\n",msg_recive,size_msg_recive);
    ///Analizam raspunsul:
    /*===================================================================================*/
    /*                               get <prop> <ident>                                  */
    if(strncmp(msg_recive,"get",strlen("get"))==0)
    {  
      char propriety[256];
      char ident[256];
      if(getProp_Ident(msg_recive,propriety,ident)==-1)///wrong syntax at get <> <>
      {
        printf("[server]Wrong syntax on call : get <prop> <ident>\n");
        bzero(&size_msg_send,sizeof(int));///cleaning send vars
        bzero(msg_send,1024*sizeof(char));

        strcpy(msg_send,"Wrong syntax get");
        size_msg_send=strlen(msg_send)+1;
        printf("[server]Sending %s of size %d\n",msg_send,size_msg_send);

        if(write(tdL.cl,&size_msg_send,sizeof(int))<0)
        {
          perror("[server]Error at write()\n");
        }
        if(write(tdL.cl,msg_send,size_msg_send)<0)
        {
          perror("[server]Error at write()\n");
        }
        continue;
      }
      printf("[server]Propriety is %s\n",propriety);
      printf("[server]Ident is %s\n",ident);

      

    }
    else
    /*=========================================================================================*/
    /*                                    CLOSE                                                */
    if(strcmp(msg_recive,"close")==0)
    {
      is_open=0;
      
      bzero(&size_msg_send,sizeof(int));///cleaning send vars
      bzero(msg_send,1024*sizeof(char));

      strcpy(msg_send,"close");
      size_msg_send=strlen(msg_send)+1;
      printf("[server]Sending %s of size %d\n",msg_send,size_msg_send);

      if(write(tdL.cl,&size_msg_send,sizeof(int))<0)
      {
        perror("[server]Error at write()\n");
      }
      if(write(tdL.cl,msg_send,size_msg_send)<0)
      {
        perror("[server]Error at write()\n");
      }
      printf("[server]Client on thread %d is closing\n",tdL.idThread);
    }
    else                                            
    /*=====================================================================================*/
    /*                                UNKNOWN COMMAND                                      */
    {
      bzero(&size_msg_send,sizeof(int));///cleaning send vars
      bzero(msg_send,1024*sizeof(char));

      strcpy(msg_send,"Unknown");
      size_msg_send=strlen(msg_send)+1;
      printf("[server]Sending %s of size %d\n",msg_send,size_msg_send);

      if(write(tdL.cl,&size_msg_send,sizeof(int))<0)
      {
        perror("[server]Error at write()\n");
      }
      if(write(tdL.cl,msg_send,size_msg_send)<0)
      {
        perror("[server]Error at write()\n");
      }
    }
  } 

}



int getProp_Ident(char msg_recive[1024],char Prop_rez[256],char Ident_rez[256])
{
  char *token=strtok(msg_recive," ");
  int word_nr=0;
  while (token!=nullptr)
  {
    token=strtok(nullptr," ");
    word_nr++;
    if(word_nr==1)
    {
      strcpy(Prop_rez,token);
    }
    if(word_nr==2)
    {
      strcpy(Ident_rez,token);
    }
  }
  if(word_nr!=3)
    return -1;
  return 1;
};


int getPropFromVM(char Prop[256],char Ident[256],char Rez[20][256])
{
  
}