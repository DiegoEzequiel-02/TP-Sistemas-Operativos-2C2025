#ifndef UTILS_H
#define UTILS_H

void log_info(const char* fmt, ...);
void log_error(const char* fmt, ...);
void configurar_signal_handler();
char* trim_nueva_linea(char* str);

#endif
