//COMPILAR: gcc main.c -o cliente
//gcc servidor.c funcion.c -o servidor -lpthread
//gcc cliente.c -o cliente

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER 512

void enviar_comando(int sock, const char* comando) {
    char buffer[BUFFER];
    send(sock, comando, strlen(comando), 0);

    memset(buffer, 0, BUFFER);
    int n = recv(sock, buffer, BUFFER - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Servidor: %s\n", buffer);
    } else {
        printf("Servidor desconectado.\n");
        exit(1);
    }
}

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

    printf("Conectado al servidor %s:%d\n\n", ip, puerto);

    int opcion;
    char comando[BUFFER];

    do {
        printf("\n=== MENU CLIENTE ===\n");
        printf("1) BEGIN TRANSACTION\n");
        printf("2) CONSULTA\n");
        printf("3) ALTA\n");
        printf("4) BAJA\n");
        printf("5) MODIFICAR\n");
        printf("6) COMMIT TRANSACTION\n");
        printf("0) SALIR\n");
        printf("Seleccione: ");
        scanf("%d", &opcion);
        getchar(); // limpiar el \n

        switch (opcion) {
            case 1:
                enviar_comando(sock, "BEGIN TRANSACTION");
                break;

            case 2: {
                char campo[50], valor[50];
                printf("Campo a consultar (ID/DNI/APELLIDO): ");
                fgets(campo, sizeof(campo), stdin);
                campo[strcspn(campo, "\n")] = 0;
                printf("Valor: ");
                fgets(valor, sizeof(valor), stdin);
                valor[strcspn(valor, "\n")] = 0;
                snprintf(comando, BUFFER, "CONSULTA:%s=%s", campo, valor);
                enviar_comando(sock, comando);
                break;
            }

            case 3: {
                int id, dni, materias;
                char nombre[30], apellido[30], carrera[30];
                printf("ID: "); scanf("%d", &id); getchar();
                printf("DNI: "); scanf("%d", &dni); getchar();
                printf("Nombre: "); fgets(nombre, sizeof(nombre), stdin);
                nombre[strcspn(nombre, "\n")] = 0;
                printf("Apellido: "); fgets(apellido, sizeof(apellido), stdin);
                apellido[strcspn(apellido, "\n")] = 0;
                printf("Carrera: "); fgets(carrera, sizeof(carrera), stdin);
                carrera[strcspn(carrera, "\n")] = 0;
                printf("Materias: "); scanf("%d", &materias); getchar();

                snprintf(comando, BUFFER,
                         "ALTA:ID=%d,DNI=%d,NOMBRE=%s,APELLIDO=%s,CARRERA=%s,MATERIAS=%d",
                         id, dni, nombre, apellido, carrera, materias);
                enviar_comando(sock, comando);
                break;
            }

            case 4: {
                int id;
                printf("ID a dar de baja: ");
                scanf("%d", &id); getchar();
                snprintf(comando, BUFFER, "BAJA:ID=%d", id);
                enviar_comando(sock, comando);
                break;
            }

            case 5: {
                int id;
                char campo[30], valor[50];
                printf("ID a modificar: ");
                scanf("%d", &id); getchar();
                printf("Campo a modificar (NOMBRE/APELLIDO/CARRERA/MATERIAS): ");
                fgets(campo, sizeof(campo), stdin);
                campo[strcspn(campo, "\n")] = 0;
                printf("Nuevo valor: ");
                fgets(valor, sizeof(valor), stdin);
                valor[strcspn(valor, "\n")] = 0;
                snprintf(comando, BUFFER, "MODIFICAR:ID=%d,%s=%s", id, campo, valor);
                enviar_comando(sock, comando);
                break;
            }

            case 6:
                enviar_comando(sock, "COMMIT TRANSACTION");
                break;

            case 0:
                enviar_comando(sock, "SALIR");
                break;

            default:
                printf("Opción inválida.\n");
        }

    } while (opcion != 0);

    close(sock);
    return 0;
}
