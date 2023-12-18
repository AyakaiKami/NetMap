/* cliTCPIt.c - Exemplu de client TCP
   Trimite un numar la server; primeste de la server numarul incrementat.
         
   Autor: Lenuta Alboaie  <adria@info.uaic.ro> (c)
*/
#include <math.h>
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
#include <vector>
#include <SFML/Graphics.hpp>
#include <math.h>
#include <algorithm>
#include <iostream>
/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

struct Tree_vms
{
  char name[256];
  std::vector<Tree_vms*>*connections;
};

std::vector<Tree_vms*>*getTreeList(int fd);
Tree_vms*getTree(int fd);

void burnList(std::vector<Tree_vms*>*list);
void burnTree(Tree_vms* tree);

//void GraphDraw(sf::RenderWindow &window,Tree_vms* tree,int sx,int fx,int height);
void GraphDrawList(sf::RenderWindow &window,std::vector<Tree_vms*>*con,int x);
void GraphDraw(sf::RenderWindow& window, Tree_vms* tree, float x, float y, float xOffset, float levelOffset);

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
  /*=================================================================*/
  /*                              CHILD                              */
  if(pid_window==0)///child
  {
    close(wpipe[1]);////child cant write with this pipe
    close(rpipe[0]);////child cant read with this pipe
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
    printf("[client_child]Connecting on port %d\n",port_c);
    sleep(1);
    if (connect (sd_child, (struct sockaddr *) &server_child,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client_child]Eroare la connect().\n");
      return errno;
    }
    printf("[client_child]Connected\n");
    /*==========================================================================*/
    /*                                 Child Connected                          */
    int on=1;int hon=0;
    int size_msg_server_child;
    char msg_server_child[1024];
    sleep(1);
    while (on)
    {
      ///clear vars
      bzero(&size_msg_server_child,sizeof(int));
      bzero(msg_server_child,1024*sizeof(char));

      ///get data from parrent
      if(read(wpipe[0],&size_msg_server_child,sizeof(int))<=0)
      {
        perror("[client_child]Error read\n");
      }
      if(read(wpipe[0],msg_server_child,size_msg_server_child)<=0)
      {
        perror("[client_child]Error read\n");
      }     
      ///printf("[client_child]Got %s of size %d from parrent\n",msg_server_child,size_msg_server_child);
      //send data to server_child
      sleep(1);
      if(write(sd_child,&size_msg_server_child,sizeof(int))<=0)
      {
        perror("[client_child]Error write\n");
      }
      if(write(sd_child,msg_server_child,size_msg_server_child)<=0)
      {
        perror("[client_child]Error write\n");
      }
      //printf("[client_child]Sent %s of size %d to server_child\n",msg_server_child,size_msg_server_child);
      sleep(1);
      if(strcmp(msg_server_child,"close hexagram")==0)
      {
        printf("Hexagram not open\n");
      }
      if(strcmp(msg_server_child,"close parabola")==0)
      {
        printf("Parabola not open\n");
      }

      if(strcmp(msg_server_child,"close")==0)
      {
        on=0;
        continue;
      }
      if(strcmp(msg_server_child,"hexagram")==0)
      {
        printf("Opening Hexagram\n");
        int hon=1;
        std::vector<Tree_vms*>*list=getTreeList(sd_child);
        sf::RenderWindow *window=new sf::RenderWindow(sf::VideoMode(1200,800),"Hexagram");
        window->clear();
        GraphDrawList(*window,list,1200);
        window->display();
        while (hon && window->isOpen())
        {
          sf::Event event;
          while (window->pollEvent(event) ) 
          {
              if (event.type == sf::Event::Closed) {
                  window->close();
              }
          }
          
          bzero(&size_msg_server_child,sizeof(int));
          bzero(msg_server_child,1024*sizeof(char));        
          if(read(wpipe[0],&size_msg_server_child,sizeof(int))<0 )
          {
            perror("[client_child]Error at read\n");
          }
          if(read(wpipe[0],msg_server_child,size_msg_server_child)<0)
          {
            perror("[client_child]Error at read\n");
          }
            
            printf("Child Got: %s of %d\n",msg_server_child,size_msg_server_child);
            if(strcmp(msg_server_child,"close hexagram")==0)
            {
              printf("[server_child]Closing hexagram\n");
              hon=0;
              window->close();
              free(window);
              bzero(&size_msg_server_child,sizeof(int));
              bzero(msg_server_child,1024*sizeof(char));
              strcpy(msg_server_child,"close hexagram");size_msg_server_child=strlen(msg_server_child)+1;

              if(write(sd_child,&size_msg_server_child,sizeof(int))<=0)
              {
                perror("[client_child]Error at write\n");
              }
              if(write(sd_child,msg_server_child,size_msg_server_child)<=0)
              {
                perror("[client_child]Error at write\n");
              }
              continue;
            }
            else
            {///default msg to server_child
              bzero(&size_msg_server_child,sizeof(int));
              bzero(msg_server_child,1024*sizeof(char));
              strcpy(msg_server_child,"none");size_msg_server_child=strlen(msg_server_child)+1;

              if(write(sd_child,&size_msg_server_child,sizeof(int))<=0)
              {
                perror("[client_child]Error at write\n");
              }
              if(write(sd_child,msg_server_child,size_msg_server_child)<=0)
              {
                perror("[client_child]Error at write\n");
              }

            }

          
          bzero(&size_msg_server_child,sizeof(int));
          bzero(msg_server_child,1024*sizeof(char));
          if(read(sd_child,&size_msg_server_child,sizeof(int))<0)
          {perror("[client_child]Error at read\n");} 
          
          if(read(sd_child,msg_server_child,size_msg_server_child)<0)
          {perror("[client_child]Error at read\n");}
          
          printf("Child Got from server: %s of %d\n",msg_server_child,size_msg_server_child);          
          if(strcmp(msg_server_child,"new list")==0)
          {
            printf("[client_child]New list\n");
            burnList(list);
            list=getTreeList(sd_child);///getting the new list
            window->clear();///draw
            //window->close();
            GraphDrawList(*window,list,1200);
            window->display();
            continue;
          }         
                       


          ///default msg to server_child
         /* bzero(&size_msg_server_child,sizeof(int));
          bzero(msg_server_child,1024*sizeof(char));        
          strcpy(msg_server_child,"none");size_msg_server_child=strlen(msg_server_child)+1;

          if(write(sd_child,&size_msg_server_child,sizeof(int))<=0)
          {
            perror("[client_child]Error at write\n");
          }
          if(write(sd_child,msg_server_child,size_msg_server_child)<=0)
          {
            perror("[client_child]Error at write\n");
          }*/
        }

      } 

      if  (strcmp(msg_server_child,"parabola")==0)
      {
        printf("Opening Parabola\n");
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
  int hon=0;
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
    ///to server
    sleep(1);
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
    ///printf("[client]Sending %s of size %d\n",msg_send,size_msg_send);
    
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

    if(hon==1)///default msg to child
    {
      char msg_child[1024];bzero(msg_child,1024*sizeof(char));
      int size_msg_child;bzero(&size_msg_child,sizeof(int));

      strcpy(msg_child,"none");size_msg_child=strlen(msg_child)+1;

      if(write(wpipe[1],&size_msg_child,sizeof(int))<=0)
      {
        perror("[client]Error write\n");
      }  
      if(write(wpipe[1],msg_child,size_msg_child)<=0)
      {
        perror("[client]Error write\n");
      }  
    }
    //printf("[client]Got %s of size %d\n",msg_recive,size_msg_recive);
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
      hon=0;
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
      hon=1;
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
      continue;
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
      continue;
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
      continue;
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
      bzero(&size_msg_to_child,sizeof(int));
      bzero(msg_to_child,1024*sizeof(char));

      strcpy(msg_to_child,"close");
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
  return 1;
};


std::vector<Tree_vms*>*getTreeList(int fd)
{
  std::vector<Tree_vms*>*listTrees=new std::vector<Tree_vms*>();
  int size_list;bzero(&size_list,sizeof(int));
  if(read(fd,&size_list,sizeof(int))<0)
  {
    perror("[client_child]Error a read\n");
  }
  
  for(int i=0;i<size_list;i++)
  {
    listTrees->push_back(getTree(fd));
  }
  return listTrees;
};
Tree_vms*getTree(int fd)
{
  Tree_vms *tree=new Tree_vms();

  char name[1024];bzero(name,1024*sizeof(char));
  int size_name;bzero(&size_name,sizeof(int));

  if(read(fd,&size_name,sizeof(int))<0)
  {
    perror("[client_child]Error at read\n");
  }
  if(read(fd,name,size_name)<0)
  {
    perror("[client_child]Error at read\n");
  }  

  strcpy(tree->name,name);

  int size_con;bzero(&size_con,sizeof(int));
  if(read(fd,&size_con,sizeof(int))<0)
  {
    perror("[client_child]Error at read\n");
  }
  tree->connections=new std::vector<Tree_vms*>();
  for(int i=0;i<size_con;i++)
  {
    tree->connections->push_back(getTree(fd));
  }
  return tree;
}

void GraphDraw(sf::RenderWindow& window, Tree_vms* tree, float x, float y, float xOffset, float levelOffset) 
{
    if (tree == nullptr) {
        return;
    }
    printf("Name : %s\n",tree->name);
    // Draw the circle
    sf::CircleShape circle(40);
    circle.setPosition(x - circle.getRadius(), y - circle.getRadius());
    circle.setFillColor(sf::Color::White);
    circle.setOutlineThickness(2);
    circle.setOutlineColor(sf::Color::Black);
    window.draw(circle);

    // Draw the text inside the circle
    sf::Font font;
    if (!font.loadFromFile("Basic-Regular.ttf")) {
        std::cerr << "Failed to load font\n";
        return;
    }

    sf::Text text(tree->name, font, 12);
    sf::FloatRect textBounds = text.getLocalBounds();
    text.setPosition(x - textBounds.width / 2.0f, y - textBounds.height / 2.0f);
    text.setFillColor(sf::Color::Black);
    window.draw(text);

    // Draw lines to connections
    float sp = xOffset / std::max(1, static_cast<int>(tree->connections->size()));
    for (size_t i = 0; i < tree->connections->size(); i++) {
        float xConnection = x - xOffset + sp * i + sp / 2.0f;
        float yConnection = y + levelOffset;
        sf::Vertex line[] = {sf::Vertex(sf::Vector2f(x, y + circle.getRadius())), sf::Vertex(sf::Vector2f(xConnection, yConnection))};
        window.draw(line, 2, sf::Lines);

        GraphDraw(window, tree->connections->at(i), xConnection, yConnection, xOffset * 0.5f, levelOffset * 1.5f);
    }
};

void GraphDrawList(sf::RenderWindow &window,std::vector<Tree_vms*>*con,int x) 
{
  int sp=0;
    if(con->size()>0)
    {
      sp=x/(con->size()+1);
      for (int i = 0; i < con->size(); i++) 
      {
        GraphDraw(window, con->at(i), sp * (i+1),50,200,100);
      }
    }
};

void burnList(std::vector<Tree_vms*>*list)
{
  for(int i=0;i<list->size();i++)
  {
    burnTree(list->at(i));
  }
  
};
void burnTree(Tree_vms* tree)
{
  for(int i=0;i<tree->connections->size();i++)
    burnTree(tree->connections->at(i));
  delete tree->connections;
  
  delete tree;
};