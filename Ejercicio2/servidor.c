#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "funcion.h"

#define BACKLOG_DEFAULT 10

/* Concurrencia y lock de transacción */
pthread_mutex_t mutex_trans = PTHREAD_MUTEX_INITIALIZER;
int trans_lock_sock = 0; /* socket que posee el lock (0 = ninguno) */

/* semáforo para limitar N concurrentes */
sem_t sem_concurrentes;

/* contador de cliente-id seguro */
pthread_mutex_t id_lock = PTHREAD_MUTEX_INITIALIZER;
int siguiente_id = 1;

/* ruta CSV (setear desde argv) -> utils.c usa csv_path global */
void enviar_respuesta(int sock, const char* msg) {
    send(sock, msg, strlen(msg), 0);
}

typedef struct {
    int sock;
    int id;
} cliente_info;

/* rutina del hilo cliente */
void* manejar_cliente(void* arg) {
    cliente_info *info = (cliente_info*)arg;
    int sock = info->sock;
    int id = info->id;
    free(info);

    /* cada hilo tendrá su copia temporal para transacciones */
    t_alumno tmp_alumnos[MAX_ALUMNOS];
    int tmp_cnt = 0;
    int en_transaccion = 0;

    printf("[Cliente %d conectado] (sock=%d)\n", id, sock);

    char buf[BUFFER];
    while (1) {
        memset(buf,0,sizeof(buf));
        int n = recv(sock, buf, BUFFER - 1, 0);
        if (n <= 0) {
            printf("[Cliente %d] desconectó (recv=%d)\n", id, n);
            break;
        }
        buf[n] = '\0';
        /* eliminar \r\n si existen */
        buf[strcspn(buf, "\r\n")] = 0;
        printf("[Cliente %d] cmd: %s\n", id, buf);

        /* COMANDOS */
        if (strcasecmp(buf, "BEGIN TRANSACTION") == 0 || strcasecmp(buf, "BEGIN") == 0) {
            /* intentar tomar el lock (no bloqueante para dar mensaje) */
            if (pthread_mutex_trylock(&mutex_trans) == 0) {
                trans_lock_sock = sock;
                /* copiar datos globales a tmp */
                memcpy(tmp_alumnos, alumnos, sizeof(alumnos));
                tmp_cnt = cantidad_alumnos;
                en_transaccion = 1;
                enviar_respuesta(sock, "OK: transacción iniciada\n");
            } else {
                enviar_respuesta(sock, "ERROR: transacción activa por otro cliente\n");
            }
        }
        else if (strcasecmp(buf, "COMMIT TRANSACTION") == 0 || strcasecmp(buf, "COMMIT") == 0) {
            printf("[DEBUG] COMMIT desde cliente %d -> copiar tmp a global\n", id);
            if (!en_transaccion || trans_lock_sock != sock) {
                enviar_respuesta(sock, "ERROR: no posee transacción activa\n");
            } else {
                /* copiar tmp -> global, guardar CSV */
                memcpy(alumnos, tmp_alumnos, sizeof(alumnos));
                cantidad_alumnos = tmp_cnt;
                if (guardar_alumnos_csv() == 0)
                    enviar_respuesta(sock, "OK: transacción confirmada\n");
                else
                    enviar_respuesta(sock, "ERROR: fallo al guardar CSV\n");
                /* liberar lock */
                trans_lock_sock = 0;
                pthread_mutex_unlock(&mutex_trans);
                en_transaccion = 0;
            }
        }
        else if (strcasecmp(buf, "SALIR") == 0 || strcasecmp(buf, "EXIT") == 0) {
            enviar_respuesta(sock, "OK: adiós\n");
            break;
        }
        else if (strncasecmp(buf, "LISTAR", 6) == 0) {
            if (trans_lock_sock != 0 && trans_lock_sock != sock) {
                enviar_respuesta(sock, "ERROR: transacción activa, reintente luego\n");
            } else {
                int cnt;
                t_alumno *arr;
                if (en_transaccion) {
                    arr = tmp_alumnos;
                    cnt = tmp_cnt;
                } else {
                    arr = alumnos;
                    cnt = cantidad_alumnos;
                }
                for (int i = 0; i < cnt; i++) {
                    char linea[BUFFER];
                    snprintf(linea, BUFFER,
                        "ID: %d\n- Nombre: %s\n- Apellido: %s\n- Carrera: %s\n- Materias: %d\n\n",
                        arr[i].ID,
                        arr[i].nombre,
                        arr[i].apellido,
                        arr[i].carrera,
                        arr[i].materias
                    );
                    send(sock, linea, strlen(linea), 0);
                }
                if (cnt == 0)
                    send(sock, "No hay alumnos registrados.\n", 29, 0);
                send(sock, "__FIN__\n", 7, 0);
            }
        } else if (strncasecmp(buf, "CONSULTA:",9) == 0 || strncasecmp(buf, "CONSULTA",8) == 0) {
            /* documentación: CONSULTA:CAMPO=VALOR  (ej: CONSULTA:APELLIDO=GOMEZ) */
            if (trans_lock_sock != 0 && trans_lock_sock != sock) {
                enviar_respuesta(sock, "ERROR: transacción activa, reintente luego\n");
            } else {
                char *payload = strchr(buf, ':');
                if (!payload) { enviar_respuesta(sock, "ERROR: formato CONSULTA inválido\n"); continue; }
                payload++; /* avanza después de ':' */
                char campo[50], valor[50];
                char *eq = strchr(payload, '=');
                if (!eq) { enviar_respuesta(sock, "ERROR: formato CONSULTA inválido\n"); continue; }
                *eq = 0;
                strncpy(campo, payload, sizeof(campo)-1); campo[sizeof(campo)-1]=0;
                strncpy(valor, eq+1, sizeof(valor)-1); valor[sizeof(valor)-1]=0;
                char out[BUFFER*2];
                if (en_transaccion)
                    consulta_por_campo(tmp_alumnos, tmp_cnt, campo, valor, out, sizeof(out));
                else
                    consulta_por_campo(alumnos, cantidad_alumnos, campo, valor, out, sizeof(out));
                send(sock, out, strlen(out), 0);
            }
        }
        else if (strncasecmp(buf, "ALTA:",5) == 0 || strncasecmp(buf, "ALTA",4) == 0) {
            /* ALTA:ID=..,DNI=..,NOMBRE=..,APELLIDO=..,CARRERA=..,MATERIAS=.. */
            if (!en_transaccion || trans_lock_sock != sock) {
                enviar_respuesta(sock, "ERROR: debe iniciar transacción (BEGIN TRANSACTION)\n");
            } else {
                printf("[DEBUG] alta temporal -> tmp_alumnos en cliente %d\n", id);
                char *payload = strchr(buf, ':');
                if (!payload) { enviar_respuesta(sock,"ERROR: formato ALTA inválido\n"); continue; }
                payload++;
                /* parse simple con strtok */
                t_alumno a; memset(&a,0,sizeof(a));
                char tmp_line[MAX_LINEA];
                strncpy(tmp_line, payload, sizeof(tmp_line)-1); tmp_line[sizeof(tmp_line)-1]=0;
                char *tok = strtok(tmp_line, ",");
                while (tok) {
                    char campo[50], valor[50];
                    if (sscanf(tok, "%[^=]=%s", campo, valor) == 2) {
                        if (strcasecmp(campo,"ID")==0) a.ID = atoi(valor);
                        else if (strcasecmp(campo,"DNI")==0) a.DNI = atoi(valor);
                        else if (strcasecmp(campo,"NOMBRE")==0) strncpy(a.nombre, valor,29);
                        else if (strcasecmp(campo,"APELLIDO")==0) strncpy(a.apellido, valor,29);
                        else if (strcasecmp(campo,"CARRERA")==0) strncpy(a.carrera, valor,29);
                        else if (strcasecmp(campo,"MATERIAS")==0) a.materias = atoi(valor);
                    }
                    tok = strtok(NULL, ",");
                }
                int r = alta_en_array(tmp_alumnos, &tmp_cnt, a);
                if (r == 0) enviar_respuesta(sock, "OK: alumno agregado (temporal)\n");
                else if (r == -2) enviar_respuesta(sock, "ERROR: ID duplicado\n");
                else enviar_respuesta(sock, "ERROR: no se pudo agregar\n");
            }
        }
        else if (strncasecmp(buf, "BAJA:",5) == 0) {
            /* BAJA:ID=3 */
            if (!en_transaccion || trans_lock_sock != sock) {
                enviar_respuesta(sock, "ERROR: debe iniciar transacción (BEGIN TRANSACTION)\n");
            } else {
                int id;
                if (sscanf(buf, "BAJA:ID=%d", &id) != 1) {
                    enviar_respuesta(sock, "ERROR: formato BAJA inválido\n");
                } else {
                    int r = baja_en_array(tmp_alumnos, &tmp_cnt, id);
                    if (r == 0) enviar_respuesta(sock, "OK: alumno dado de baja (temporal)\n");
                    else enviar_respuesta(sock, "ERROR: ID no encontrado\n");
                }
            }
        }
        else if (strncasecmp(buf, "MODIFICAR:",10) == 0) {
            /* MODIFICAR:ID=3,APELLIDO=Nuevo */
            if (!en_transaccion || trans_lock_sock != sock) {
                enviar_respuesta(sock, "ERROR: debe iniciar transacción (BEGIN TRANSACTION)\n");
            } else {
                int id;
                char campo[50], valor[50];
                if (sscanf(buf, "MODIFICAR:ID=%d,%[^=]=%s", &id, campo, valor) < 3) {
                    enviar_respuesta(sock, "ERROR: formato MODIFICAR inválido\n");
                } else {
                    int r = modificar_en_array(tmp_alumnos, tmp_cnt, id, campo, valor);
                    if (r == 0) enviar_respuesta(sock, "OK: alumno modificado (temporal)\n");
                    else if (r == -1) enviar_respuesta(sock, "ERROR: ID no encontrado\n");
                    else enviar_respuesta(sock, "ERROR: campo inválido\n");
                }
            }
        }
        else {
            enviar_respuesta(sock, "ERROR: comando inválido\n");
        }
    }

    /* cleanup: si se desconecta mientras tiene lock, liberar */
    if (en_transaccion && trans_lock_sock == sock) {
        trans_lock_sock = 0;
        pthread_mutex_unlock(&mutex_trans);
        printf("[Cliente %d] desconectado: lock liberado\n", id);
    }

    close(sock);
    sem_post(&sem_concurrentes);
    printf("[Cliente %d] hilo finalizado\n", id);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Uso: %s <IP> <PUERTO> <N_concurrentes> <M_espera> [ruta_csv]\n", argv[0]);
        return 1;
    }
    char* ip = argv[1];
    int puerto = atoi(argv[2]);
    int N = atoi(argv[3]);
    int M = atoi(argv[4]);
    if (argc >= 6) strncpy(RUTA_CSV, argv[5], sizeof(RUTA_CSV)-1);

    leer_alumnos_csv();

    sem_init(&sem_concurrentes, 0, N);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in serv = {0};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(puerto);
    serv.sin_addr.s_addr = inet_addr(ip);

    if (bind(server_fd, (struct sockaddr*)&serv, sizeof(serv)) < 0) { perror("bind"); return 1; }
    if (listen(server_fd, M) < 0) { perror("listen"); return 1; }

    printf("Servidor en %s:%d  (N=%d concurrentes, M=%d espera) CSV='%s'\n", ip, puerto, N, M, RUTA_CSV);

    while (1) {
        struct sockaddr_in cliente;
        socklen_t tam = sizeof(cliente);

        /* aceptar conexión (no bloquea semáforo todavía) */
        int sock = accept(server_fd, (struct sockaddr*)&cliente, &tam);
        if (sock < 0) continue;

        /* esperar un slot disponible */
        sem_wait(&sem_concurrentes);

        /* ahora atendemos: enviar mensaje y crear hilo */
        send(sock, "Bienvenido. Espere si hay muchos usuarios conectados...\n", 55, 0);
        send(sock, "Ya está siendo atendido por el servidor.\n", 39, 0);

        cliente_info *info = malloc(sizeof(cliente_info));
        info->sock = sock;
        pthread_mutex_lock(&id_lock);
        info->id = siguiente_id++;
        pthread_mutex_unlock(&id_lock);

        pthread_t hilo;
        pthread_create(&hilo, NULL, manejar_cliente, info);
        pthread_detach(hilo);
    }

    close(server_fd);
    sem_destroy(&sem_concurrentes);
    return 0;
}
