#ifndef TRANSACCION_H
#define TRANSACCION_H

#include "csv.h"  // Para definir Registro
#include <pthread.h>

extern pthread_mutex_t mutex_transaccion;
extern int transaccion_activa;
extern int cliente_transaccion_fd;

int iniciar_transaccion(int cliente_fd);
int finalizar_transaccion(int cliente_fd);
int cancelar_transaccion(int cliente_fd);
int puede_realizar_operacion(int cliente_fd);

int confirmar_transaccion(int cliente_fd, Registro* registros_locales, int total_locales);

#endif
