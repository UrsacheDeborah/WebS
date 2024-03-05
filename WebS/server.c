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


#define PAGE_DIRECTORY ""
/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
  int autentificat;
}thData;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);

int main (int argc, char *argv[])
{
	if (argc != 2) {
			 fprintf(stderr, "Usage: %s <port_number>\n", argv[0]);
			 exit(EXIT_FAILURE);
	 }

	 int PORT = atoi(argv[1]);
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;
  char buf[1024];		//mesajul primit de trimis la client
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
      if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
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
  td->autentificat=0;
	pthread_create(&th[i], NULL, &treat, td);

	}//while
};

void handle_login(int client_socket, char *cl_username, char *cl_passwd, thData *td) {
    char user[32], rand[100];
    int okk = 0;
    memset(user, '\0', strlen(user));

    strcat(user, cl_username);
    if (user[strlen(user) - 1] == '\n')
        user[strlen(user) - 1] = '\0';
    strcat(user, " ");
    strcat(user, cl_passwd);
    if (user[strlen(user) - 1] == '\n')
        user[strlen(user) - 1] = '\0';

    FILE *fin = fopen("cont.txt", "r");

    while (fgets(rand, 512, fin) != NULL) {
        if ((strstr(rand, user)) != NULL) {
            okk++;
            break;
        }
    }

    if (okk != 0) {
        char message[] = "User si parola sunt corecte!\n";
        if (write(client_socket, message, sizeof(message)) <= 0) {
            printf("[Thread]Eroare la write() catre client.\n");
        } else {
            printf("Mesajul a fost transmis cu succes.\n");
        }
        td->autentificat = 1;
    } else {
        char message[] = "Autentificare esuata!\n";
        if (write(client_socket, message, sizeof(message)) <= 0) {
            printf("[Thread]Eroare la write() catre client.\n");
        } else {
            printf("Mesajul a fost trasmis cu succes.\n");
        }
        td->autentificat = 0;
    }

    fclose(fin);
}


void handle_list(int client_socket) {
  char file_list[1024];
  memset(file_list, 0, sizeof(file_list));

  FILE *fp;
  char command[50];

  snprintf(command, sizeof(command), "ls %s", PAGE_DIRECTORY);

  fp = popen(command, "r");
  fread(file_list, 1, sizeof(file_list)-1, fp);
  pclose(fp);

  //send(client_socket, file_list, strlen(file_list), 0);
  if (write (client_socket, &file_list, sizeof(file_list)) <= 0)
   {
    printf("[Thread %d] ");
    perror ("[Thread]Eroare la write() catre client.\n");
   }
 else
   printf ("[Thread %d]Mesajul a fost transmis cu succes.\n");
}


void handle_get(int client_socket, char *page_name) {
    char file_path[256];
    FILE *file = fopen(page_name, "rb");

    if (file != NULL) {
			//merge la final pt a vedea dimensiunea
        fseek(file, 0, SEEK_END);
				//dimensiunea fisierului in octeti
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *file_content = malloc(file_size + 1); // +1 pentru caracterul null terminator
        fread(file_content, 1, file_size, file);
        file_content[file_size] = '\0\n'; // Adăugați terminatorul null pentru a asigura o șir de caractere corect.

        fclose(file);

        // Trimite conținutul paginii către client
        if (write(client_socket, file_content, file_size + 1) <= 0) {
            perror("[Thread]Eroare la write() catre client.\n");
        } else {
            printf("[Thread]Mesajul a fost transmis cu succes.\n");
        }

        free(file_content);
    } else {
        char *message = "Pagina nu a fost gasita!\n";
        if (write(client_socket, message, strlen(message) + 1) <= 0) {
            perror("[Thread]Eroare la write() catre client.\n");
        } else {
            printf("[Thread]Mesajul a fost transmis cu succes.\n");
        }
    }
}


