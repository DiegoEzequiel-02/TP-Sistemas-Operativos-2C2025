#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>

#define REGISTROS_POR_GENERADOR 10
#define MAX_REGISTROS 1000
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

typedef struct {
    int id;
    int dni;
    char nombre[32];
    char apellido[32];
    char carrera[32];
    int materias;
    int listo; // 1 si el registro está listo para ser leído
} Registro;

const char *nombres[] = {"Juan", "Ana", "Luis", "Maria", "Pedro"};
const char *apellidos[] = {"Perez", "Gomez", "Lopez", "Diaz", "Fernandez"};
const char *carreras[] = {"Ingenieria", "Medicina", "Derecho", "Arquitectura", "Economia"};

// Semáforo: 0 = generador puede escribir, 1 = coordinador puede leer
void sem_wait(int semid, int num) {
    struct sembuf op = {num, -1, 0};
    semop(semid, &op, 1);
}
void sem_signal(int semid, int num) {
    struct sembuf op = {num, 1, 0};
    semop(semid, &op, 1);
}

void generar_registro(Registro *reg, int id) {
    reg->id = id;
    reg->dni = 30000000 + rand() % 20000000;
    strcpy(reg->nombre, nombres[rand() % 5]);
    strcpy(reg->apellido, apellidos[rand() % 5]);
    strcpy(reg->carrera, carreras[rand() % 5]);
    reg->materias = rand() % 40;
    reg->listo = 1;
}

void generador(int shm_id, int sem_id, int inicio_id, int cantidad) {
    Registro *reg = (Registro *)shmat(shm_id, NULL, 0);
    srand(getpid());
    for (int i = 0; i < cantidad; i++) {
        sem_wait(sem_id, 0); // Espera permiso para escribir
        generar_registro(reg, inicio_id + i);
        sem_signal(sem_id, 1); // Señala que hay registro listo
    }
    shmdt(reg);
    exit(0);
}

void coordinador(int shm_id, int sem_id, int total_registros, int generadores) {
    FILE *csv = fopen("alumnos.csv", "w");
    fprintf(csv, "ID,DNI,Nombre,Apellido,Carrera,Materias\n");

    Registro *reg = (Registro *)shmat(shm_id, NULL, 0);
    int recibidos = 0;
    while (recibidos < total_registros) {
        sem_wait(sem_id, 1); // Espera registro listo
        if (reg->listo) {
            fprintf(csv, "%d,%d,%s,%s,%s,%d\n",
                reg->id, reg->dni, reg->nombre, reg->apellido, reg->carrera, reg->materias);
            reg->listo = 0;
            recibidos++;
        }
        sem_signal(sem_id, 0); // Permite escribir al generador
    }
    shmdt(reg);
    fclose(csv);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <generadores> <total_registros>\n", argv[0]);
        return 1;
    }
    int generadores = atoi(argv[1]);
    int total_registros = atoi(argv[2]);
    if (generadores <= 0 || total_registros <= 0 || total_registros % generadores != 0) {
        printf("Parámetros inválidos. total_registros debe ser múltiplo de generadores.\n");
        return 1;
    }

    int shm_id = shmget(SHM_KEY, sizeof(Registro), IPC_CREAT | 0666);
    int sem_id = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1); // Generador puede escribir
    semctl(sem_id, 1, SETVAL, 0); // Coordinador espera

    pid_t coord_pid = fork();
    if (coord_pid == 0) {
        coordinador(shm_id, sem_id, total_registros, generadores);
        exit(0);
    }

    int registros_por_gen = total_registros / generadores;
    int id_actual = 1;
    for (int i = 0; i < generadores; i++) {
        pid_t gen_pid = fork();
        if (gen_pid == 0) {
            generador(shm_id, sem_id, id_actual, registros_por_gen);
        }
        id_actual += registros_por_gen;
    }

    for (int i = 0; i < generadores + 1; i++) wait(NULL);

    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    printf("Generación finalizada. Ver alumnos.csv\n");
    return 0;
}