#ifndef FUNCION_H
#define FUNCION_H

#include <stdio.h>

#define MAX_ALUMNOS 200
#define MAX_LINEA 256
#define BUFFER 512
#define RUTA_CSV "../Ejercicio1/alumnos.csv"

typedef struct {
    int ID;
    int DNI;
    char nombre[30];
    char apellido[30];
    char carrera[30];
    int materias;
} t_alumno;



/* Datos en memoria (servidor) */
extern t_alumno alumnos[MAX_ALUMNOS];
extern int cantidad_alumnos;

/* Funciones CSV / operaciones */
void leer_alumnos_csv();
int guardar_alumnos_csv();

/* Operaciones sobre arrays */
int buscar_por_ID(t_alumno arr[], int cnt, int id); /* devuelve índice o -1 */
void listar_a_buffer(t_alumno arr[], int cnt, char *out, size_t out_size);
int alta_en_array(t_alumno arr[], int *cnt, t_alumno a);
int baja_en_array(t_alumno arr[], int *cnt, int id);
int modificar_en_array(t_alumno arr[], int cnt, int id, const char* campo, const char* valor);
void consulta_por_campo(t_alumno arr[], int cnt, const char* campo, const char* valor, char* out, size_t out_size);

#endif
