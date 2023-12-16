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
#include <map>
#include <string.h>
#include <string>
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

  fflush(stdout);
  
  int wpipe[2];///parent writes to child
  int rpipe[2];///parent read from child

  
  int is_window_on=0;
  pid_t pid_window;
  
  if(pipe(wpipe)==-1)
  {
    exit(EXIT_FAILURE);
  }
  if(pipe(rpipe)==-1)
  {
    exit(EXIT_FAILURE);
  }
  if((pid_window=fork())==-1 )
  {
    exit(EXIT_FAILURE);
  }
  if(pid_window==0)///child
  {
    close(wpipe[1]);////child cant write with this pipe
    close(rpipe[0]);////child cant read with this pipe
    sleep(1);
    ////new connection to the client child
  
    int sd_child;
    struct sockaddr_in server_child;
    
    int port_c;
    bzero(&port_c,sizeof(int));
  
    if(read(wpipe[0],&port_c,sizeof(port_c))<0)
    {
      perror("Did not get port\n");
      
    }
    if((sd_child=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
      perror("Eror at socket()\n");
      return errno;
    }
  
    server_child.sin_family=AF_INET;
    server_child.sin_addr.s_addr=inet_addr(argv[1]);
    server_child.sin_port=htons(port_c);

    if (connect (sd_child, (struct sockaddr *) &server_child,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client_child]Eroare la connect().\n");
      return errno;
    }
    int is_on=1;
    char msg_from_parent[1024];
    int size_msg_from_parent;

    char msg_from_server[1024];
    int size_msg_from_server;
    char msg_to_server[1024];
    int size_msg_to_server;
    while(is_on)
    {
      bzero(&size_msg_from_parent,sizeof(int));
      bzero(msg_from_parent,1024*sizeof(char));
      if(read(wpipe[0],&size_msg_from_parent,sizeof(int))<0)
      {
        perror("[client_child]Could not get size of msg\n");
      }
      if(read(wpipe[0],msg_from_parent,size_msg_from_parent)<0)
      {
        perror("[client_child]Could not get msg\n");
      }
    
      if(strcmp(msg_from_parent,"close hexagram")==0)
      {
        printf("[client_child]Hexagram not open\n");
      }

      if(strcmp(msg_from_parent,"close parabola")==0)
      {
        printf("[client_child]Parabola not open\n");
      }
      if(strcmp(msg_from_parent,"hexagram")==0)
      {
        int hexagram_on=1;
        bzero(&size_msg_to_server,sizeof(int));
        bzero(msg_to_server,1024);
        strcpy(msg_to_server,"hexagram");
        size_msg_to_server=strlen(msg_to_server)+1;
        if(write(sd_child,&size_msg_to_server,sizeof(int))<=0)
        {
          perror("[client_child]Error write\n");
        }
        if(write(sd_child,msg_to_server,size_msg_to_server)<=0)
        {
          perror("[client-child]Error at write\n");
        }

        std::map<std::string,std::string>vm_con;
        ///Getting first list of connections
        size_t mapSize;
        if(read(sd_child,&mapSize,sizeof(size_t))<0)
        {
          perror("[client_child]Error at read\n");
        }
        
        for(size_t i=0;i<mapSize;i++)
        {
          //key
          size_t keys;
          if(read(sd_child,&keys,sizeof(size_t))<0)
          {
            perror("[client_child]Error at read\n");
          }
          char keyB[keys+1];
          if(read(sd_child,keyB,keys)<0)
          {
            perror("[client_child]Error at read\n");
          }
          keyB[keys]='\0';

          //value
          size_t values;
          if(read(sd_child,&values,sizeof(size_t))<0)
          {
            perror("[client_child]Error at read\n");
          }
          char valueB[values+1];
          if(read(sd_child,valueB,values)<0)
          {
            perror("[client_child]Error at read\n");
          }
          valueB[values]='\0';
         
          ///key into map
          vm_con[keyB]=valueB;
        }
        
        
        while (hexagram_on)
        {
          ///from parent
          bzero(&size_msg_from_parent,sizeof(int));
          bzero(msg_from_parent,1024*sizeof(char));          
          if(read(wpipe[0],&size_msg_from_parent,sizeof(int))<0)
          {
            perror("[client-child]Could note get size of msg\n");
          }
          if(read(wpipe[0],msg_from_parent,size_msg_from_parent)<0)
          {
            perror("[client-child]Could not get msg\n");
          }
        
          ///from server
          bzero(&size_msg_from_server,sizeof(int));
          bzero(msg_from_server,1024*sizeof(char));          
          if(read(sd_child,&size_msg_from_server,sizeof(int))<0)
          {
            perror("[client_child]Could not obtain size of msg\n");
          }
          if(read(sd_child,msg_from_server,size_msg_from_server)<0)
          {
            perror("[client_child]Could not obtain msg\n");
          }
        
          if(strcmp(msg_from_parent,"close hexagram")==0)
          {
            hexagram_on=0;
            bzero(&size_msg_to_server,sizeof(int));
            bzero(msg_to_server,1024);
            strcpy(msg_to_server,"close hexagram");
            size_msg_to_server=strlen(msg_to_server)+1;
            if(write(sd_child,&size_msg_to_server,sizeof(int))<=0)
          {
            perror("[client_child]Error write\n");
          }
            if(write(sd_child,msg_to_server,size_msg_to_server)<=0)
          {
            perror("[client-child]Error at write\n");
          }
            continue;
          }
          else
          {
            perror("[client_child]Error unknown\n");
          }

          if(strcmp(msg_from_server,"new list")==0)
          {
            vm_con.clear();///empty the map
            bzero(&mapSize,sizeof(size_t));
            if(read(sd_child,&mapSize,sizeof(size_t))<0)
            {
            perror("[client_child]Error at read\n");
            }
        
            for(size_t i=0;i<mapSize;i++)
        {
          //key
          size_t keys;
          if(read(sd_child,&keys,sizeof(size_t))<0)
          {
            perror("[client_child]Error at read\n");
          }
          char keyB[keys+1];
          if(read(sd_child,keyB,keys)<0)
          {
            perror("[client_child]Error at read\n");
          }
          keyB[keys]='\0';

          //value
          size_t values;
          if(read(sd_child,&values,sizeof(size_t))<0)
          {
            perror("[client_child]Error at read\n");
          }
          char valueB[values+1];
          if(read(sd_child,valueB,values)<0)
          {
            perror("[client_child]Error at read\n");
          }
          valueB[values]='\0';
         
          ///key into map
          vm_con[keyB]=valueB;
        }
             
          }
        }
        
      }
    

    }
    close(wpipe[0]);
    close(rpipe[1]);
    close(sd_child);
    exit(EXIT_SUCCESS);
  }
  
  
  
  /*==============================PARENT==================================*/
  close(wpipe[0]);
  close(rpipe[1]);
  int port_empty;
  if(read(sd,&port_empty,sizeof(int))<0)
  {
    perror("[client]Read error\n");
  }
 
  if(write(wpipe[1],&port_empty,sizeof(int))<0)///sending port to child
  {
    perror("[client]Write error\n");
  }

  int is_open=1;
  char msg_send[1024];int size_msg_send;
  char msg_recive[1024];int size_msg_recive;
  
  char msg_to_child[1024];///between parent and child
  int size_msg_to_child;
  while(is_open)
  {
    ///null to child
    bzero(&size_msg_to_child,sizeof(int));
    bzero(msg_to_child,1024*sizeof(char));
    strcpy(msg_to_child,"NULL");
    size_msg_to_child=strlen(msg_to_child)+1;
    if(write(wpipe[1],&size_msg_to_child,sizeof(int))<=0)
    {
      perror("[client]Error at write\n");
    }
    if(write(wpipe[1],msg_to_child,size_msg_to_child)<=0)
    {
      perror("[client]Error at write\n");
    }

    ///to server
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
    printf("[client]Sending %s of size %d\n",msg_send,size_msg_send);
    
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
    /*==================================================================*/
    /*                           CLOSE PARABOLA                         */
     if(strcmp(msg_recive,"close parabola")==0)
    {
      bzero(&size_msg_to_child,sizeof(int));
      bzero(msg_to_child,1024*sizeof(char));

      strcpy(msg_to_child,"close parabola");
      size_msg_to_child=strlen(msg_to_child)+1;
 
      if(write(wpipe[1],&size_msg_to_child,sizeof(int))<0)///sending port to child
      {
      perror("[client]Write error\n");
      }

      if(write(wpipe[1],msg_to_child,size_msg_to_child)<0)///sending port to child
      {
        perror("[client]Write error\n");
      }
      continue;      
    }else   
    /*==================================================================*/
    /*                            PARABOLA                              */
    if(strcmp(msg_recive,"parabola")==0)
    {
      bzero(&size_msg_to_child,sizeof(int));
      bzero(msg_to_child,1024*sizeof(char));

      strcpy(msg_to_child,"parabola");
      size_msg_to_child=strlen(msg_to_child)+1;
 
      if(write(wpipe[1],&size_msg_to_child,sizeof(int))<0)///sending port to child
      {
      perror("[client]Write error\n");
      }

      if(write(wpipe[1],msg_to_child,size_msg_to_child)<0)///sending port to child
      {
        perror("[client]Write error\n");
      }
      continue;      
    }else
    /*=================================================================*/
    /*                         CLOSE HEXAGRAM                          */
    if(strcmp(msg_recive,"close hexagram")==0)
    {
      bzero(&size_msg_to_child,sizeof(int));
      bzero(msg_to_child,1024*sizeof(char));

      strcpy(msg_to_child,"close hexagram");
      size_msg_to_child=strlen(msg_to_child)+1;
 
      if(write(wpipe[1],&size_msg_to_child,sizeof(int))<0)///sending port to child
      {
      perror("[client]Write error\n");
      }

      if(write(wpipe[1],msg_to_child,size_msg_to_child)<0)///sending port to child
      {
        perror("[client]Write error\n");
      }
      continue;
    }else
    /*================================================================*/
    /*                            HEXAGRAM                            */
    if(strcmp(msg_recive,"hexagram")==0)
    {
      bzero(&size_msg_to_child,sizeof(int));
      bzero(msg_to_child,1024*sizeof(char));

      strcpy(msg_to_child,"hexagram");
      size_msg_to_child=strlen(msg_to_child)+1;
 
      if(write(wpipe[1],&size_msg_to_child,sizeof(int))<0)///sending port to child
      {
      perror("[client]Write error\n");
      }

      if(write(wpipe[1],msg_to_child,size_msg_to_child)<0)///sending port to child
      {
        perror("[client]Write error\n");
      }
      continue;
    }else
    /*===============================================================*/
    /*                          LIST                                 */
    if(strcmp(msg_recive,"list could not be made")==0)
    {
      printf("[client]Server could not send list\n");
      continue;
    }
    else
    if(strcmp(msg_recive,"list was sent")==0)
    {
      int lines;
      if(read(sd,&lines,sizeof(int))<=0)
      {
        perror("[client]Error at read()\n");
      }

      printf("IDs are :\n");
      if(lines==0)
      {
        printf("None\n");
      }
      for(int i=0;i<lines;i++)
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

        printf("%s\n",msg_recive);
      }
    }else
    /*==============================================================*/
    /*                          HELP                                */
    if(strcmp(msg_recive,"help.txt")==0)
    {
      printf("[client]Info : \n");

      bzero(&size_msg_recive,sizeof(int));

      if(read(sd,&size_msg_recive,sizeof(int))<0)
      {
        perror("[client]Error at read()\n");
      }

      while(size_msg_recive>=0)
      {
        bzero(msg_recive,1024*sizeof(char));
        if(read(sd,msg_recive,size_msg_recive)<0)
        {
          perror("[client]Error at read()\n");
        }
        printf("%s",msg_recive);

        bzero(&size_msg_recive,sizeof(int));

      if(read(sd,&size_msg_recive,sizeof(int))<0)
      {
        perror("[client]Error at read()\n");
      }
      }
      printf("\n");
      
    }else
    /*====================================================================*/
    /*                          ERROR HELP                                */
    if(strcmp(msg_recive,"Unable to send help :(")==0)
    {
      printf("[client]Unable to print help\n");
    }
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
  close(wpipe[1]);
  close(rpipe[0]);
  /* inchidem conexiunea, am terminat */
  close (sd);
};