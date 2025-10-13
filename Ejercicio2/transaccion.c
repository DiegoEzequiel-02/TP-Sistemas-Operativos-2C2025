#include "transaccion.h"
#include <stdio.h>

pthread_mutex_t mutex_transaccion = PTHREAD_MUTEX_INITIALIZER;
int transaccion_activa = 0;
int cliente_transaccion_fd = -1;

int iniciar_transaccion(int cliente_fd) {
    pthread_mutex_lock(&mutex_transaccion);
    if (transaccion_activa) {
        pthread_mutex_unlock(&mutex_transaccion);
        return 0;
    }

    transaccion_activa = 1;
    cliente_transaccion_fd = cliente_fd;
    pthread_mutex_unlock(&mutex_transaccion);
    return 1;
}

int finalizar_transaccion(int cliente_fd) {
    pthread_mutex_lock(&mutex_transaccion);
    if (transaccion_activa && cliente_transaccion_fd == cliente_fd) {
        transaccion_activa = 0;
        cliente_transaccion_fd = -1;
        pthread_mutex_unlock(&mutex_transaccion);
        return 1;
    }
    pthread_mutex_unlock(&mutex_transaccion);
    return 0;
}

int cancelar_transaccion(int cliente_fd) {
    return finalizar_transaccion(cliente_fd);
}

int puede_realizar_operacion(int cliente_fd) {
    pthread_mutex_lock(&mutex_transaccion);
    int permitido = (!transaccion_activa || cliente_transaccion_fd == cliente_fd);
    pthread_mutex_unlock(&mutex_transaccion);
    return permitido;
}

int confirmar_transaccion(int cliente_fd, Registro* registros_locales, int total_locales) {
    pthread_mutex_lock(&mutex_transaccion);
    if (!transaccion_activa || cliente_transaccion_fd != cliente_fd) {
        pthread_mutex_unlock(&mutex_transaccion);
        return 0;
    }

    // Aquí deberías implementar la lógica para guardar registros locales
    // Por ahora, solo finalizamos la transacción

    transaccion_activa = 0;
    cliente_transaccion_fd = -1;
    pthread_mutex_unlock(&mutex_transaccion);
    return 1;
}
