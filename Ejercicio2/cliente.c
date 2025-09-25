#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER 512

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Uso: %s <IP servidor> <PUERTO>\n", argv[0]);
        return 1;
    }

    char* ip = argv[1];
    int puerto = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in servidor;
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(puerto);
    servidor.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr*)&servidor, sizeof(servidor)) < 0) {
        perror("connect");
        close(sock);
        exit(1);
    }

    printf("Conectado al servidor %s:%d\n", ip, puerto);

    char buffer[BUFFER];
    while (1) {
        printf("Ingrese comando (o SALIR): ");
        if (!fgets(buffer, BUFFER, stdin)) break;
        buffer[strcspn(buffer, "\n")] = 0;  // quitar \n

        if (strlen(buffer) == 0) continue;

        send(sock, buffer, strlen(buffer), 0);

        if (strcmp(buffer, "SALIR") == 0) break;

        memset(buffer, 0, BUFFER);
        int n = recv(sock, buffer, BUFFER, 0);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Respuesta del servidor: %s\n", buffer);
        } else {
            printf("Servidor desconectado.\n");
            break;
        }
    }

    close(sock);
    return 0;
}
