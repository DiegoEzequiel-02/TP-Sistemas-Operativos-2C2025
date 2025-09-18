#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>

#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

typedef struct {
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

// ------------------- SEMÁFOROS --------------------
void sem_wait(int semid, int num) {
    struct sembuf op = {num, -1, 0};
    semop(semid, &op, 1);
}

void sem_signal(int semid, int num) {
    struct sembuf op = {num, 1, 0};
    semop(semid, &op, 1);
}

// ------------------- GENERADOR --------------------
void generar_registro(Registro *reg, int id) {
    reg->id = id;
    reg->dni = 30000000 + rand() % 20000000;
    strcpy(reg->nombre, nombres[rand() % 5]);
    strcpy(reg->apellido, apellidos[rand() % 5]);
    strcpy(reg->carrera, carreras[rand() % 5]);
    reg->materias = rand() % 40;
}

void generador(int shm_id, int sem_id, int inicio_id, int cantidad) {
    Registro *reg = (Registro *)shmat(shm_id, NULL, 0);
    srand(getpid());
    for (int i = 0; i < cantidad; i++) {
        sem_wait(sem_id, 0);
        generar_registro(reg, inicio_id + i);
        sem_signal(sem_id, 1);
    }
    shmdt(reg);
    exit(0);
}

// ------------------- COORDINADOR --------------------
int regUsado(int cont, int *idsUsados, int *dniUsados, Registro *reg) {
    for (int i = 0; i < cont; i++)
        if (idsUsados[i] == reg->id && dniUsados[i] == reg->dni)
            return 1;
    return 0;
}

// ------------------- GESTIÓN DE GENERADORES --------------------
int compMenorNum(int num1, int num2) {
    return (num1 < num2) ? num1 : num2;
}

int generarRegs(int shm_id, int sem_id, int regsCrear, int generadores, int* id_actual, int* total_procesos) {
    for (int i = 0; i < generadores; i++) {
        int id_inicial = *(id_actual) + i * regsCrear;
        pid_t gen_pid = fork();
        if (gen_pid < 0) {
            perror("Error en fork");
            exit(1);
        }
        if (gen_pid == 0) {
            generador(shm_id, sem_id, id_inicial, regsCrear);
        }
        (*total_procesos)++;  // sumamos cada proceso hijo generado
    }
    *(id_actual) += regsCrear * generadores;
    return regsCrear * generadores;
}

void coordinador(int shm_id, int sem_id, int total_registros) {
    int idsUsados[total_registros];
    int dniUsados[total_registros];
    int contRegs = 0;

    FILE *csv = fopen("./Ejercicio1/alumnos.csv", "w");
    if (!csv) {
        perror("Error abriendo alumnos.csv");
        exit(1);
    }

    fprintf(csv, "ID  |DNI       |Nombre         |Apellido       |Carrera        |Materias|\n");
    fflush(csv);  // aseguramos escritura inmediata

    Registro *reg = (Registro *)shmat(shm_id, NULL, 0);
    int recibidos = 0;

    while (recibidos < total_registros) {
        sem_wait(sem_id, 1);  // espera a que haya dato
        if (!regUsado(contRegs, idsUsados, dniUsados, reg)) {
            fprintf(csv, "%04d|%-10d|%-15s|%-15s|%-15s|%-8d|\n",
                    reg->id, reg->dni, reg->nombre, reg->apellido, reg->carrera, reg->materias);
            fflush(csv);  // importante si hay fallos intermedios
            idsUsados[contRegs] = reg->id;
            dniUsados[contRegs] = reg->dni;
            contRegs++;
            recibidos++;
        }
        sem_signal(sem_id, 0);  // permite que se escriba otro
    }

    shmdt(reg);
    fclose(csv);
}

// --- MAIN ---
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <generadores> <total_registros>\n", argv[0]);
        return 1;
    }

    int generadores = atoi(argv[1]);
    int total_registros = atoi(argv[2]);
    if (generadores <= 0 || total_registros <= 0) {
        printf("Parámetros inválidos.\n");
        return 1;
    }

    int shm_id = shmget(SHM_KEY, sizeof(Registro), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Error en shmget");
        exit(1);
    }

    int sem_id = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1); // generador puede escribir
    semctl(sem_id, 1, SETVAL, 0); // coordinador espera

    int total_procesos = 0; // incluye generadores y coordinador

    pid_t coord_pid = fork();
    if (coord_pid < 0) {
        perror("Error al crear coordinador");
        exit(1);
    } else if (coord_pid == 0) {
        coordinador(shm_id, sem_id, total_registros);
        exit(0);
    }
    total_procesos++;  // coordinador

    int id_actual = 1;
    while (total_registros > 0) {
        int generadores_uso = (total_registros >= generadores) ? generadores : total_registros;
        int regs_por_gen = compMenorNum(total_registros / generadores_uso, 10);
        total_registros -= generarRegs(shm_id, sem_id, regs_por_gen, generadores_uso, &id_actual, &total_procesos);
    }

    for (int i = 0; i < total_procesos; i++)
        wait(NULL);

    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
    semctl(sem_id, 1, IPC_RMID);

    printf("Generación finalizada. Ver alumnos.csv\n");
    return 0;
}
