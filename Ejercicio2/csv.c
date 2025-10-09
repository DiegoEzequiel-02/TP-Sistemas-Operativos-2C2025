#include "csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSV_PATH "./alumnos.csv"

Registro registros[MAX_REGISTROS];
int total_registros = 0;

void parse_linea_to_registro(const char* linea, Registro* reg) {
    sscanf(linea, "%d|%31[^|]|%63[^|]|%63[^|]|%31[^|]|%127[^|]|",
        &reg->id, reg->dni, reg->nombre, reg->apellido, reg->carrera, reg->materias);
    reg->activo = 1;
}

void registro_to_linea(const Registro* reg, char* linea) {
    snprintf(linea, MAX_LINEA, "%04d|%s|%s|%s|%s|%s|\n",
        reg->id, reg->dni, reg->nombre, reg->apellido, reg->carrera, reg->materias);
}

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

        parse_linea_to_registro(linea, &registros[total_registros]);
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

    char linea[MAX_LINEA];
    for (int i = 0; i < total_registros; i++) {
        if (registros[i].activo) {
            registro_to_linea(&registros[i], linea);
            fprintf(f, "%s", linea);
        }
    }

    fclose(f);
}

char* consultar_registro(int id) {
    static char respuesta[MAX_LINEA];
    for (int i = 0; i < total_registros; i++) {
        if (registros[i].activo && registros[i].id == id) {
            registro_to_linea(&registros[i], respuesta);
            return respuesta;
        }
    }
    return "[✘] Registro no encontrado.\n";
}

int agregar_registro(const char* linea_sin_id) {
    if (total_registros >= MAX_REGISTROS) return 0;

    Registro* reg = &registros[total_registros];
    reg->id = (total_registros > 0) ? registros[total_registros - 1].id + 1 : 1;
    reg->activo = 1;

    sscanf(linea_sin_id, "%31[^|]|%63[^|]|%63[^|]|%31[^|]|%127[^|]|",
        reg->dni, reg->nombre, reg->apellido, reg->carrera, reg->materias);

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

int modificar_registro(int id, const Registro* nuevos_datos) {
    for (int i = 0; i < total_registros; i++) {
        if (registros[i].activo && registros[i].id == id) {
            registros[i] = *nuevos_datos;
            registros[i].id = id;
            registros[i].activo = 1;
            return 1;
        }
    }
    return 0;
}
