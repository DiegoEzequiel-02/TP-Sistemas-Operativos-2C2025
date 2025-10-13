#ifndef CSV_H
#define CSV_H

#include <stddef.h>  // para size_t

#define MAX_REGISTROS 1000
#define MAX_LINEA 512

typedef struct {
    int id;
    char dni[32];
    char nombre[64];
    char apellido[64];
    char carrera[32];
    char materias[128];
    int activo;
} Registro;

extern Registro registros[MAX_REGISTROS];
extern int total_registros;

void cargar_registros();
int guardar_registros();

void consultar_registro(int id, char* respuesta, size_t tamaño);
int agregar_registro(const char* linea_sin_id);
int eliminar_registro(int id);
int modificar_registro(int id, const Registro* nuevos_datos);

void parse_linea_to_registro(const char* linea, Registro* reg);
void registro_to_linea(const Registro* reg, char* linea);

#endif
