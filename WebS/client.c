#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

extern int errno;
int port;

int main(int argc, char *argv[]) {
    int sd;
    struct sockaddr_in server;
    char buf[1024];

    if (argc != 3) {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    port = atoi(argv[2]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Eroare la socket().\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
        perror("[client]Eroare la connect().\n");
        return errno;
    }

    while (1) {
        printf("Comenzi disponibile: login, list, get <page_name>, add <page_name> <page_content>, delete <page_name>\n");
        printf("Introdu comanda (sau scrie 'exit' pentru a iesi): ");
        fflush(stdout);

        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            perror("Eroare la citirea comenzii de la tastatura.\n");
            break;
        }

        buf[strcspn(buf, "\n")] = '\0';

        if (write(sd, buf, strlen(buf) + 1) <= 0) {
            perror("[client]Eroare la write() spre server.\n");
            break;
        }

        if (strcmp(buf, "exit") == 0) {
            // Dacă utilizatorul a introdus "exit", închide bucla ca ssa iasa din program
            break;
        }

        memset(buf, 0, sizeof(buf));

        if (read(sd, buf, sizeof(buf)) <= 0) {
            perror("[client]Eroare la read() de la server.\n");
            break;
        }

        printf("[client]Mesajul primit este: %s", buf);
        if (strcmp(buf, "Autentificare esuata!\n") == 0) {
            break;
        }



    }

    close(sd);
    return 0;
}
