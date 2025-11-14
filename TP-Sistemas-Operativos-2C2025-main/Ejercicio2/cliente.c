#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER 512

static int recv_into_buf(int sock, char *buf, size_t bufsz) {
    int n = recv(sock, buf, (int)bufsz - 1, 0);
    if (n > 0) buf[n] = '\0';
    return n;
}

static int recv_until_attention(int sock) {
    char tmp[BUFFER];
    int n;
    while ((n = recv_into_buf(sock, tmp, sizeof(tmp))) > 0) {
        printf("%s", tmp);
        if (strstr(tmp, "Ya está siendo atendido") != NULL) return 1;
        if (strstr(tmp, "ERROR: servidor ocupado") != NULL) return -1;
    }
    return 0; /* desconectado o error */
}

static int handle_initial_messages(int sock) {
    char buf[BUFFER];
    int n = recv_into_buf(sock, buf, sizeof(buf));
    if (n <= 0) {
        perror("recv");
        return 0;
    }
    printf("%s", buf);
    if (strstr(buf, "ERROR: servidor ocupado") != NULL) return -1;
    if (strstr(buf, "en espera") != NULL && strstr(buf, "Ya está siendo atendido") == NULL) {
        /* estamos en cola: esperar el aviso de atención */
        int r = recv_until_attention(sock);
        if (r <= 0) {
            printf("Servidor cerró la conexión mientras esperaba. Saliendo.\n");
            return 0;
        }
    } else if (strstr(buf, "Ya está siendo atendido") == NULL) {
        /* puede venir el segundo mensaje en otro paquete */
        int m = recv_into_buf(sock, buf, sizeof(buf));
        if (m > 0) printf("%s", buf);
    }
    return 1;
}

static void enviar_comando_listar(int sock) {
    char recvbuf[BUFFER];
    int r;
    while ((r = recv(sock, recvbuf, sizeof(recvbuf)-1, 0)) > 0) {
        recvbuf[r] = '\0';
        char *fin = strstr(recvbuf, "__FIN__");
        if (fin) {
            *fin = '\0';
            printf("%s", recvbuf);
            break;
        }
        printf("%s", recvbuf);
    }
    printf("\n");
}

static void enviar_comando_generico(int sock, const char *comando) {
    char buf[BUFFER];
    send(sock, comando, strlen(comando), 0);
    int r = recv(sock, buf, sizeof(buf)-1, 0);
    if (r > 0) {
        buf[r] = '\0';
        printf("%s", buf);
    } else {
        printf("Servidor desconectado.\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Uso: %s <IP_servidor> <PUERTO>\n", argv[0]);
        return 1;
    }
    char *ip = argv[1];
    int puerto = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in serv = {0};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(puerto);
    serv.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    int ok = handle_initial_messages(sock);
    if (ok <= 0) { close(sock); return 0; }

    printf("Conectado al servidor %s:%d\n", ip, puerto);

    while (1) {
        printf("\n=== MENU CLIENTE ===\n");
        printf("1) BEGIN TRANSACTION\n");
        printf("2) CONSULTA\n");
        printf("3) ALTA\n");
        printf("4) BAJA\n");
        printf("5) MODIFICAR\n");
        printf("6) COMMIT TRANSACTION\n");
        printf("7) LISTAR ALUMNOS\n");
        printf("0) SALIR\n");
        printf("Seleccione: ");

        int opcion;
        if (scanf("%d", &opcion) != 1) break;
        while (getchar() != '\n');

        char comando[BUFFER] = {0};

        switch (opcion) {
            case 0:
                strcpy(comando, "SALIR");
                send(sock, comando, strlen(comando), 0);
                close(sock);
                printf("Cliente finalizado.\n");
                return 0;
            case 1:
                strcpy(comando, "BEGIN TRANSACTION");
                enviar_comando_generico(sock, comando);
                break;
            case 2: {
                char campo[50], valor[50];
                printf("Campo a consultar (ID/DNI/APELLIDO): ");
                fgets(campo, sizeof(campo), stdin); campo[strcspn(campo,"\n")]=0;
                printf("Valor: ");
                fgets(valor, sizeof(valor), stdin); valor[strcspn(valor,"\n")]=0;
                snprintf(comando, sizeof(comando), "CONSULTA:%s=%s", campo, valor);
                enviar_comando_generico(sock, comando);
                break;
            }
            case 3: {
                int id,dni,materias;
                char nombre[30], apellido[30], carrera[30];
                printf("ID: "); scanf("%d",&id); while(getchar()!='\n');
                printf("DNI: "); scanf("%d",&dni); while(getchar()!='\n');
                printf("Nombre: "); fgets(nombre,sizeof(nombre),stdin); nombre[strcspn(nombre,"\n")]=0;
                printf("Apellido: "); fgets(apellido,sizeof(apellido),stdin); apellido[strcspn(apellido,"\n")]=0;
                printf("Carrera: "); fgets(carrera,sizeof(carrera),stdin); carrera[strcspn(carrera,"\n")]=0;
                printf("Materias: "); scanf("%d",&materias); while(getchar()!='\n');
                snprintf(comando, sizeof(comando),
                         "ALTA:ID=%d,DNI=%d,NOMBRE=%s,APELLIDO=%s,CARRERA=%s,MATERIAS=%d",
                         id,dni,nombre,apellido,carrera,materias);
                enviar_comando_generico(sock, comando);
                break;
            }
            case 4:
                { int id; printf("ID a dar de baja: "); scanf("%d",&id); while(getchar()!='\n');
                  snprintf(comando, sizeof(comando), "BAJA:ID=%d", id);
                  enviar_comando_generico(sock, comando);
                }
                break;
            case 5: {
                int id; char campo[30], valor[50];
                printf("ID a modificar: "); scanf("%d",&id); while(getchar()!='\n');
                printf("Campo a modificar (NOMBRE/APELLIDO/CARRERA/MATERIAS): ");
                fgets(campo,sizeof(campo),stdin); campo[strcspn(campo,"\n")]=0;
                printf("Nuevo valor: "); fgets(valor,sizeof(valor),stdin); valor[strcspn(valor,"\n")]=0;
                snprintf(comando, sizeof(comando), "MODIFICAR:ID=%d,%s=%s", id, campo, valor);
                enviar_comando_generico(sock, comando);
                break;
            }
            case 6:
                strcpy(comando, "COMMIT TRANSACTION");
                enviar_comando_generico(sock, comando);
                break;
            case 7:
                strcpy(comando, "LISTAR");
                send(sock, comando, strlen(comando), 0);
                enviar_comando_listar(sock);
                break;
            default:
                printf("Opción inválida.\n");
        }
    }

    close(sock);
    printf("Cliente finalizado.\n");
    return 0;
}