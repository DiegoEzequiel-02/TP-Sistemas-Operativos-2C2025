#ifndef CSV_H
#define CSV_H

char* consultar_registro(int id);
int agregar_registro(const char* linea);
int eliminar_registro(int id);
int modificar_registro(int id, const char* nuevos_datos);

void cargar_registros();
void guardar_registros();

#endif
