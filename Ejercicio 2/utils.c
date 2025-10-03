#include "utils.h"
#include "csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <time.h>

void log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("[INFO] ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

void log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void handler_cierre(int sig) {
    printf("\n[✱] Señal %d recibida. Guardando datos...\n", sig);
    guardar_registros();
    printf("[✔] Datos guardados correctamente. Saliendo.\n");
    exit(0);
}

void configurar_signal_handler() {
    signal(SIGINT, handler_cierre);
    signal(SIGTERM, handler_cierre);
}

char* trim_nueva_linea(char* str) {
    str[strcspn(str, "\n")] = '\0';
    return str;
}
