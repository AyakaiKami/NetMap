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
#include <libvirt/libvirt-qemu.h>
#include <arpa/inet.h>
#include <sstream>
#include <ctime>
#include <sqlite3.h>
#include <vector>
#include <map>
#include <cstring>
#include <chrono>
#include <iostream>
#include <ctime>
#include <iomanip>
//#include <jsoncpp/json/json.h>
/* portul folosit */
#define PORT 2908

int get_Port() 
{
    FILE* fd = fopen("port.txt", "r+");
    if (fd == nullptr) {
        perror("Error opening port.txt");
        exit(EXIT_FAILURE);
    }

    int port;
    if (fscanf(fd, "%d", &port) != 1) {
        perror("Error reading port from port.txt");
        fclose(fd);
        exit(EXIT_FAILURE);
    }

    // Set the new empty port
    int newEmptyPort = port + 1;
    fseek(fd, 0, SEEK_SET);  // Move file pointer to the beginning
    fprintf(fd, "%d", newEmptyPort);  // Write the new empty port to the file

    fclose(fd);
    return port;
};


/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
}thData;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);

int getProp_Type_Ident(char msg_recive[1024],char Prop_rez[256],char Type_rez[256],char Ident_rez[256]);

int getPropFromVM(char Prop[256],char Type[256],char Ident[256],char Rez[50][256]);

double getCPULoad(virDomainPtr vm);
struct vm_info
{
  char name[256];
  char CPU_number[256];
  char CPU_time[256];
  char RAM[256];
  char state[256];
  int interface_nr;
  char interface_list[25][256];
  char ip_address[25][256];
  char load[256];

};

struct Tree_vms
{
  char name[256];
  std::vector<Tree_vms*>*connections;
};

void saveVMS(std::vector<vm_info*> list_vm_info);

std::vector<Tree_vms*>* hexagram();

Tree_vms* makeGraph(vm_info &current_vm,std::vector<vm_info*>&list_vm_info,std::vector<int>&marked_list);

vm_info* vm_data_make(virDomainPtr vm);

std::vector<Tree_vms*>* parabola(int id_save);

int list_vms(char Rez[25][1024]);

int vm_con(vm_info vm1,vm_info vm2);////are 2 vm s connected?

int sendTreeList(int fd,std::vector<Tree_vms*>*list);


int sendTree(int fd,Tree_vms*tree);

int parabolCallBack(void* data, int argc, char** argv, char** azColName);

int parabol(char Rez[1024]);


int intf_parabolaCallBack(void* data, int argc, char** argv, char** azColName);