void handle_add(int client_socket, char *page_name, char *page_content) {
    char file_path[256];
    FILE *file = fopen(page_name, "wb"); // Deschide fișierul în mod binar

    if (file != NULL) {
        fwrite(page_content, 1, strlen(page_content), file);
        fclose(file);

        char *message = "Pagina a fost adaugata cu succes!\n";
        if (write(client_socket, message, strlen(message) + 1) <= 0) {
            perror("[Thread]Eroare la write() catre client.\n");
        } else {
            printf("[Thread]Mesajul a fost transmis cu succes.\n");
        }
    } else {
        char *message = "Eroare la adaugarea paginii!\n";
        if (write(client_socket, message, strlen(message) + 1) <= 0) {
            perror("[Thread]Eroare la write() catre client.\n");
        } else {
            printf("[Thread]Mesajul a fost transmis cu succes.\n");
        }
    }
}

void handle_delete(int client_socket, char *page_name) {
    char file_path[256];

    if (remove(page_name) == 0) {
        char *message = "Pagina a fost stearsa cu succes!\n";
        if (write(client_socket, message, strlen(message) + 1) <= 0) {
            perror("[Thread]Eroare la write() catre client.\n");
        } else {
            printf("[Thread]Mesajul a fost transmis cu succes.\n");
        }
    } else {
        char *message = "Pagina nu a fost gasita!";
        if (write(client_socket, message, strlen(message) + 1) <= 0) {
            perror("[Thread]Eroare la write() catre client.\n");
        } else {
            printf("[Thread]Mesajul a fost transmis cu succes.\n");
        }
    }
}


static void *treat(void * arg)
{
		struct thData tdL;
		tdL= *((struct thData*)arg);
		printf ("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
		fflush (stdout);
		pthread_detach(pthread_self());
		raspunde((struct thData*)arg);
		/* am terminat cu acest client, inchidem conexiunea */
		//close ((intptr_t)arg);
		return(NULL);

};



void raspunde(void *arg) {
    char command[1024];
    struct thData tdL;
    tdL = *((struct thData*)arg);

    while (1) {
        if (read(tdL.cl, command, sizeof(command)) <= 0) {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
            break;
        }

        printf("[Thread %d] Mesajul a fost receptionat...%s\n", tdL.idThread, command);

        if (strncmp(command, "exit", 4) == 0) {
            // inchide bucla, dar nu inchide conexiunea
            break;
        }

        if (strncmp(command, "login", 5) != 0 && tdL.autentificat != 1) {
            char message[] = "Authentication required. Please login first.\n";
            if (write(tdL.cl, message, strlen(message)) <= 0) {
                printf("[Thread] Eroare la write() catre client.\n");
            } else {
                printf("Mesajul a fost transmis cu succes.\n");
            }
        } else {
            if (strncmp(command, "login", 5) == 0) {
                char username[256];
                char parola[256];
                sscanf(command, "login %s %s", username, parola);
                handle_login(tdL.cl, username, parola, &tdL);
                if (tdL.autentificat == 1) {
                    printf("Autentificare cu succes!\n");
                }
								else {
                    printf("Autentificare esuata!\n");}
                //     char message[] = "Autentificare esuata!\n";
                //     if (write(tdL.cl, message, strlen(message)) <= 0) {
                //         printf("[Thread] Eroare la write() catre client.\n");
                //     } else {
                //         printf("Mesajul a fost transmis cu succes.\n");
                //     }
                //     // break;
                // }
            } else if (strcmp(command, "list") == 0) {
                handle_list(tdL.cl);
            } else if (strncmp(command, "get", 3) == 0) {
                char page_name[256];
                sscanf(command, "get %s", page_name);
                handle_get(tdL.cl, page_name);
            } else if (strncmp(command, "add", 3) == 0) {
                char page_name[256];
                char page_content[1024];
                sscanf(command, "add %s %[^\n]s", page_name, page_content);
                handle_add(tdL.cl, page_name, page_content);
            } else if (strncmp(command, "delete", 6) == 0) {
                char page_name[256];
                sscanf(command, "delete %s", page_name);
                handle_delete(tdL.cl, page_name);
            } else {
                perror("Eroare la tot.\n");
            }
        }
    }


}
