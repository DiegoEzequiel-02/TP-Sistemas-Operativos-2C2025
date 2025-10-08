#include "servidor.h"
#include "transaccion.h"
#include "csv.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTES 100
#define MAX_EN_ESPERA 10
#define MAX_BUFFER 1024

extern pthread_mutex_t mutex_transaccion;
extern int transaccion_activa;
extern int cliente_transaccion_fd;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

extern pthread_mutex_t mutex_transaccion;
extern int transaccion_activa;
extern int cliente_transaccion_fd;

#define MAX_BUFFER 1024

void* manejar_cliente(void* arg) {
    int cliente_fd = *((int*)arg);
    free(arg);

    char buffer[MAX_BUFFER];
    ssize_t bytes_leidos;

    int en_transaccion = 0;

    while (1) {
        bytes_leidos = recv(cliente_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_leidos <= 0) {
            // Cliente desconectado o error
            if (en_transaccion) {
                // Liberar lock si cliente tenía transacción activa
                pthread_mutex_unlock(&mutex_transaccion);
                transaccion_activa = 0;
                cliente_transaccion_fd = -1;
            }
            close(cliente_fd);
            printf("Cliente desconectado (fd=%d)\n", cliente_fd);
            return NULL;
        }

        buffer[bytes_leidos] = '\0';

        // Eliminar salto de línea si existe
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';

        printf("Cliente(fd=%d) dijo: %s\n", cliente_fd, buffer);

        // Procesar comandos básicos
        if (strcasecmp(buffer, "BEGIN TRANSACTION") == 0) {
            if (transaccion_activa) {
                char *msg = "ERROR: Transacción ya activa por otro cliente. Intente luego.\n";
                send(cliente_fd, msg, strlen(msg), 0);
            } else {
                pthread_mutex_lock(&mutex_transaccion);
                transaccion_activa = 1;
                cliente_transaccion_fd = cliente_fd;
                en_transaccion = 1;
                char *msg = "OK: Transacción iniciada.\n";
                send(cliente_fd, msg, strlen(msg), 0);
            }
        }
        else if (strcasecmp(buffer, "COMMIT TRANSACTION") == 0) {
            if (!en_transaccion) {
                char *msg = "ERROR: No hay transacción activa para confirmar.\n";
                send(cliente_fd, msg, strlen(msg), 0);
            } else {
                // Aquí iría lógica para confirmar cambios (persistencia en CSV)
                pthread_mutex_unlock(&mutex_transaccion);
                transaccion_activa = 0;
                cliente_transaccion_fd = -1;
                en_transaccion = 0;
                char *msg = "OK: Transacción confirmada.\n";
                send(cliente_fd, msg, strlen(msg), 0);
            }
        }
        else if (strcasecmp(buffer, "EXIT") == 0) {
            if (en_transaccion) {
                pthread_mutex_unlock(&mutex_transaccion);
                transaccion_activa = 0;
                cliente_transaccion_fd = -1;
                en_transaccion = 0;
            }
            char *msg = "OK: Desconectando.\n";
            send(cliente_fd, msg, strlen(msg), 0);
            close(cliente_fd);
            printf("Cliente(fd=%d) se desconectó voluntariamente.\n", cliente_fd);
            return NULL;
        }
        else {
            // Aquí podés agregar más comandos (consultas, modificaciones, etc.)
            char *msg = "ERROR: Comando no reconocido.\n";
            send(cliente_fd, msg, strlen(msg), 0);
        }
    }
}


void configurar_servidor(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <IP> <PUERTO>\n", argv[0]);
        exit(1);
    }

    char* ip = argv[1];
    int puerto = atoi(argv[2]);

    int servidor_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor_fd == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in servidor_addr;
    servidor_addr.sin_family = AF_INET;
    servidor_addr.sin_port = htons(puerto);
    servidor_addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(servidor_fd, (struct sockaddr*)&servidor_addr, sizeof(servidor_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(servidor_fd, MAX_EN_ESPERA) < 0) {
        perror("listen");
        exit(1);
    }

    log_info("Servidor escuchando en %s:%d", ip, puerto);

    while (1) {
        struct sockaddr_in cliente_addr;
        socklen_t len = sizeof(cliente_addr);
        int cliente_fd = accept(servidor_fd, (struct sockaddr*)&cliente_addr, &len);
        if (cliente_fd < 0) {
            perror("accept");
            continue;
        }

        log_info("Nuevo cliente conectado: %s", inet_ntoa(cliente_addr.sin_addr));

        pthread_t hilo;
        int* nuevo_fd = malloc(sizeof(int));
        *nuevo_fd = cliente_fd;
        pthread_create(&hilo, NULL, manejar_cliente, nuevo_fd);
        pthread_detach(hilo);
    }
}
