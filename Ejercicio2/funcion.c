#include "funcion.h"
#include <string.h>
#include <stdlib.h>

t_alumno alumnos[MAX_ALUMNOS];
int cantidad_alumnos = 0;

/* Lee CSV a 'alumnos' (simple, asume formato: ID,DNI,NOMBRE,APELLIDO,CARRERA,MATERIAS) */
void leer_alumnos_csv() {
    FILE *archivo = fopen(RUTA_CSV, "r");
    if (!archivo) {
        perror("No se pudo abrir el archivo CSV");
        return;
    }
    char linea[MAX_LINEA];
    cantidad_alumnos = 0;
    while (fgets(linea, sizeof(linea), archivo) && cantidad_alumnos < MAX_ALUMNOS) {
        t_alumno a;
        char *token = strtok(linea, ",");
        if (token) a.ID = atoi(token);

        token = strtok(NULL, ",");
        if (token) a.DNI = atoi(token);

        token = strtok(NULL, ",");
        if (token) strncpy(a.nombre, token, 29);

        token = strtok(NULL, ",");
        if (token) strncpy(a.apellido, token, 29);

        token = strtok(NULL, ",");
        if (token) strncpy(a.carrera, token, 29);

        token = strtok(NULL, ",\n");
        if (token) a.materias = atoi(token);

        alumnos[cantidad_alumnos++] = a;
    }
    fclose(archivo);
}

/* Guarda el array 'alumnos' actual en el archivo */
int guardar_alumnos_csv() {
    FILE *f = fopen(RUTA_CSV, "w");
    if (!f) {
        perror("guardar_alumnos_csv: fopen");
        return -1;
    }
    for (int i = 0; i < cantidad_alumnos; i++) {
        fprintf(f, "%d,%d,%s,%s,%s,%d\n",
                alumnos[i].ID,
                alumnos[i].DNI,
                alumnos[i].nombre,
                alumnos[i].apellido,
                alumnos[i].carrera,
                alumnos[i].materias);
    }
    fclose(f);
    return 0;
}

/* util: buscar por ID en array */
int buscar_por_ID(t_alumno arr[], int cnt, int id) {
    for (int i = 0; i < cnt; i++) if (arr[i].ID == id) return i;
    return -1;
}

/* lista el arreglo a un buffer (para enviar por socket) */
void listar_a_buffer(t_alumno arr[], int cnt, char *out, size_t out_size) {
    out[0] = '\0';
    char linea[BUFFER];
    for (int i = 0; i < cnt; i++) {
        snprintf(linea, sizeof(linea),
            "ID: %d\n- Nombre: %s\n- Apellido: %s\n- Carrera: %s\n- Materias: %d\n\n",
            arr[i].ID, arr[i].nombre, arr[i].apellido, arr[i].carrera, arr[i].materias);
        strncat(out, linea, out_size - strlen(out) - 1);
        if (strlen(out) >= out_size - 100) break; /* seguridad */
    }
    if (cnt == 0) strncat(out, "No hay alumnos registrados.\n", out_size - strlen(out) - 1);
}

/* ALTA */
int alta_en_array(t_alumno arr[], int *cnt, t_alumno a) {
    if (*cnt >= MAX_ALUMNOS) return -1;
    if (buscar_por_ID(arr, *cnt, a.ID) != -1) return -2; /* ID duplicado */
    arr[*cnt] = a;
    (*cnt)++;
    return 0;
}

/* BAJA por ID */
int baja_en_array(t_alumno arr[], int *cnt, int id) {
    int idx = buscar_por_ID(arr, *cnt, id);
    if (idx == -1) return -1;
    arr[idx] = arr[(*cnt) - 1];
    (*cnt)--;
    return 0;
}

/* MODIFICAR: campo = NOMBRE/APELLIDO/CARRERA/MATERIAS */
int modificar_en_array(t_alumno arr[], int cnt, int id, const char* campo, const char* valor) {
    int idx = buscar_por_ID(arr, cnt, id);
    if (idx == -1) return -1;
    if (strcasecmp(campo, "NOMBRE") == 0) { strncpy(arr[idx].nombre, valor, 29); arr[idx].nombre[29]=0; }
    else if (strcasecmp(campo, "APELLIDO") == 0) { strncpy(arr[idx].apellido, valor, 29); arr[idx].apellido[29]=0; }
    else if (strcasecmp(campo, "CARRERA") == 0) { strncpy(arr[idx].carrera, valor, 29); arr[idx].carrera[29]=0; }
    else if (strcasecmp(campo, "MATERIAS") == 0) arr[idx].materias = atoi(valor);
    else return -2; /* campo inválido */
    return 0;
}

/* CONSULTA simple: campo = ID/DNI/APELLIDO (igual que tu anterior) */
void consulta_por_campo(t_alumno arr[], int cnt, const char* campo, const char* valor, char* out, size_t out_size) {
    out[0] = '\0';
    char linea[BUFFER];
    for (int i = 0; i < cnt; i++) {
        int match = 0;
        if (strcasecmp(campo, "ID") == 0) {
            if (arr[i].ID == atoi(valor)) match = 1;
        } else if (strcasecmp(campo, "DNI") == 0) {
            if (arr[i].DNI == atoi(valor)) match = 1;
        } else if (strcasecmp(campo, "APELLIDO") == 0) {
            if (strcasecmp(arr[i].apellido, valor) == 0) match = 1;
        }
        if (match) {
            snprintf(linea, sizeof(linea), "%d,%d,%s,%s,%s,%d\n",
                     arr[i].ID, arr[i].DNI, arr[i].nombre,
                     arr[i].apellido, arr[i].carrera, arr[i].materias);
            strncat(out, linea, out_size - strlen(out) - 1);
        }
    }
    if (strlen(out) == 0) strncat(out, "No se encontraron registros\n", out_size - strlen(out) - 1);
}
/* FIN FUNCIONES */