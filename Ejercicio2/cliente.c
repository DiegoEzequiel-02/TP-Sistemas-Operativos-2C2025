#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024

void mostrar_menu();
void limpiar_buffer();

int main(int argc, char *argv[]) {
    char ip[64] = "127.0.0.1";
    int puerto = 5000;

    // Permitir configuración por archivo
    if (argc == 3) {
        strncpy(ip, argv[1], sizeof(ip));
        puerto = atoi(argv[2]);
    } else {
        FILE* f = fopen("config.txt", "r");
        if (f) {
            char linea[128];
            while (fgets(linea, sizeof(linea), f)) {
                if (strncmp(linea, "IP=", 3) == 0)
                    sscanf(linea + 3, "%63s", ip);
                else if (strncmp(linea, "PUERTO=", 7) == 0)
                    sscanf(linea + 7, "%d", &puerto);
            }
            fclose(f);
        }
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket");
        return 1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(puerto);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Conexión");
        return 1;
    }

    printf("\n[✔] Conectado a %s:%d\n", ip, puerto);

    int opcion;
    char buffer[MAX_BUFFER];
    char respuesta[MAX_BUFFER];

    while (1) {
        mostrar_menu();
        printf("Elija una opción: ");
        scanf("%d", &opcion);
        limpiar_buffer();

        switch (opcion) {
            case 1:
                send(sockfd, "BEGIN TRANSACTION", 18, 0);
                break;

            case 2:
                send(sockfd, "COMMIT", 6, 0);
                break;

            case 3:
                send(sockfd, "ROLLBACK", 8, 0);
                break;

            case 4: {
                int id;
                printf("ID del registro: ");
                scanf("%d", &id);
                limpiar_buffer();
                snprintf(buffer, sizeof(buffer), "CONSULTAR %d", id);
                send(sockfd, buffer, strlen(buffer), 0);
                break;
            }

            case 5: {
                char linea[MAX_BUFFER];
                printf("Ingrese los datos (DNI|Nombre|Apellido|Carrera|Materias):\n> ");
                fgets(linea, sizeof(linea), stdin);
                linea[strcspn(linea, "\n")] = 0;
                snprintf(buffer, sizeof(buffer), "AGREGAR %.900s", linea);
                send(sockfd, buffer, strlen(buffer), 0);
                break;
            }

            case 6: {
                int id;
                char datos[MAX_BUFFER];
                printf("ID del registro a modificar: ");
                scanf("%d", &id);
                limpiar_buffer();
                printf("Nuevos datos (DNI|Nombre|Apellido|Carrera|Materias):\n> ");
                fgets(datos, sizeof(datos), stdin);
                datos[strcspn(datos, "\n")] = 0;
                snprintf(buffer, sizeof(buffer), "MODIFICAR %d %.900s", id, datos);
                send(sockfd, buffer, strlen(buffer), 0);
                break;
            }

            case 7: {
                int id;
                printf("ID del registro a eliminar: ");
                scanf("%d", &id);
                limpiar_buffer();
                snprintf(buffer, sizeof(buffer), "ELIMINAR %d", id);
                send(sockfd, buffer, strlen(buffer), 0);
                break;
            }

            case 0:
                send(sockfd, "SALIR", 5, 0);
                close(sockfd);
                printf("[✔] Sesión cerrada.\n");
                return 0;

            default:
                printf("[✘] Opción inválida.\n");
                continue;
        }

        // Respuesta del servidor
        memset(respuesta, 0, sizeof(respuesta));
        int bytes = recv(sockfd, respuesta, sizeof(respuesta) - 1, 0);
        if (bytes <= 0) {
            printf("[✘] Conexión cerrada por el servidor.\n");
            break;
        }
        printf("\n📬 Respuesta:\n%s\n", respuesta);
    }

    close(sockfd);
    return 0;
}

void mostrar_menu() {
    printf("\n--- MENÚ CLIENTE ---\n");
    printf("1. BEGIN TRANSACTION\n");
    printf("2. COMMIT\n");
    printf("3. ROLLBACK\n");
    printf("4. CONSULTAR registro\n");
    printf("5. AGREGAR registro\n");
    printf("6. MODIFICAR registro\n");
    printf("7. ELIMINAR registro\n");
    printf("0. SALIR\n");
}

void limpiar_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
