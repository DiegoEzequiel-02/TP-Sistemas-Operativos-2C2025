#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "funcion.h"

#define MAX_ALUMNOS 100
#define MAX_LINEA 256
#define BUFFER 512

t_alumnos alumnos[MAX_ALUMNOS];
int cantidad_alumnos = 0;

// Función auxiliar: comparación insensible a mayúsculas
int strcmpi(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower(*a) != tolower(*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

void leer_alumnos_csv() {
    FILE *archivo = fopen("../Ejercicio1/alumnos.csv", "r");
    if (!archivo) {
        perror("No se pudo abrir el archivo alumnos.csv");
        return;
    }

    char linea[MAX_LINEA];
    cantidad_alumnos = 0;
    while (fgets(linea, sizeof(linea), archivo) && cantidad_alumnos < MAX_ALUMNOS) {
        t_alumnos a;
        char *token = strtok(linea, ",");
        if (token) a.ID = atoi(token);

        token = strtok(NULL, ",");
        if (token) a.DNI = atoi(token);

        token = strtok(NULL, ",");
        if (token) { strncpy(a.nombre, token, 30); a.nombre[strcspn(a.nombre, "\n")] = 0; }

        token = strtok(NULL, ",");
        if (token) { strncpy(a.apellido, token, 30); a.apellido[strcspn(a.apellido, "\n")] = 0; }

        token = strtok(NULL, ",");
        if (token) { strncpy(a.carrera, token, 30); a.carrera[strcspn(a.carrera, "\n")] = 0; }

        token = strtok(NULL, ",\n");
        if (token) a.materias = atoi(token);

        alumnos[cantidad_alumnos++] = a;
    }
    fclose(archivo);
}

void guardar_alumnos_csv() {
    FILE *archivo = fopen("../Ejercicio1/alumnos.csv", "w");
    if (!archivo) {
        perror("Error al abrir CSV para guardar");
        return;
    }

    for (int i = 0; i < cantidad_alumnos; i++) {
        fprintf(archivo, "%d,%d,%s,%s,%s,%d\n",
                alumnos[i].ID,
                alumnos[i].DNI,
                alumnos[i].nombre,
                alumnos[i].apellido,
                alumnos[i].carrera,
                alumnos[i].materias);
    }
    fclose(archivo);
}

void procesar_consulta(int sock, const char* comando) {
    char aux[BUFFER];
    snprintf(aux, BUFFER, "%s", comando + 9); // "CONSULTA:"
    aux[BUFFER-1] = 0;

    char* campo = strtok(aux, "=");
    char* valor = strtok(NULL, "=");
    if (!campo || !valor) {
        send(sock, "ERROR: formato de consulta\n", 28, 0);
        return;
    }

    valor[strcspn(valor, "\n")] = 0;

    char resultado[BUFFER*2];
    resultado[0] = '\0';

    for (int i = 0; i < cantidad_alumnos; i++) {
        alumnos[i].nombre[strcspn(alumnos[i].nombre, "\n")] = 0;
        alumnos[i].apellido[strcspn(alumnos[i].apellido, "\n")] = 0;
        alumnos[i].carrera[strcspn(alumnos[i].carrera, "\n")] = 0;

        int match = 0;
        if (strcmpi(campo, "ID") == 1 && alumnos[i].ID == atoi(valor)) match = 1;
        else if (strcmpi(campo, "DNI") == 1 && alumnos[i].DNI == atoi(valor)) match = 1;
        else if (strcmpi(campo, "APELLIDO") == 1 && strcmpi(alumnos[i].apellido, valor) == 1) match = 1;

        if (match) {
            char linea[BUFFER];
            snprintf(linea, BUFFER, "%d,%d,%s,%s,%s,%d\n",
                     alumnos[i].ID, alumnos[i].DNI, alumnos[i].nombre,
                     alumnos[i].apellido, alumnos[i].carrera, alumnos[i].materias);
            strncat(resultado, linea, sizeof(resultado) - strlen(resultado) - 1);
        }
    }

    if (strlen(resultado) == 0)
        send(sock, "No se encontraron registros\n", 28, 0);
    else
        send(sock, resultado, strlen(resultado), 0);
}

void procesar_alta(int sock, const char* comando) {
    if (cantidad_alumnos >= MAX_ALUMNOS) {
        send(sock, "ERROR: capacidad máxima alcanzada\n", 35, 0);
        return;
    }

    t_alumnos a;
    char aux[BUFFER];
    snprintf(aux, BUFFER, "%s", comando + 5); // "ALTA:"
    aux[BUFFER-1] = 0;

    char* token = strtok(aux, ",");
    while (token) {
        char campo[50], valor[50];
        sscanf(token, "%[^=]=%s", campo, valor);
        valor[strcspn(valor, "\n")] = 0;

        if (strcmpi(campo, "ID") == 1) a.ID = atoi(valor);
        else if (strcmpi(campo, "DNI") == 1) a.DNI = atoi(valor);
        else if (strcmpi(campo, "NOMBRE") == 1) { strncpy(a.nombre, valor, 30); a.nombre[29]=0; }
        else if (strcmpi(campo, "APELLIDO") == 1) { strncpy(a.apellido, valor, 30); a.apellido[29]=0; }
        else if (strcmpi(campo, "CARRERA") == 1) { strncpy(a.carrera, valor, 30); a.carrera[29]=0; }
        else if (strcmpi(campo, "MATERIAS") == 1) a.materias = atoi(valor);

        token = strtok(NULL, ",");
    }

    alumnos[cantidad_alumnos++] = a;
    guardar_alumnos_csv();
    send(sock, "OK: alumno dado de alta\n", 25, 0);
}

void procesar_baja(int sock, const char* comando) {
    int id = atoi(comando + 5); // "BAJA:ID="
    int encontrado = 0;
    for (int i = 0; i < cantidad_alumnos; i++) {
        if (alumnos[i].ID == id) {
            alumnos[i] = alumnos[cantidad_alumnos - 1];
            cantidad_alumnos--;
            guardar_alumnos_csv();
            send(sock, "OK: alumno dado de baja\n", 25, 0);
            encontrado = 1;
            break;
        }
    }
    if (!encontrado)
        send(sock, "ERROR: ID no encontrado\n", 26, 0);
}

void procesar_modificar(int sock, const char* comando) {
    int id;
    char campo[50], valor[50];
    sscanf(comando, "MODIFICAR:ID=%d,%[^=]=%s", &id, campo, valor);
    valor[strcspn(valor, "\n")] = 0;

    int encontrado = 0;
    for (int i = 0; i < cantidad_alumnos; i++) {
        if (alumnos[i].ID == id) {
            if (strcmpi(campo, "NOMBRE") == 1) { strncpy(alumnos[i].nombre, valor, 30); alumnos[i].nombre[29]=0; }
            else if (strcmpi(campo, "APELLIDO") == 1) { strncpy(alumnos[i].apellido, valor, 30); alumnos[i].apellido[29]=0; }
            else if (strcmpi(campo, "CARRERA") == 1) { strncpy(alumnos[i].carrera, valor, 30); alumnos[i].carrera[29]=0; }
            else if (strcmpi(campo, "MATERIAS") == 1) alumnos[i].materias = atoi(valor);

            guardar_alumnos_csv();
            send(sock, "OK: alumno modificado\n", 23, 0);
            encontrado = 1;
            break;
        }
    }
    if (!encontrado)
        send(sock, "ERROR: ID no encontrado\n", 26, 0);
}
