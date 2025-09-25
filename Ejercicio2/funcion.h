#ifndef FUNCION_H
#define FUNCION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_ALUMNOS 100
#define MAX_LINEA 256
#define BUFFER 512

typedef struct {
    int ID;
    int DNI;
    char nombre[30];
    char apellido[30];
    char carrera[30];
    int materias;
} t_alumnos;

// variables globales
extern t_alumnos alumnos[MAX_ALUMNOS];
extern int cantidad_alumnos;

// funciones
void leer_alumnos_csv();
void guardar_alumnos_csv();
void procesar_consulta(int sock, const char* comando);
void procesar_alta(int sock, const char* comando);
void procesar_baja(int sock, const char* comando);
void procesar_modificar(int sock, const char* comando);

#endif
