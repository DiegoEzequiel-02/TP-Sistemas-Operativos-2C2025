#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER 512

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Uso: %s <IP> <PUERTO>\n", argv[0]);
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
        scanf("%d", &opcion);
        getchar(); // limpiar \n

        char comando[BUFFER] = "";

        if (opcion == 0) {
            strcpy(comando, "SALIR");
            send(sock, comando, strlen(comando), 0);
            break;
        } else if (opcion == 1) {
            strcpy(comando, "BEGIN TRANSACTION");
        } else if (opcion == 2) {
            printf("Campo (ID/DNI/APELLIDO): ");
            char campo[32], valor[32];
            fgets(campo, 32, stdin); campo[strcspn(campo, "\n")] = 0;
            printf("Valor: ");
            fgets(valor, 32, stdin); valor[strcspn(valor, "\n")] = 0;
            snprintf(comando, BUFFER, "CONSULTA:%s,%s", campo, valor);
        } else if (opcion == 3) {
            int id, dni, materias;
            char nombre[30], apellido[30], carrera[30];

            // ID
            do {
                printf("ID (número entero > 0): ");
                if (scanf("%d", &id) != 1 || id <= 0) {
                    printf("Valor inválido. Intente nuevamente.\n");
                    while (getchar() != '\n');
                } else break;
            } while (1);
            while (getchar() != '\n');

            // DNI
            do {
                printf("DNI (número entero > 0): ");
                if (scanf("%d", &dni) != 1 || dni <= 0) {
                    printf("Valor inválido. Intente nuevamente.\n");
                    while (getchar() != '\n');
                } else break;
            } while (1);
            while (getchar() != '\n');

            // Nombre
            do {
                printf("Nombre: ");
                fgets(nombre, sizeof(nombre), stdin);
                nombre[strcspn(nombre, "\n")] = 0;
            } while (strlen(nombre) == 0);

            // Apellido
            do {
                printf("Apellido: ");
                fgets(apellido, sizeof(apellido), stdin);
                apellido[strcspn(apellido, "\n")] = 0;
            } while (strlen(apellido) == 0);

            // Carrera
            do {
                printf("Carrera: ");
                fgets(carrera, sizeof(carrera), stdin);
                carrera[strcspn(carrera, "\n")] = 0;
            } while (strlen(carrera) == 0);

            // Materias
            do {
                printf("Materias (número entero >= 0): ");
                if (scanf("%d", &materias) != 1 || materias < 0) {
                    printf("Valor inválido. Intente nuevamente.\n");
                    while (getchar() != '\n');
                } else break;
            } while (1);
            while (getchar() != '\n');

            snprintf(comando, BUFFER,
                "ALTA:ID=%d,DNI=%d,NOMBRE=%s,APELLIDO=%s,CARRERA=%s,MATERIAS=%d",
                id, dni, nombre, apellido, carrera, materias);
        } else if (opcion == 4) {
            printf("ID a dar de baja: ");
            int id;
            scanf("%d", &id); getchar();
            snprintf(comando, BUFFER, "BAJA:ID=%d", id);
        } else if (opcion == 5) {
            printf("ID a modificar: ");
            int id;
            scanf("%d", &id); getchar();
            printf("Campo a modificar (NOMBRE/APELLIDO/CARRERA/MATERIAS): ");
            char campo[32], valor[32];
            fgets(campo, 32, stdin); campo[strcspn(campo, "\n")] = 0;
            printf("Nuevo valor: ");
            fgets(valor, 32, stdin); valor[strcspn(valor, "\n")] = 0;
            snprintf(comando, BUFFER, "MODIFICAR:ID=%d,%s=%s", id, campo, valor);
        } else if (opcion == 6) {
            strcpy(comando, "COMMIT TRANSACTION");
        } else if (opcion == 7) {
            strcpy(comando, "LISTAR");
        } else {
            printf("Opción inválida.\n");
            continue;
        }

        send(sock, comando, strlen(comando), 0);

        memset(buffer, 0, BUFFER);
        if (opcion == 7) { // LISTAR
            int n;
            while ((n = recv(sock, buffer, BUFFER - 1, 0)) > 0) {
                buffer[n] = '\0';
                char* fin = strstr(buffer, "__FIN__");
                if (fin) {
                    *fin = '\0';
                    if (strlen(buffer) > 0) printf("%s", buffer); // imprime lo que vino antes de FIN
                    break;
                }
                printf("%s", buffer);
            }
            printf("\n");
            // El ciclo principal sigue y el menú vuelve a aparecer
        } else {
            int n = recv(sock, buffer, BUFFER, 0);
            if (n > 0) {
                buffer[n] = '\0';
                printf("%s\n", buffer);
            } else {
                printf("Servidor desconectado.\n");
                break;
            }
        }
    }

    close(sock);
    return 0;
}
