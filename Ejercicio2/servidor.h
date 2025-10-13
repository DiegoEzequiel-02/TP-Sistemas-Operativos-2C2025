#ifndef SERVIDOR_H
#define SERVIDOR_H

#include "csv.h"

typedef struct {
    int cliente_fd;
    int en_transaccion;
    Registro registros_locales[MAX_REGISTROS];
    int total_locales;
} TransaccionCliente;

// Prototipos
void* manejar_cliente(void* arg);
void configurar_servidor(int argc, char* argv[]);
void finalizar_cliente(TransaccionCliente* tc);
int confirmar_transaccion(int cliente_fd, Registro* registros_locales, int total_locales);

#endif
