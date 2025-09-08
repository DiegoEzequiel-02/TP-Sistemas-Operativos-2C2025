// Para ejecutar, gcc "Ejercicio1/main.c" -o Ejercicio1/alumnos && ./alumnos 5 50
//(5 generadores, 50 registros en total)
// El archivo alumnos.csv se genera en el mismo directorio del ejecutable

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>

// #define REGISTROS_POR_GENERADOR 10
// #define MAX_REGISTROS 1000
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

typedef struct
{
    int id;
    int dni;
    char nombre[32];
    char apellido[32];
    char carrera[32];
    int materias;
} Registro;

const char *nombres[] = {"Juan", "Ana", "Luis", "Maria", "Pedro"};
const char *apellidos[] = {"Perez", "Gomez", "Lopez", "Diaz", "Fernandez"};
const char *carreras[] = {"Ingenieria", "Medicina", "Derecho", "Arquitectura", "Economia"};

// Semáforo: 0 = generador puede escribir, 1 = coordinador puede leer
/*Bloquea el proceso hasta que el semáforo num del conjunto semid
tenga valor mayor a cero, y luego lo decrementa.
Se usa para sincronización entre procesos*/
void sem_wait(int semid, int num)
{
    struct sembuf op = {num, -1, 0};
    semop(semid, &op, 1);
}

/*Incrementa el semáforo num del conjunto semid,
permitiendo que otro proceso continúe.
Es la operación opuesta a sem_wait.*/
void sem_signal(int semid, int num)
{
    struct sembuf op = {num, 1, 0};
    semop(semid, &op, 1);
}

/*Llena la estructura Registro con datos aleatorios
(id, dni, nombre, apellido, carrera y materias).
Simula la generación de un registro de alumno.*/
void generar_registro(Registro *reg, int id)
{
    reg->id = id;
    reg->dni = 30000000 + rand() % 20000000;
    strcpy(reg->nombre, nombres[rand() % 5]);
    strcpy(reg->apellido, apellidos[rand() % 5]);
    strcpy(reg->carrera, carreras[rand() % 5]);
    reg->materias = rand() % 40;
}

/*Proceso hijo que genera cantidad registros. Por cada registro:

    Espera permiso del semáforo para escribir.
    Genera el registro y lo escribe en memoria compartida.
    Señala al coordinador que hay un registro listo. Al terminar, se desconecta de la memoria compartida y finaliza.
*/
void generador(int shm_id, int sem_id, int inicio_id, int cantidad)
{
    Registro *reg = (Registro *)shmat(shm_id, NULL, 0);
    srand(getpid());
    for (int i = 0; i < cantidad; i++)
    {
        sem_wait(sem_id, 0); // Espera permiso para escribir
        generar_registro(reg, inicio_id + i);
        sem_signal(sem_id, 1); // Señala que hay registro listo
    }
    shmdt(reg);
    exit(0);
}

/*Proceso hijo que recibe los registros generados:

    Abre el archivo alumnos.csv y escribe el encabezado.
    Por cada registro:
        Espera que el generador lo escriba (semáforo).
        Lo lee de memoria compartida y lo guarda en el CSV.
        Señala al generador que puede escribir el siguiente. Al terminar, cierra el archivo y la memoria compartida.
*/
void coordinador(int shm_id, int sem_id, int total_registros, int generadores)
{
    FILE *csv = fopen("./Ejercicio1/alumnos.csv", "w");
    fprintf(csv, "ID,DNI,Nombre,Apellido,Carrera,Materias\n");

    Registro *reg = (Registro *)shmat(shm_id, NULL, 0);
    int recibidos = 0;
    while (recibidos < total_registros)
    {
        sem_wait(sem_id, 1); // Espera registro listo
        fprintf(csv, "%d,%d,%s,%s,%s,%d\n",
                reg->id, reg->dni, reg->nombre, reg->apellido, reg->carrera, reg->materias);

        recibidos++;
        sem_signal(sem_id, 0); // Permite escribir al generador
    }
    shmdt(reg);
    fclose(csv);
}

/*
    Valida los argumentos (cantidad de generadores y registros).
    Crea memoria compartida y semáforos.
    Lanza el proceso coordinador y los procesos generadores.
    Espera que todos los procesos terminen.
    Libera recursos (memoria y semáforos).
    Informa que la generación terminó.
*/
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Uso: %s <generadores> <total_registros>\n", argv[0]);
        return 1;
    }
    int generadores = atoi(argv[1]);
    int total_registros = atoi(argv[2]);
    // if (generadores <= 0 || total_registros <= 0 || total_registros % generadores != 0)
    printf("Generadores: %d| Registros: %d\n", generadores, total_registros);
    if (generadores <= 0 || total_registros <= 0)
    {
        printf("Parámetros inválidos. total_registros debe ser múltiplo de generadores.\n");
        return 1;
    }

    int shm_id = shmget(SHM_KEY, sizeof(Registro), IPC_CREAT | 0666);
    int sem_id = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1); // Generador puede escribir
    semctl(sem_id, 1, SETVAL, 0); // Coordinador espera

    pid_t coord_pid = fork();
    if (coord_pid == 0)
    {
        coordinador(shm_id, sem_id, total_registros, generadores);
        exit(0);
    }

    // int registros_por_gen = total_registros / generadores;
    // int val = total_registros / generadores;
    // int registros_por_gen = (val < 10) ? val : 10;
    // int id_actual = 1;
    // for (int i = 0; i < generadores; i++) {
    //     pid_t gen_pid = fork();
    //     if (gen_pid == 0) {
    //         generador(shm_id, sem_id, id_actual, registros_por_gen);
    //     }
    //     id_actual += registros_por_gen;
    // }

    int id_actual = 1;
    int aux_total = total_registros;
    while(id_actual < total_registros){
        if(aux_total < generadores){
            for (int i = 0; i < aux_total; i++, id_actual++)
                {
                    pid_t gen_pid = fork();
                    if (gen_pid == 0)
                    {
                        generador(shm_id, sem_id, id_actual, 1);
                    }
                }
        } else {
            int val = aux_total / generadores;
            int base = (val < 10) ? val : 10;
            for (int i = 0; i < generadores; i++)
            {
                pid_t gen_pid = fork();
                if (gen_pid == 0)
                {
                    generador(shm_id, sem_id, id_actual, base);
                }
                id_actual += base;
                aux_total -= base;
            }
        }
    }
    // Caso de que falte un registro
    if(id_actual == total_registros){
        pid_t gen_pid = fork();
        if (gen_pid == 0)
            generador(shm_id, sem_id, id_actual, 1);
    }
    for (int i = 0; i < generadores + 1; i++)
        wait(NULL);

    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    printf("Generación finalizada. Ver alumnos.csv\n");
    return 0;
}