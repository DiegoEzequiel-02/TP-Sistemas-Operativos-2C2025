#include "csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REGISTROS 1000
#define MAX_LINEA 256
#define CSV_PATH "./alumnos.csv"

typedef struct {
    int id;
    char linea[MAX_LINEA];
    int activo;
} RegistroMem;

RegistroMem registros[MAX_REGISTROS];
int total_registros = 0;

void cargar_registros() {
    FILE* f = fopen(CSV_PATH, "r");
    if (!f) {
        perror("No se pudo abrir alumnos.csv");
        return;
    }

    char linea[MAX_LINEA];
    fgets(linea, sizeof(linea), f);  // encabezado

    total_registros = 0;
    while (fgets(linea, sizeof(linea), f)) {
        if (total_registros >= MAX_REGISTROS) break;

        RegistroMem* reg = &registros[total_registros];
        reg->activo = 1;
        strcpy(reg->linea, linea);
        sscanf(linea, "%d|", &reg->id);
        total_registros++;
    }

    fclose(f);
}

void guardar_registros() {
    FILE* f = fopen(CSV_PATH, "w");
    if (!f) {
        perror("Error al escribir CSV");
        return;
    }

    fprintf(f, "ID|DNI|Nombre|Apellido|Carrera|Materias|\n");

    for (int i = 0; i < total_registros; i++) {
        if (registros[i].activo) {
            fprintf(f, "%s", registros[i].linea);
        }
    }

    fclose(f);
}

char* consultar_registro(int id) {
    static char respuesta[MAX_LINEA];
    for (int i = 0; i < total_registros; i++) {
        if (registros[i].activo && registros[i].id == id) {
            snprintf(respuesta, sizeof(respuesta), "[✔] Registro: %.200s", registros[i].linea);
            return respuesta;
        }
    }
    return "[✘] Registro no encontrado.\n";
}

int agregar_registro(const char* linea_sin_id) {
    if (total_registros >= MAX_REGISTROS) return 0;

    RegistroMem* reg = &registros[total_registros];
    reg->id = (total_registros > 0) ? registros[total_registros - 1].id + 1 : 1;
    reg->activo = 1;

    snprintf(reg->linea, sizeof(reg->linea), "%04d|%s\n", reg->id, linea_sin_id);
    total_registros++;

    return 1;
}

int eliminar_registro(int id) {
    for (int i = 0; i < total_registros; i++) {
        if (registros[i].activo && registros[i].id == id) {
            registros[i].activo = 0;
            return 1;
        }
    }
    return 0;
}

int modificar_registro(int id, const char* nuevos_datos) {
    for (int i = 0; i < total_registros; i++) {
        if (registros[i].activo && registros[i].id == id) {
            snprintf(registros[i].linea, MAX_LINEA, "%04d|%s\n", id, nuevos_datos);
            return 1;
        }
    }
    return 0;
}
