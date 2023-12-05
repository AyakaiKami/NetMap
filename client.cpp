/* cliTCPIt.c - Exemplu de client TCP
   Trimite un numar la server; primeste de la server numarul incrementat.
         
   Autor: Lenuta Alboaie  <adria@info.uaic.ro> (c)
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

int main (int argc, char *argv[])
{
  int sd;			// descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
  		// mesajul trimis
  int nr=0;
  char buf[10];

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  /* stabilim portul */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);
  
  /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }

  int is_open=1;
  char msg_send[1024];int size_msg_send;
  char msg_recive[1024];int size_msg_recive;
  fflush(stdout);
  while(is_open)
  {
    printf("Waiting command : ");
    fflush(stdout);
    bzero(&size_msg_send,sizeof(int));///cleaning send vars 
    bzero(msg_send,1024*sizeof(char));

    if(read(0,msg_send,1024)<=0)        ////reading command
    {
      perror("[client]Error at read\n");
    }
    msg_send[strlen(msg_send)-1]='\0';
    size_msg_send=strlen(msg_send)+1;
    printf("[client]Sending %s of size %d",msg_send,size_msg_send);
    
    if(write(sd,&size_msg_send,sizeof(int))<0)///sending size of msg
    {
      perror("[client]Error at write()\n");
    }
    if(write(sd,msg_send,size_msg_send)<0)///sending msg
    {
      perror("[client]Error at write()\n");
    }


    bzero(&size_msg_recive,sizeof(int));///cleaning output vars
    bzero(msg_recive,1024*sizeof(char));

    if(read(sd,&size_msg_recive,sizeof(int))<=0)
    {
      perror("[client]Error at read()\n");
    }
    if(read(sd,msg_recive,size_msg_recive)<=0)
    {
      perror("[client]Error at read()\n");
    }

    printf("[client]Got %s of size %d\n",msg_recive,size_msg_recive);
    ///Analizam raspunsul:
    /*====================================================================*/
    /*                      Info about a VM                               */
    if(strcmp(msg_recive,"Info about a VM")==0)
    {
      printf("[client]Getting info about a VM :\n");

      int nr_lines;
      if(read(sd,&nr_lines,sizeof(int))<=0)
      {
        perror("[client]Error at read()\n");
      }

      if(nr_lines<0)
      {
        printf("[client]Could not find VM or error on connection to hypervisor\n");
        continue;
      }
      for(int i=0;i<nr_lines;i++)
      {
        bzero(&size_msg_recive,sizeof(int));///cleaning output vars
        bzero(msg_recive,1024*sizeof(char));

        if(read(sd,&size_msg_recive,sizeof(int))<=0)
        {
          perror("[client]Error at read()\n");
        }
        if(read(sd,msg_recive,size_msg_recive)<=0)
        {
         perror("[client]Error at read()\n");
        }

        printf("[client]%s\n",msg_recive);///printing msg
      }
    }
    /*===================================================================*/
    /*                      ERRORS/DATA NOT FOUND                        */
    if(strcmp(msg_recive,"Wrong syntax get")==0)
    {
      printf("[client]Wrong syntax get <prop> <ident>\n");
    }else
    if(strcmp(msg_recive,"Could not obtain data")==0)
    {
      printf("[client]Could not obtain data\n");
    }else
    /*===================================================================*/
    /*                         CLOSE                                    */
    if(strcmp(msg_recive,"close")==0)
    {
      is_open=0;
    }else
    /*===================================================================*/
    /*                         UNKNOWN COMMAND                           */
    if(strcmp(msg_recive,"Unknown")==0)
    {
        printf("[client]Unknown command\n");
    }

  }
  /* inchidem conexiunea, am terminat */
  close (sd);
};