///====================================START==============================================================

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
	
  int is_window_on=0;
  pid_t pid_window;

  int is_open=1;
  char msg_send[1024];//parent
  int size_msg_send;//parent
  char msg_recive[1024];//parent
  int size_msg_recive;//parent

  int wpipe[2];///parent writes to child
  int rpipe[2];///parent read from child

  int port_empty=get_Port();  

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
////========================Child======================================
  if(pid_window==0)///child
  {
    close(wpipe[1]);////child cant write with this pipe
    close(rpipe[0]);////child cant read with this pipe
  
    ////new connection to the client child
    int serverChildSocket=socket(AF_INET,SOCK_STREAM,0);
    if(serverChildSocket==-1)
    {
      printf("[server]Failed\n");
      exit(EXIT_FAILURE);
    }
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port_empty);
    if (bind(serverChildSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) 
    {
      printf("Error binding socket\n");
      close(serverChildSocket);
      close(wpipe[0]);
      close(rpipe[1]);
      exit(EXIT_FAILURE);
    }

    printf("[server_child]Waiting at port %d\n",port_empty);
    if (listen(serverChildSocket, 5) == -1) 
    { 
      printf("Error listening for child client connections\n");
      close(serverChildSocket);
      close(wpipe[0]);
      close(rpipe[1]);
      exit(EXIT_FAILURE);
    }
    sockaddr_in clientAddress;
    socklen_t clientAddrLen = sizeof(clientAddress);
    int clientSocket = accept(serverChildSocket, (struct sockaddr*)&clientAddress, &clientAddrLen);
    if (clientSocket == -1) 
    {
        printf("Error accepting connection\n");
        close(serverChildSocket);
        close(wpipe[0]);
        close(rpipe[1]);
        exit(EXIT_FAILURE);
    }
    //printf("Connected child\n");
    /*=============================================================================*/
    /*                             Child Connected                                 */
    int on=1;
    int size_msg_client_child;
    char msg_client_child[1024];
    while (on)
    {
      bzero(&size_msg_client_child,sizeof(int));
      bzero(msg_client_child,1024*sizeof(char));

      if(read(clientSocket,&size_msg_client_child,sizeof(int))<=0)
      {
        perror("[server_child]Read \n");
        close(clientSocket);
        exit(EXIT_FAILURE);
      }
      if(read(clientSocket,msg_client_child,size_msg_client_child)<=0)
      {
        perror("[server_child]Read \n");
        close(clientSocket);
        exit(EXIT_FAILURE);
      }

      //printf("[server_child]Got %s of size %d from client_child\n",msg_client_child,size_msg_client_child);
/*===============================================================================*/
/*                           HEXAGRAM                                   */
      if(strcmp(msg_client_child,"hexagram")==0)
      {
        int hon=1;
        std::vector<Tree_vms*>*listT=hexagram();
        sendTreeList(clientSocket,listT);
        auto start=std::chrono::steady_clock::now();
        
        while (hon)
        {
          bzero(&size_msg_client_child,sizeof(int));
          bzero(msg_client_child,1024*sizeof(char));
          if(read(clientSocket,&size_msg_client_child,sizeof(int))<0)
          {
            perror("[server_child]Error at read\n");
          } 
          if(read(clientSocket,msg_client_child,size_msg_client_child)<0)
          {
            perror("[server_child]Error at read\n");
          }
          
          printf("[server_child]New msg from client \n %s of size %d\n",msg_client_child,size_msg_client_child);
          if(strcmp(msg_client_child,"close hexagram")==0)
          {
            printf("[server_child]Closing hexagram\n");
            hon=0;
            continue;
          }
          auto end=std::chrono::steady_clock::now();
          auto dif=std::chrono::duration_cast<std::chrono::seconds>(end-start);
          std::cout<<dif.count()<<std::endl;
          if(dif.count()>=20)
          {
            start=std::chrono::steady_clock::now();
            printf("[server_child]New list\n");
            free(listT);listT=hexagram();
            bzero(&size_msg_client_child,sizeof(int));
            bzero(msg_client_child,1024*sizeof(char));
            strcpy(msg_client_child,"new list");size_msg_client_child=strlen(msg_client_child)+1;
            if(write(clientSocket,&size_msg_client_child,sizeof(int))<=0)
            {
              perror("[server-child]Error at write\n");
            }
            if(write(clientSocket,msg_client_child,size_msg_client_child)<=0)
            {
              perror("[server-child]Error at write\n");
            }            
            sendTreeList(clientSocket,listT);
          }
          else
          {
            printf("[server_child]Not a new list\n");
            bzero(&size_msg_client_child,sizeof(int));
            bzero(msg_client_child,1024*sizeof(char));
            strcpy(msg_client_child,"none");size_msg_client_child=strlen(msg_client_child)+1;            
           if(write(clientSocket,&size_msg_client_child,sizeof(int))<=0)
            {
              perror("[server-child]Error at write\n");
            }
            if(write(clientSocket,msg_client_child,size_msg_client_child)<=0)
            {
              perror("[server-child]Error at write\n");
            }            
          }
        }
      }
/*================================================================================*/
/*                           PARABOLA                                     */

      if(strncmp(msg_client_child,"parabola",strlen("parabola"))==0)
      {
        int id_save;
        ///get id_save
        int ind=strlen("parabola");
        while(msg_client_child[ind]==' ')
        {
          ind++;
        }
        char nr[11];int inr=0;
        while(msg_client_child[ind]>='0' && msg_client_child[ind]<='9')
        {
          nr[inr]=msg_client_child[ind];
        }
        id_save=atoi(nr);

        std::vector<Tree_vms*>*list=parabola(id_save);
        sendTreeList(clientSocket,list);
        int pton=1;
        while (pton)
        {
          if(read(clientSocket,&size_msg_client_child,sizeof(int))<=0)
          {
            perror("[server_child]Read \n");
            close(clientSocket);
            exit(EXIT_FAILURE);
          }
          if(read(clientSocket,msg_client_child,size_msg_client_child)<=0)
          {
            perror("[server_child]Read \n");
            close(clientSocket);
            exit(EXIT_FAILURE);
          }
          
          if(strcmp(msg_client_child,"close parabola")==0)
          {
            printf("Closing parabola\n");
            pton=0;
            continue;
          }
        }
      }
/*==========================================================================*/
/*                             CLOSE                                    */
      if(strcmp(msg_client_child,"close")==0)
      {
        on=0;
        printf("[server_child]Closing\n");
        continue;
      }


    }
    
    close(wpipe[0]);
    close(rpipe[1]);
    close(clientSocket);

    exit(EXIT_SUCCESS);
  }
  
  //////==============PARENT=====================
  close(wpipe[0]);
  close(rpipe[1]);
  
  if(write(tdL.cl,&port_empty,sizeof(int))<0)
  {
    perror("[server]Error sending port\n");
  }

  while(is_open)
  {
    printf("[server]Waitting for input on thread : %d \n",tdL.idThread);
    bzero(&size_msg_recive,sizeof(int));///cleaning recive vars
    bzero(msg_recive,1024*sizeof(char));

    if(read(tdL.cl,&size_msg_recive,sizeof(int))<0)
    {
      perror("[server]Error at read()\n");
    }
    if(read(tdL.cl,msg_recive,size_msg_recive)<0)
    {
      perror("[server]Error at read()\n");
    }

   // printf("[server]Got %s of size %d\n",msg_recive,size_msg_recive);
    ///Analizam raspunsul:

    if(strcmp(msg_recive,"parabol")==0)
    {
      char Rez[1024];parabol(Rez);
      bzero(&size_msg_send,sizeof(int));///cleaning send vars
      bzero(msg_send,1024*sizeof(char));

      strcpy(msg_send,"parabol");
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

      int size_Rez;bzero(&size_Rez,sizeof(int));
      size_Rez=strlen(Rez)+1;
      if(write(tdL.cl,&size_Rez,sizeof(int))<0)
      {
        perror("[server]Error at write()\n");
      }
      if(write(tdL.cl,Rez,size_Rez)<0)
      {
        perror("[server]Error at write()\n");
      }
      continue;
    }
    else
    /*======================================================================*/
    /*                           CLOSE PARABOLA                             */
   if(strcmp(msg_recive,"close parabola")==0)
    {
      bzero(&size_msg_send,sizeof(int));///cleaning send vars
      bzero(msg_send,1024*sizeof(char));

      strcpy(msg_send,"close parabola");
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
    }else        
    /*======================================================================*/
    /*                              PARABOLA                                */
   if(strncmp(msg_recive,"parabola",strlen("parabola"))==0)
    {
      bzero(&size_msg_send,sizeof(int));///cleaning send vars
      bzero(msg_send,1024*sizeof(char));

      strcpy(msg_send,msg_recive);
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
    }else        
    /*=====================================================================*/
    /*                            CLOSE HEXAGRAM                           */
    if(strcmp(msg_recive,"close hexagram")==0)
    {
      
      bzero(&size_msg_send,sizeof(int));///cleaning send vars
      bzero(msg_send,1024*sizeof(char));

      strcpy(msg_send,"close hexagram");
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
    }else    
    /*====================================================================*/
    /*                               HEXAGRAM                             */
    if(strcmp(msg_recive,"hexagram")==0)
    {
      bzero(&size_msg_send,sizeof(int));///cleaning send vars
      bzero(msg_send,1024*sizeof(char));

      strcpy(msg_send,"hexagram");
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
    }else
    /*====================================================================*/
    /*                         LIST                                       */
    if(strcmp(msg_recive,"list")==0)
    {
      char rez[25][1024];
      int lines;
      if((lines=list_vms(rez))<0)
      {
        bzero(&size_msg_send,sizeof(int));
        bzero(msg_send,1024*sizeof(char));

        strcpy(msg_send,"list could not be made");;
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

      bzero(&size_msg_send,sizeof(int));
      bzero(msg_send,1024*sizeof(char));

      strcpy(msg_send,"list was sent");
      size_msg_send=strlen(msg_send)+1;
      printf("[server]Sending %s of size %d\n",msg_send,size_msg_send);

      if(write(tdL.cl,&size_msg_send,sizeof(int))<0)
      {
        perror("[server]Error at write\n");
      }
      if(write(tdL.cl,msg_send,size_msg_send)<0)
      {
        perror("[server]Error at write\n");
      }

      if(write(tdL.cl,&lines,sizeof(int))<0)
      {
        perror("[server]Error at write\n");
      }
      for(int i=0;i<lines;i++)
      {
      bzero(&size_msg_send,sizeof(int));
      bzero(msg_send,1024*sizeof(char));

      strcpy(msg_send,rez[i]);
      size_msg_send=strlen(msg_send)+1;
      printf("[server]Sending %s of size %d\n",msg_send,size_msg_send);

      if(write(tdL.cl,&size_msg_send,sizeof(int))<0)
      {
        perror("[server]Error at write\n");
      }
      if(write(tdL.cl,msg_send,size_msg_send)<0)
      {
        perror("[server]Error at write\n");
      }        
      }
      continue;
    }
    else
    /*===================================================================================*/
    /*                               get <prop> <ident>                                  */
    if(strncmp(msg_recive,"get",strlen("get"))==0)
    {  
      char propriety[256];
      char type[256];
      char ident[256];
      if(getProp_Type_Ident(msg_recive,propriety,type,ident)==-1)///wrong syntax at get <> <>
      {
        printf("[server]Wrong syntax on call : get <prop> <type> <ident>\n");
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

      char rez[50][256];
      int lines_rez=getPropFromVM(propriety,type,ident,rez);

      if(lines_rez==-1)
      {
        printf("[server]Could not get %s from VM %s\n",propriety,ident);

        bzero(&size_msg_send,sizeof(int));///cleaning send vars
        bzero(msg_send,1024*sizeof(char));

        strcpy(msg_send,"Could not obtain data");
        size_msg_send=strlen(msg_send)+1;
        printf("[server]Sending %s of size %d\n",msg_send,size_msg_send);
        if(write(tdL.cl,&size_msg_send,sizeof(int))<0)
        {
          perror("[server]Error at write()\n");
        }
        if(write(tdL.cl,msg_send,size_msg_send)<0)
        {
          perror("[server]Error at write()\n");
        }continue;
      }else
      {
        printf("[server]Sending info about a VM\n");

        bzero(&size_msg_send,sizeof(int));///cleaning send vars
        bzero(msg_send,1024*sizeof(char));

        strcpy(msg_send,"Info about a VM");
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

        printf("[server]Sending number of lines\n");
        if(write(tdL.cl,&lines_rez,sizeof(int))<0)
        {
          perror("[server]Error at write()\n");
        }
        for(int i=0;i<lines_rez;i++)
        {
          bzero(&size_msg_send,sizeof(int));///cleaning send vars
          bzero(msg_send,1024*sizeof(char));

          strcpy(msg_send,rez[i]);
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
      continue;
    }
    else
    if(strcmp(msg_recive,"help")==0)
    {
      FILE* helpf=fopen("help.txt","r");
      if(helpf==nullptr)
      {
        bzero(&size_msg_send,sizeof(int));///cleaning send vars
        bzero(msg_send,1024*sizeof(char));

        strcpy(msg_send,"Unable to send help :(");
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
      

      bzero(&size_msg_send,sizeof(int));///cleaning send vars
      bzero(msg_send,1024*sizeof(char));
      strcpy(msg_send,"help.txt");

      size_msg_send = strlen(msg_send) + 1;
      printf("[server]Sending %s of size %d\n", msg_send, size_msg_send);

      if (write(tdL.cl, &size_msg_send, sizeof(int)) < 0) 
      {
        perror("[server] Error at write()\n");
      }
      if (write(tdL.cl, msg_send, size_msg_send) < 0) 
      {
        perror("[server] Error at write()\n");
      }

      bzero(&size_msg_send,sizeof(int));///cleaning send vars
      bzero(msg_send,1024*sizeof(char));
      
      while(fgets(msg_send,1024,helpf)!=nullptr)
      {
        
        size_msg_send = strlen(msg_send) + 1;
        printf("[server]Sending %s of size %d\n", msg_send, size_msg_send);

        if (write(tdL.cl, &size_msg_send, sizeof(int)) < 0) {
            perror("[server]Error at write()\n");
        }
        if (write(tdL.cl, msg_send, size_msg_send) < 0) {
            perror("[server]Error at write()\n");
        }
        bzero(&size_msg_send,sizeof(int));///cleaning send vars
        bzero(msg_send,1024*sizeof(char));
      }

      bzero(&size_msg_send,sizeof(int));
      size_msg_send=-1;
      if(write(tdL.cl,&size_msg_send,sizeof(int))<0)
      {
        perror("[server]Error at write()\n");
      }
      fclose(helpf);
      continue;
    }else
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
      continue;
    }
    /*=====================================================================================*/
    /*                                UNKNOWN COMMAND                                      */
    else                                            
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
      continue;
    }
  } 
  printf("[server]Closing client on thread %d is closing\n",tdL.idThread);
  close(wpipe[1]);
  close(rpipe[0]);
};



int getProp_Type_Ident(char msg_recive[1024],char Prop_rez[256],char Type_rez[256],char Ident_rez[256])
{
  char *token=strtok(msg_recive," ");
  int word_nr=1;
  while (token!=nullptr)
  {
    printf("Token %d : %s ",word_nr,token);
    if(word_nr==2)
    {
      strcpy(Prop_rez,token);
    }else 
    if(word_nr==3)
    {
      strcpy(Type_rez,token);
    }else
    if(word_nr==4)
    {
      strcpy(Ident_rez,token);
    }
    token=strtok(nullptr," ");
    word_nr++;
  }
  if(word_nr!=5)
    return -1;
  return 1;
};

int getPropFromVM(char Prop[256],char Type[256],char Ident[256],char Rez[50][256])
{
  int lines=0;///nr de linii
  virInitialize();///deschidem libvirt

  ///Ne conectam la hypervisor
  virConnectPtr con=virConnectOpen("qemu:///system");

  if(con==nullptr)///conexiunea nu s-a realizat
  {
    printf("[server]Failed to open connection\n");
    return -1;
  };

  virDomainPtr vm;///domeniu dupa IP/ID

  if(strcmp(Type,"ID")==0)///ne conectam prin ID
  {
    int domainID = atoi(Ident);
    vm=virDomainLookupByID(con,domainID);
  }else
  if(strcmp(Type,"IP")==0)///ne conectam prin IP
  {
    vm=virDomainLookupByUUIDString(con,Ident);
      
  }else
  if(strcmp(Type,"name")==0)///ne conectam prin nume
  {
    vm=virDomainLookupByName(con,Ident);
  }

  if(vm==nullptr)
  {
    virConnectClose(con);
    printf("[server]Could not find VM\n");
    return -1;
  }

  virDomainInfo vm_info;
  if(virDomainGetInfo(vm,&vm_info)!=0)
  {
    virDomainFree(vm);
    virConnectClose(con);
    return -1;
  }

///Get ID
  if(strcmp(Prop,"ID")==0 || strcmp(Prop,"all")==0)
  {
    sprintf(Rez[lines],"ID : %d",virDomainGetID(vm));
    lines++;
  }
///Get name
  if(strcmp(Prop,"name")==0 || strcmp(Prop,"all")==0)
  {
    sprintf(Rez[lines],"Name : %s",virDomainGetName(vm));
    lines++;
  }
///Get number of CPU and CPU time
  if(strcmp(Prop,"CPU")==0 || strcmp(Prop,"all")==0)
  {
    sprintf(Rez[lines],"CPU number : %d",vm_info.nrVirtCpu);
    lines++;
    sprintf(Rez[lines],"CPU time : %d ns",vm_info.nrVirtCpu);
    lines++;
  }

///Get RAM

  if(strcmp(Prop,"RAM")==0 || strcmp(Prop,"all")==0)
  {
    sprintf(Rez[lines],"Random Access Memory (RAM) :%d KB",vm_info.memory);
    lines++;
  }

///Get State
  if(strcmp(Prop,"state")==0 || strcmp(Prop,"all")==0)
  {
    int vm_state,reason;
    if(virDomainGetState(vm,&vm_state,&reason,0)!=0)
    {
      virDomainFree(vm);
      virConnectClose(con);
      return -1;
    }
    sprintf(Rez[lines],"State : %d ",vm_state);
    lines++;

  }
///Get interface
  if(strcmp(Prop,"interface")==0 || strcmp(Prop,"all")==0)
{
  virDomainInterfacePtr *ifaces=nullptr;
  int count_if;
  if((count_if=virDomainInterfaceAddresses(vm,&ifaces,VIR_DOMAIN_INTERFACE_ADDRESSES_SRC_LEASE,0))<0)
  {
    virDomainFree(vm);
    virConnectClose(con);
    return -1;
  }
  strcpy(Rez[lines],"Interfaces : ");
  lines++;
  for(int i=0;i<count_if;i++)
  {
    strcpy(Rez[lines],ifaces[i]->name);
    lines++;

    ///Get IP address
    _virDomainInterfaceIPAddress *ips=ifaces[i]->addrs;
    if(ips->type==VIR_IP_ADDR_TYPE_IPV4)
      sprintf(Rez[lines],"IP:%s", ips->addr);
    lines++;
  }
  for (int i = 0; i < count_if; i++)
            virDomainInterfaceFree(ifaces[i]);
  free(ifaces);
}
///Get load
  if(strcmp(Prop,"load")==0 || strcmp(Prop,"all")==0)
  {
    double load_rez=getCPULoad(vm);
    if(load_rez==-1.0)
    {
      virDomainFree(vm);
      virConnectClose(con);
      return -1;
    }
    sprintf(Rez[lines],"load : %f",load_rez);
    lines++;
  }

  ////free VM domain and close connection to hypervisor
  virDomainFree(vm);
  virConnectClose(con);
  return lines;
};

double getCPULoad(virDomainPtr vm)
{

  std::time_t s_time=std::time(nullptr);
  virDomainInfo cpu_info;
  if(virDomainGetInfo(vm,&cpu_info)!=0)
  {
    ///nu s-au putut obtine datele
    return -1.0;
  }

  unsigned long long s_cpu_time=cpu_info.cpuTime;

  sleep(1);

  if(virDomainGetInfo(vm,&cpu_info)!=0)
  {
    ///nu s-au putut obtine datele
    return -1.0;
  }

  std::time_t e_time=std::time(nullptr);
  unsigned long long e_cpu_time=cpu_info.cpuTime;

  unsigned long long dif_cpu_time=e_cpu_time-s_cpu_time;
  std::time_t dif_time=e_time-s_time;

  double load = (dif_cpu_time/ static_cast<double>(dif_time))*100.0;

  return load;
};


vm_info* vm_data_make(virDomainPtr vm)
{
  
  vm_info* vm_d=new vm_info;

  ///name
  strcpy(vm_d->name,virDomainGetName(vm));
  virDomainInfo vm_d_info;
  if(virDomainGetInfo(vm,&vm_d_info)!=0)
  {
    printf("[server]Eroare la obtinerea datelor\n");
    sprintf(vm_d->CPU_number,"NULL");
    sprintf(vm_d->CPU_time,"NULL");
    sprintf(vm_d->RAM,"NULL");
  }
  else
  {
  ///CPU
  sprintf(vm_d->CPU_number,"%d",vm_d_info.nrVirtCpu);
  sprintf(vm_d->CPU_time,"%d",vm_d_info.cpuTime);

  ///RAM
  sprintf(vm_d->RAM,"%d KB",vm_d_info.memory);
  
  }
  ///state
  int vm_state,reason;
  if(virDomainGetState(vm,&vm_state,&reason,0)!=0)
  {
    perror("[server]Eroare la obinerea starii\n");
    sprintf(vm_d->state,"NULL");
  }
  else
  sprintf(vm_d->state,"%d",vm_state);

  ///load

  double load_rez=getCPULoad(vm);
    if(load_rez==-1.0)
    {
      printf("[server]Eroare la getCPULoad\n");
      sprintf(vm_d->load,"NULL");
    }
    else
    sprintf(vm_d->load,"%f",load_rez);


  ///interface and ip address

  virDomainInterfacePtr *ifaces=nullptr;
  int count_if;
  if((count_if=virDomainInterfaceAddresses(vm,&ifaces,VIR_DOMAIN_INTERFACE_ADDRESSES_SRC_LEASE,0))<0)
  {
    printf("[server]Eroare la obtinerea interfetelor\n");
    vm_d->interface_nr=-1;
  }
  else
  {
    vm_d->interface_nr=count_if;
    for(int i=0;i<count_if;i++)
  {
    strcpy(vm_d->interface_list[i],ifaces[i]->name);
    ///ip address for interface
    _virDomainInterfaceIPAddress *ips=ifaces[i]->addrs;
  
    if(ips->type==VIR_IP_ADDR_TYPE_IPV4)
      sprintf(vm_d->ip_address[i],"%s", ips->addr);
    else
      sprintf(vm_d->ip_address[i],"NULL");
  }
  }
  for (int i = 0; i < count_if; i++)
            virDomainInterfaceFree(ifaces[i]);
  free(ifaces);

  return vm_d; 
}


int parabol(char Rez[1024])
{

  std::vector<char*> resultArray  ;
  sqlite3* db;
  int rc;
  rc = sqlite3_open("saves.db", &db);  
  if (rc) {
      // Error handling if the database couldn't be opened
      std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
      exit(EXIT_FAILURE);
  }
  const char* stmt = "SELECT id_save, date_save FROM saves_vm;";

  // Execute the SQL statement and invoke selectCallback for each row
  rc = sqlite3_exec(db, stmt, parabolCallBack, &resultArray, 0); 
  if (rc != SQLITE_OK) 
  {
    printf("SQL error: %s\n",sqlite3_errmsg(db));
    exit(EXIT_FAILURE);
  }
  
  // Close the database
  sqlite3_close(db);

  bzero(Rez,1024*sizeof(char));strcpy(Rez,"");
  for(int i=0;i<resultArray.size();i++)
  {
    strcat(Rez,resultArray[i]);
    strcat(Rez,"\n");
  }

  return 1;
};



int list_vms(char rez[25][1024])
{
  virConnectPtr con=virConnectOpen("qemu:///system");
  
  if(con==NULL)
  { 
    printf("[server]Failed to connect to hypervisor\n"); 
    return -1;
  }

  int nr_vms=virConnectNumOfDomains(con);
  if(nr_vms>0)
  {
    int *vmIDs=new int[nr_vms];

    if(virConnectListDomains(con,vmIDs,nr_vms)<0)
    {
      printf("[server]Failed to get list of IDs\n");
      return -1;
    }

    for(int i=0;i<nr_vms;i++)
    {
      printf("%d\n",vmIDs[i]);
      sprintf(rez[i],"%d",vmIDs[i]);
    }
    free(vmIDs);
  }
  virConnectClose(con);
  return nr_vms;
}



std::vector<Tree_vms*>* hexagram()
{
  virConnectPtr con=virConnectOpen("qemu:///system");///Stabilim conexiunea la hyperviser
  if(con==nullptr)
  {
    printf("Failed to connect to hypervisor\n");
    exit(EXIT_FAILURE);
  }

  int nr_vms=virConnectNumOfDomains(con);///aflam nr de vm 

  int *ListdomainID=new int[nr_vms];
  nr_vms=virConnectListDomains(con,ListdomainID,nr_vms);////primim o lista cu vm urile pornite
  if(nr_vms<0)
  {
    printf("[server]No virtual machine is on\n");
  }
  
  ///parcurgem fiecare vm
  std::vector<vm_info*>list_vm_info;
  
  for(int i=0;i<nr_vms;i++)
  {
    virDomainPtr vmp=virDomainLookupByID(con,ListdomainID[i]);
    list_vm_info.push_back(vm_data_make(vmp));
    virDomainFree(vmp);
  }
  virConnectClose(con);

  saveVMS(list_vm_info);
  
  std::vector<Tree_vms*>*VMconnections =new std::vector<Tree_vms*>();
  std::vector<int>marked_list;
  for(int i=0;i<list_vm_info.size();i++)
  {
    marked_list.push_back(0);
  }

  for(int i=0;i<list_vm_info.size();i++)
  {
    if(marked_list[i]==0)
    {
      VMconnections->push_back(makeGraph(*list_vm_info[i],list_vm_info,marked_list));
    }
  }
  return VMconnections;
};
Tree_vms* makeGraph(vm_info &current_vm,std::vector<vm_info*>&list_vm_info,std::vector<int>&marked_list)
{
  Tree_vms *node;
  node=new Tree_vms();
  strcpy(node->name,current_vm.name);
  int i=0;
  while(strcmp(list_vm_info[i]->name,current_vm.name)!=0)
    i++;
  marked_list[i]=1;
  node->connections=new std::vector<Tree_vms*>();
  for(int i=0;i<list_vm_info.size();i++)
  {
    if(marked_list[i]==0 && vm_con(*list_vm_info[i],current_vm)==1)
    {
      node->connections->push_back(makeGraph(*list_vm_info[i],list_vm_info,marked_list));
    }
  }
  return node;
}
int vm_con(vm_info vm1,vm_info vm2)
{
  char ip_vm1[256];strcpy(ip_vm1,vm1.ip_address[0]);
  char ip_vm2[256];strcpy(ip_vm2,vm2.ip_address[0]);

  int index=0;
  int nr_points=0;
  while(nr_points<3)
  {
    if(ip_vm1[index]!=ip_vm2[index])
      return 0;
    if(ip_vm1[index]=='.')
      nr_points++;
    index++;
  }
  return 1;
};

int sendTreeList(int fd,std::vector<Tree_vms*>*list)
{
  int lsize=list->size();
  if(write(fd,&lsize,sizeof(int))<=0)
  {
    perror("[server_child]Error at write\n");
    return -1;
  }
  for(int i=0;i<list->size();i++)
  {
    if(sendTree(fd,list->at(i))==-1)
      return -1;
  }
  return 1;
};

int sendTree(int fd,Tree_vms*tree)
{
  char name[1024];int name_size;
  bzero(name,1024*sizeof(char));bzero(&name_size,sizeof(int));
  strcpy(name,tree->name);
  name_size=strlen(name)+1;

  if(write(fd,&name_size,sizeof(int))<=0)
  {
    return -1;
  }
  if(write(fd,name,name_size)<=0)
  {
    return -1;
  }
  
  int size_con;bzero(&size_con,sizeof(int));size_con=tree->connections->size();
  if(write(fd,&size_con,sizeof(int))<=0)
  {
    return -1;
  }
  
  for(int i=0;i<tree->connections->size();i++)
  {
    sendTree(fd,tree->connections->at(i));
  }
  return 1;
}

void saveVMS(std::vector<vm_info*> list_vm_info) 
{
    char* errMsg = nullptr;
    sqlite3* db;
    int rc;
    rc = sqlite3_open("saves.db", &db);

    if (rc) {
        // Error handling if the database couldn't be opened
        printf("Can't open database: %s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }

    std::time_t now = std::time(nullptr);
    std::tm date_save = *std::localtime(&now);
    std::stringstream ss;
    ss << std::put_time(&date_save, "%Y-%m-%d %H:%M:%S");
    std::string formattedDateTime = ss.str();

    // Construct the SQL statement with a parameter
    char stmt[1024];
    sprintf(stmt,"INSERT INTO saves_vm (date_save) VALUES ('%s');",formattedDateTime.c_str());
    rc = sqlite3_exec(db, stmt, 0, 0, &errMsg);

    if (rc != SQLITE_OK) {
        printf("Error inserting save: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
    ///get id of the save
    int lastInsertId = static_cast<int>(sqlite3_last_insert_rowid(db));
    
    ///insert vms
    for(int i=0;i<list_vm_info.size();i++)
    {
      bzero(stmt,1024*sizeof(char));
      sprintf(stmt,"INSERT INTO vm_list (id_save,name,CPU_number,CPU_time,RAM,state,interface_nr,load) VALUES (%d,'%s','%s','%s','%s','%s',%d,'%s');",
      lastInsertId,list_vm_info[i]->name,list_vm_info[i]->CPU_number,list_vm_info[i]->CPU_time,list_vm_info[i]->RAM,list_vm_info[i]->state,list_vm_info[i]->interface_nr,list_vm_info[i]->load);
      rc = sqlite3_exec(db, stmt, 0, 0, &errMsg);
      if (rc != SQLITE_OK) 
      {
        printf("Error inserting save: %s\n", errMsg);
        sqlite3_free(errMsg);
      }      
      ///interfaces
      for(int j=0;j<list_vm_info[i]->interface_nr;j++)
      {
        bzero(stmt,1024*sizeof(char));
        sprintf(stmt,"INSERT INTO vm_interface (id_save,name,interface,IP) VALUES (%d,'%s','%s','%s');",lastInsertId,list_vm_info[i]->name,list_vm_info[i]->interface_list[j],list_vm_info[i]->ip_address[j]);
        rc=sqlite3_exec(db,stmt,0,0,&errMsg);
        if(rc!=SQLITE_OK)
        {
          printf("Error inserting save: %s\n", errMsg);
          sqlite3_free(errMsg);
        }
      }
    }
    
    // Close the database
    sqlite3_close(db);
};


int parabolCallBack(void* data, int argc, char** argv, char** azColName)
{
  std::vector<char*>*rezArray=static_cast<std::vector<char*>*>(data);
  char* cop = static_cast<char*>(malloc(strlen(argv[0]) + strlen(argv[1]) + 4)); 

  if (cop != nullptr)
  {
    sprintf(cop, "%s : %s", argv[0], argv[1]);
    rezArray->push_back(cop);
  }
  return 0;
}

int parabolaCallBack(void* data, int argc, char** argv, char** azColName)
{
  std::vector<vm_info*>* rezArray=static_cast<std::vector<vm_info*>*>(data);

  vm_info*cop=new vm_info;

  strcpy(cop->name,argv[1]);
  strcpy(cop->CPU_number, argv[2]);
  strcpy(cop->CPU_time, argv[3]);
  strcpy(cop->RAM, argv[4]);
  strcpy(cop->state, argv[5]);
  cop->interface_nr = atoi(argv[6]); // Convert interface_nr to int
  strcpy(cop->load, argv[7]);
  
  rezArray->push_back(cop);
  return 0;
}

std::vector<Tree_vms*>* parabola(int id_save)
{
  sqlite3 *db;
  int rc;
  rc=sqlite3_open("saves.db",&db);
  if(rc)
  {
    printf("Can't open database: %s\n",sqlite3_errmsg(db));
    exit(EXIT_FAILURE);
  }

  std::vector<vm_info*>list_vm_info;

  char stmt[1024];

  sprintf(stmt,"SELECT * FROM vm_list WHERE id_save = %d ;",id_save);

  rc=sqlite3_exec(db,stmt,parabolaCallBack,&list_vm_info,0);

  if (rc != SQLITE_OK) 
  {
    printf("SQL error: %s\n",sqlite3_errmsg(db));
    exit(EXIT_FAILURE);
  }

  for(int i=0;i<list_vm_info.size();i++)
  {
    if(list_vm_info[i]->interface_nr>0)
    {
      bzero(stmt,1024);
      sprintf(stmt,"SELECT * FROM vm_interface WHERE id_save = %d AND name LIKE '%s';",id_save,list_vm_info[i]->name);
      std::vector<std::pair<char*,char*>>if_ip;
      rc=sqlite3_exec(db,stmt,intf_parabolaCallBack,&if_ip,0);
      if (rc != SQLITE_OK) 
      {
        printf("SQL error: %s\n",sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
      }
      for(int j=0;j<if_ip.size();j++)
      {
        strcpy(list_vm_info[i]->interface_list[j],if_ip[j].first);
        strcpy(list_vm_info[i]->ip_address[j],if_ip[j].second);
      }
    }
  }

  sqlite3_close(db);

  std::vector<Tree_vms*>*VMconnections =new std::vector<Tree_vms*>();
  std::vector<int>marked_list;
  for(int i=0;i<list_vm_info.size();i++)
  {
    marked_list.push_back(0);
  }

  for(int i=0;i<list_vm_info.size();i++)
  {
    if(marked_list[i]==0)
    {
      VMconnections->push_back(makeGraph(*list_vm_info[i],list_vm_info,marked_list));
    }
  }
  return VMconnections;
};

int intf_parabolaCallBack(void* data, int argc, char** argv, char** azColName)
{
  std::vector<std::pair<char*,char*>>*rez_pair=static_cast<std::vector<std::pair<char*,char*>>*>(data);
  std::pair<char*,char*>*cop=new std::pair<char*,char*>;
 
  cop->first = new char[strlen(argv[2]) + 1];
  strcpy(cop->first, argv[2]);

  cop->second = new char[strlen(argv[3]) + 1];
  strcpy(cop->second, argv[3]);
  
  rez_pair->push_back(*cop);
  return 0;
};