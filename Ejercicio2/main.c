#include <stdio.h>
#include "csv.h"
#include "transaccion.h"
#include "servidor.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    cargar_registros();
    configurar_signal_handler();
    configurar_servidor(argc, argv);
    return 0;
}
