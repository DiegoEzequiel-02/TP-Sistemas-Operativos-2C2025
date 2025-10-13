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
#include <semaphore.h>
#include <signal.h>

#define MAX_BUFFER 1024

extern pthread_mutex_t mutex_transaccion;
extern int transaccion_activa;
extern int cliente_transaccion_fd;

sem_t sem_concurrentes;
int MAX_CONCURRENTES = 0;
int server_fd_global = -1;

void cerrar_servidor(int sig) {
    log_info("Señal %d recibida. Cerrando servidor...", sig);
    if (server_fd_global >= 0) {
        close(server_fd_global);
    }
    sem_destroy(&sem_concurrentes);
    exit(EXIT_SUCCESS);
}

void copiar_registros_a_local(TransaccionCliente* tc) {
    for (int i = 0; i < total_registros; i++) {
        tc->registros_locales[i] = registros[i];
    }
    tc->total_locales = total_registros;
}

int buscar_registro_local(TransaccionCliente* tc, int id) {
    for (int i = 0; i < tc->total_locales; i++) {
        if (tc->registros_locales[i].activo && tc->registros_locales[i].id == id) {
            return i;
        }
    }
    return -1;
}

void finalizar_cliente(TransaccionCliente* tc) {
    if (tc->en_transaccion) {
        cancelar_transaccion(tc->cliente_fd);
    }
    close(tc->cliente_fd);
    sem_post(&sem_concurrentes);
    pthread_exit(NULL);
}

// Declaración si no está en un header
int confirmar_transaccion(int cliente_fd, Registro* registros_locales, int total_locales);

void* manejar_cliente(void* arg) {
    int cliente_fd = *((int*)arg);
    free(arg);

    TransaccionCliente tc;
    tc.cliente_fd = cliente_fd;
    tc.en_transaccion = 0;
    tc.total_locales = 0;

    char buffer[MAX_BUFFER];
    ssize_t bytes_leidos;

    while (1) {
        if (!tc.en_transaccion) {
            const char* menu = "\nMenu:\n1 - Begin transaction\n0 - Salir\nElija opción: ";
            send(cliente_fd, menu, strlen(menu), 0);

            bytes_leidos = recv(cliente_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_leidos <= 0) break;
            buffer[bytes_leidos] = '\0';
            trim_nueva_linea(buffer);

            if (strcmp(buffer, "1") == 0) {
                if (!iniciar_transaccion(cliente_fd)) {
                    const char* msg = "ERROR: Transacción ya activa por otro cliente. Intente luego.\n";
                    send(cliente_fd, msg, strlen(msg), 0);
                } else {
                    copiar_registros_a_local(&tc);
                    tc.en_transaccion = 1;
                    const char* msg = "OK: Transacción iniciada.\n";
                    send(cliente_fd, msg, strlen(msg), 0);
                }
            } else if (strcmp(buffer, "0") == 0) {
                const char* msg = "OK: Desconectando.\n";
                send(cliente_fd, msg, strlen(msg), 0);
                break;
            } else {
                const char* msg = "ERROR: Opción no válida.\n";
                send(cliente_fd, msg, strlen(msg), 0);
            }
        } else {
            const char* menu_tx = "\nMenu Transacción:\n1 - Consultar registro\n2 - Modificar registro\n3 - Eliminar registro\n4 - Commit transaction\n0 - Salir\nElija opción: ";
            send(cliente_fd, menu_tx, strlen(menu_tx), 0);

            bytes_leidos = recv(cliente_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_leidos <= 0) break;
            buffer[bytes_leidos] = '\0';
            trim_nueva_linea(buffer);

            if (strcmp(buffer, "1") == 0) {
                const char* pedir_id = "Ingrese ID a consultar: ";
                send(cliente_fd, pedir_id, strlen(pedir_id), 0);
                bytes_leidos = recv(cliente_fd, buffer, sizeof(buffer) -1, 0);
                if (bytes_leidos <= 0) break;
                buffer[bytes_leidos] = '\0';

                if (!es_numero_valido(buffer)) {
                    const char* msg = "ID inválido.\n";
                    send(cliente_fd, msg, strlen(msg), 0);
                    continue;
                }

                int id = atoi(buffer);
                char resp[MAX_LINEA];
                consultar_registro(id, resp, sizeof(resp));
                send(cliente_fd, resp, strlen(resp), 0);

            } else if (strcmp(buffer, "2") == 0) {
                const char* pedir_id = "Ingrese ID a modificar: ";
                send(cliente_fd, pedir_id, strlen(pedir_id), 0);
                bytes_leidos = recv(cliente_fd, buffer, sizeof(buffer) -1, 0);
                if (bytes_leidos <= 0) break;
                buffer[bytes_leidos] = '\0';

                if (!es_numero_valido(buffer)) {
                    const char* msg = "ID inválido.\n";
                    send(cliente_fd, msg, strlen(msg), 0);
                    continue;
                }

                int id = atoi(buffer);
                int idx = buscar_registro_local(&tc, id);
                if (idx < 0) {
                    const char* msg = "ERROR: Registro no encontrado.\n";
                    send(cliente_fd, msg, strlen(msg), 0);
                } else {
                    Registro* reg = &tc.registros_locales[idx];
                    while (1) {
                        char menu_mod[512];
                        snprintf(menu_mod, sizeof(menu_mod),
                            "\nModificar registro %d:\n1 - DNI (%s)\n2 - Nombre (%s)\n3 - Apellido (%s)\n4 - Carrera (%s)\n5 - Materias (%s)\n0 - Terminar\nElija campo a modificar: ",
                            reg->id, reg->dni, reg->nombre, reg->apellido, reg->carrera, reg->materias);
                        send(cliente_fd, menu_mod, strlen(menu_mod), 0);
                        bytes_leidos = recv(cliente_fd, buffer, sizeof(buffer) -1, 0);
                        if (bytes_leidos <= 0) break;
                        buffer[bytes_leidos] = '\0';
                        trim_nueva_linea(buffer);
                        if (strcmp(buffer, "0") == 0) break;

                        const char* pedir_valor = "Ingrese nuevo valor: ";
                        send(cliente_fd, pedir_valor, strlen(pedir_valor), 0);
                        bytes_leidos = recv(cliente_fd, buffer, sizeof(buffer) -1, 0);
                        if (bytes_leidos <= 0) break;
                        buffer[bytes_leidos] = '\0';
                        trim_nueva_linea(buffer);

                        switch (buffer[0]) {
                            case '1': strncpy(reg->dni, buffer, sizeof(reg->dni) -1); reg->dni[sizeof(reg->dni)-1] = '\0'; break;
                            case '2': strncpy(reg->nombre, buffer, sizeof(reg->nombre) -1); reg->nombre[sizeof(reg->nombre)-1] = '\0'; break;
                            case '3': strncpy(reg->apellido, buffer, sizeof(reg->apellido) -1); reg->apellido[sizeof(reg->apellido)-1] = '\0'; break;
                            case '4': strncpy(reg->carrera, buffer, sizeof(reg->carrera) -1); reg->carrera[sizeof(reg->carrera)-1] = '\0'; break;
                            case '5': strncpy(reg->materias, buffer, sizeof(reg->materias) -1); reg->materias[sizeof(reg->materias)-1] = '\0'; break;
                            default:
                                send(cliente_fd, "Opción inválida.\n", 17, 0);
                                break;
                        }
                    }
                    const char* msg = "Modificación aplicada en transacción local.\n";
                    send(cliente_fd, msg, strlen(msg), 0);
                }

            } else if (strcmp(buffer, "3") == 0) {
                const char* pedir_id = "Ingrese ID a eliminar: ";
                send(cliente_fd, pedir_id, strlen(pedir_id), 0);
                bytes_leidos = recv(cliente_fd, buffer, sizeof(buffer) -1, 0);
                if (bytes_leidos <= 0) break;
                buffer[bytes_leidos] = '\0';

                if (!es_numero_valido(buffer)) {
                    const char* msg = "ID inválido.\n";
                    send(cliente_fd, msg, strlen(msg), 0);
                    continue;
                }

                int id = atoi(buffer);
                int idx = buscar_registro_local(&tc, id);
                if (idx < 0) {
                    const char* msg = "ERROR: Registro no encontrado.\n";
                    send(cliente_fd, msg, strlen(msg), 0);
                } else {
                    tc.registros_locales[idx].activo = 0;
                    const char* msg = "Registro eliminado en transacción local.\n";
                    send(cliente_fd, msg, strlen(msg), 0);
                }

            } else if (strcmp(buffer, "4") == 0) {
                pthread_mutex_lock(&mutex_transaccion);

                if (cliente_transaccion_fd == cliente_fd && transaccion_activa) {
                    if (confirmar_transaccion(cliente_fd, tc.registros_locales, tc.total_locales)) {
                        const char* msg = "Commit realizado y transacción finalizada.\n";
                        send(cliente_fd, msg, strlen(msg), 0);
                        tc.en_transaccion = 0;
                    } else {
                        const char* msg = "ERROR: Falló el commit.\n";
                        send(cliente_fd, msg, strlen(msg), 0);
                    }
                } else {
                    const char* msg = "ERROR: No se puede hacer commit.\n";
                    send(cliente_fd, msg, strlen(msg), 0);
                }

                pthread_mutex_unlock(&mutex_transaccion);

            } else if (strcmp(buffer, "0") == 0) {
                finalizar_cliente(&tc);
            } else {
                const char* msg = "Opción no válida.\n";
                send(cliente_fd, msg, strlen(msg), 0);
            }
        }
    }

    finalizar_cliente(&tc);
    return NULL;
}

