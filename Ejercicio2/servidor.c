#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "funcion.h"
#define BUFFER 512

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int lock_activo = 0;       // 0 = libre, otro valor = socket con la transacción

void* manejar_cliente(void* arg) {
    int sock = *(int*)arg;
    free(arg);

    char buffer[BUFFER];
    while (1) {
        memset(buffer, 0, BUFFER);
        int n = recv(sock, buffer, BUFFER, 0);
        if (n <= 0) break; // cliente cerró

        buffer[strcspn(buffer, "\n")] = 0; // limpiar \n
        printf("[Cliente %d] %s\n", sock, buffer);

        if (strcmp(buffer, "BEGIN TRANSACTION") == 0) {
            pthread_mutex_lock(&lock);
            if (lock_activo == 0) {
                lock_activo = sock;
                send(sock, "OK: transacción iniciada\n", 26, 0);
            } else {
                send(sock, "ERROR: transacción activa\n", 27, 0);
            }
            pthread_mutex_unlock(&lock);

        } else if (strcmp(buffer, "COMMIT TRANSACTION") == 0) {
            pthread_mutex_lock(&lock);
            if (lock_activo == sock) {
                lock_activo = 0;
                send(sock, "OK: transacción confirmada\n", 28, 0);
            } else {
                send(sock, "ERROR: no tiene transacción\n", 29, 0);
            }
            pthread_mutex_unlock(&lock);

        } else if (strncmp(buffer, "CONSULTA:", 9) == 0)
            procesar_consulta(sock, buffer);
            else if (strncmp(buffer, "ALTA:", 5) == 0)
            procesar_alta(sock, buffer);
            else if (strncmp(buffer, "BAJA:", 5) == 0)
            procesar_baja(sock, buffer);
            else if (strncmp(buffer, "MODIFICAR:", 10) == 0)
            procesar_modificar(sock, buffer);
            else if (strcmp(buffer, "SALIR") == 0) {
                    break;
            } else {
                send(sock, "ERROR: comando inválido\n", 25, 0);
            }
    }

    // cleanup: si se cae un cliente con lock, lo liberamos
    pthread_mutex_lock(&lock);
    if (lock_activo == sock) lock_activo = 0;
    pthread_mutex_unlock(&lock);

    close(sock);
    printf("[Cliente %d desconectado]\n", sock);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Uso: %s <IP> <PUERTO>\n", argv[0]);
        return 1;
    }

    char* ip = argv[1];
    int puerto = atoi(argv[2]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servidor = {0};
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(puerto);
    servidor.sin_addr.s_addr = inet_addr(ip);

    bind(server_fd, (struct sockaddr*)&servidor, sizeof(servidor));
    listen(server_fd, 5);

    printf("Servidor en %s:%d\n", ip, puerto);

    while (1) {
        struct sockaddr_in cliente;
        socklen_t tam = sizeof(cliente);
        int* nuevo_sock = malloc(sizeof(int));
        *nuevo_sock = accept(server_fd, (struct sockaddr*)&cliente, &tam);

        pthread_t hilo;
        pthread_create(&hilo, NULL, manejar_cliente, nuevo_sock);
        pthread_detach(hilo);
    }

    close(server_fd);
    return 0;
}
