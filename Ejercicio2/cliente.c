#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER 512

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

    char buf[BUFFER];
    /* recibir bienvenida/espera */
    int n = recv(sock, buf, BUFFER-1, 0);
    if (n > 0) { buf[n]=0; printf("%s", buf); }

    n = recv(sock, buf, BUFFER-1, 0);
    if (n > 0) { buf[n]=0; printf("%s", buf); }

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
        if (opcion == 0) {
            strcpy(comando, "SALIR");
            send(sock, comando, strlen(comando), 0);
            break;
        } else if (opcion == 1) strcpy(comando, "BEGIN TRANSACTION");
        else if (opcion == 6) strcpy(comando, "COMMIT TRANSACTION");
        else if (opcion == 7) strcpy(comando, "LISTAR");
        else if (opcion == 2) {
            char campo[50], valor[50];
            printf("Campo a consultar (ID/DNI/APELLIDO): ");
            fgets(campo, sizeof(campo), stdin); campo[strcspn(campo,"\n")]=0;
            printf("Valor: ");
            fgets(valor, sizeof(valor), stdin); valor[strcspn(valor,"\n")]=0;
            snprintf(comando, sizeof(comando), "CONSULTA:%s=%s", campo, valor);
        } else if (opcion == 3) {
            int id,dni,materias; char nombre[30], apellido[30], carrera[30];
            printf("ID: "); scanf("%d",&id); while(getchar()!='\n');
            printf("DNI: "); scanf("%d",&dni); while(getchar()!='\n');
            printf("Nombre: "); fgets(nombre,sizeof(nombre),stdin); nombre[strcspn(nombre,"\n")]=0;
            printf("Apellido: "); fgets(apellido,sizeof(apellido),stdin); apellido[strcspn(apellido,"\n")]=0;
            printf("Carrera: "); fgets(carrera,sizeof(carrera),stdin); carrera[strcspn(carrera,"\n")]=0;
            printf("Materias: "); scanf("%d",&materias); while(getchar()!='\n');
            snprintf(comando, sizeof(comando),
                     "ALTA:ID=%d,DNI=%d,NOMBRE=%s,APELLIDO=%s,CARRERA=%s,MATERIAS=%d",
                     id,dni,nombre,apellido,carrera,materias);
        } else if (opcion == 4) {
            int id; printf("ID a dar de baja: "); scanf("%d",&id); while(getchar()!='\n');
            snprintf(comando, sizeof(comando), "BAJA:ID=%d", id);
        } else if (opcion == 5) {
            int id; char campo[30], valor[50];
            printf("ID a modificar: "); scanf("%d",&id); while(getchar()!='\n');
            printf("Campo a modificar (NOMBRE/APELLIDO/CARRERA/MATERIAS): ");
            fgets(campo,sizeof(campo),stdin); campo[strcspn(campo,"\n")]=0;
            printf("Nuevo valor: "); fgets(valor,sizeof(valor),stdin); valor[strcspn(valor,"\n")]=0;
            snprintf(comando, sizeof(comando), "MODIFICAR:ID=%d,%s=%s", id, campo, valor);
        } else {
            printf("Opción inválida.\n");
            continue;
        }

        /* enviar el comando y recibir respuesta (o múltiples paquetes) */
        send(sock, comando, strlen(comando), 0);

        /* si LISTAR, recibir hasta marcador __FIN__ */
        if (strcmp(comando, "LISTAR") == 0) {
            char recvbuf[BUFFER];
            int r;
            while ((r = recv(sock, recvbuf, sizeof(recvbuf)-1, 0)) > 0) {
                recvbuf[r]=0;
                if (strstr(recvbuf, "__FIN__") != NULL) {
                    char *fin = strstr(recvbuf, "__FIN__");
                    *fin = '\0';
                    printf("%s", recvbuf);
                    break;
                }
                printf("%s", recvbuf);
            }
            printf("\n");
        } else {
            int r = recv(sock, buf, sizeof(buf)-1, 0);
            if (r > 0) {
                buf[r]=0;
                printf("%s", buf);
            } else {
                printf("Servidor desconectado.\n");
                break;
            }
        }
    }

    close(sock);
    printf("Cliente finalizado.\n");
    return 0;
}