void configurar_servidor(int argc, char* argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Uso: %s <IP> <PUERTO> <MAX_CONCURRENTES> <MAX_EN_ESPERA>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* ip = argv[1];
    int puerto = atoi(argv[2]);
    int max_concurrentes = atoi(argv[3]);
    int max_en_espera = atoi(argv[4]);

    if (max_concurrentes <= 0 || max_en_espera <= 0) {
        fprintf(stderr, "ERROR: <MAX_CONCURRENTES> y <MAX_EN_ESPERA> deben ser mayores que cero.\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, cerrar_servidor);

    MAX_CONCURRENTES = max_concurrentes;
    sem_init(&sem_concurrentes, 0, MAX_CONCURRENTES);

    server_fd_global = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_global < 0) {
        perror("Error creando socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(puerto);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (bind(server_fd_global, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        close(server_fd_global);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd_global, max_en_espera) < 0) {
        perror("Error en listen");
        close(server_fd_global);
        exit(EXIT_FAILURE);
    }

    log_info("Servidor iniciado en %s:%d con %d clientes concurrentes y %d en espera",
             ip, puerto, max_concurrentes, max_en_espera);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int* cliente_fd = malloc(sizeof(int));
        if (!cliente_fd) continue;

        *cliente_fd = accept(server_fd_global, (struct sockaddr*)&client_addr, &addr_len);
        if (*cliente_fd < 0) {
            perror("Error en accept");
            free(cliente_fd);
            continue;
        }

        sem_wait(&sem_concurrentes);

        log_info("Cliente conectado: %s:%d",
                 inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t hilo;
        pthread_create(&hilo, NULL, manejar_cliente, cliente_fd);
        pthread_detach(hilo);
    }

    close(server_fd_global);
}
