#ifndef TRANSACCION_H
#define TRANSACCION_H

#include <pthread.h>

extern pthread_mutex_t mutex_transaccion;
extern int transaccion_activa;
extern int cliente_transaccion_fd;

int iniciar_transaccion(int cliente_fd);
int finalizar_transaccion(int cliente_fd);
int cancelar_transaccion(int cliente_fd);
int puede_realizar_operacion(int cliente_fd);

#endif
