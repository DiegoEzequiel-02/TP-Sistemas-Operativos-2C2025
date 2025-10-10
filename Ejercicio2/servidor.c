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
    cliente_info* info = (cliente_info*)arg;
    int sock = *(int*)arg;
    int id = info->id;
    free(info);

    printf("[Cliente %d conectado]\n", id);
    char buffer[BUFFER];

    while (1) {
        memset(buffer, 0, BUFFER);
        int n = recv(sock, buffer, BUFFER, 0);
        if (n <= 0) break; // cliente cerró

        buffer[strcspn(buffer, "\n")] = 0; // limpiar \n
        printf("[Cliente %d] %s\n", id, buffer);

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
                send(sock, "OK: transacción confirmada\n", 27, 0);
            } else {
                send(sock, "ERROR: no hay transacción activa\n", 34, 0);
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
        // En el bloque LISTAR del servidor
        else if (strcmp(buffer, "LISTAR") == 0) {
            char linea[BUFFER];
            for (int i = 0; i < cantidad_alumnos; i++) {
                snprintf(linea, BUFFER,
                    "ID: %d\n- Nombre: %s\n- Apellido: %s\n- Carrera: %s\n- Materias: %d\n\n",
                    alumnos[i].ID,
                    alumnos[i].nombre,
                    alumnos[i].apellido,
                    alumnos[i].carrera,
                    alumnos[i].materias
                );
                send(sock, linea, strlen(linea), 0);
            }
            if (cantidad_alumnos == 0)
                send(sock, "No hay alumnos registrados.\n", 29, 0);


            // Envía marcador de fin
            send(sock, "__FIN__", strlen("__FIN__"), 0);
        } else if (strcmp(buffer, "SALIR") == 0) {
            break;
        } else {
            send(sock, "ERROR: comando inválido\n", 25, 0);
        }

        if ((strncmp(buffer, "CONSULTA:", 9) == 0 ||
            strncmp(buffer, "ALTA:", 5) == 0 ||
            strncmp(buffer, "BAJA:", 5) == 0 ||
            strncmp(buffer, "MODIFICAR:", 10) == 0 ||
            strcmp(buffer, "LISTAR") == 0) &&
            (lock_activo != 0 && lock_activo != sock)) {
            send(sock, "ERROR: transacción activa, reintente luego\n", 43, 0);
            continue;
        }
    }

    // cleanup: si se cae un cliente con lock, lo liberamos
    pthread_mutex_lock(&lock);
    if (lock_activo == sock) lock_activo = 0;
    pthread_mutex_unlock(&lock);

    close(sock);
    printf("[Cliente %d desconectado]\n", id);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        printf("Uso: %s <IP> <PUERTO> <N_concurrentes> <M_espera>\n", argv[0]);
        return 1;
    }

    leer_alumnos_csv();

    char* ip = argv[1];
    int puerto = atoi(argv[2]);
    int max_concurrentes = atoi(argv[3]);
    int max_espera = atoi(argv[4]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in servidor = {0};
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(puerto);
    servidor.sin_addr.s_addr = inet_addr(ip);

    if (bind(server_fd, (struct sockaddr*)&servidor, sizeof(servidor)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, max_espera) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Servidor en %s:%d\n", ip, puerto);

    int cliente_id = 1;
    while (1) {
        struct sockaddr_in cliente;
        socklen_t tam = sizeof(cliente);

        cliente_info* info = malloc(sizeof(cliente_info));
        info->sock = accept(server_fd, (struct sockaddr*)&cliente, &tam);
        info->id = cliente_id++;

        pthread_t hilo;
        pthread_create(&hilo, NULL, manejar_cliente, info);
        pthread_detach(hilo);
    }

    close(server_fd);
    return 0;
}
