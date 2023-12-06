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
#include <arpa/inet.h>
#include <sstream>
#include <ctime>
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

int getPropFromVM(char Prop[256],char Ident[256],char Rez[25][256]);

double getCPULoad(virDomainPtr vm);

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
    printf("[server]Waitting for input on thread : %d \n",tdL.idThread);
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

      char rez[50][256];
      int lines_rez=getPropFromVM(propriety,ident,rez);

      if(lines_rez==-1)
      {
        printf("[server]Could not get %s from VM %s\n");

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
      printf("[server] Sending %s of size %d\n", msg_send, size_msg_send);

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
        printf("[server] Sending %s of size %d\n", msg_send, size_msg_send);

        if (write(tdL.cl, &size_msg_send, sizeof(int)) < 0) {
            perror("[server] Error at write()\n");
        }
        if (write(tdL.cl, msg_send, size_msg_send) < 0) {
            perror("[server] Error at write()\n");
        }
        bzero(&size_msg_send,sizeof(int));///cleaning send vars
        bzero(msg_send,1024*sizeof(char));
      }

      bzero(&size_msg_send,sizeof(int));
      size_msg_send=-1;
      if(write(tdL.cl,&size_msg_send,sizeof(int))<0)
      {
        perror("[server] Error at write()\n");
      }
      fclose(helpf);
    }
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
    };
    if(word_nr==2)
    {
      strcpy(Ident_rez,token);
    };
  }
  if(word_nr!=3)
    return -1;
  return 1;
};

int getPropFromVM(char Prop[256],char Ident[256],char Rez[50][256])
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

  if(Ident[0]>='0' && Ident[0]<='9')
  ///presupunem ca s-a dat IP-ul deoarece incepe cu o cifra
  {
    int domainID = std::stoi(Ident);
    vm=virDomainLookupByID(con,domainID);///nume sugestiv
  }
  else
  {
    in_addr addr;
    if(inet_pton(AF_INET,Ident,&addr)==1)
    {
      vm=virDomainLookupByUUIDString(con,Ident);///nume sufestiv
    }
      else
      {
        virConnectClose(con);
        printf("[server]Could not find VM\n");
        return -1;
      }
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

///Get name
  if(strcmp(Prop,"name")==0 || strcmp(Prop,"all")==0)
  {
    strcpy(Rez[lines],virDomainGetName(vm));
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