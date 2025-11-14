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
#include <errno.h>
#include <signal.h> 

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
static int g_shm_id = -1;
static int g_sem_id = -1;

/* handler simple que marca IPC para eliminación y sale */
static void cleanup_and_exit(int sig)
{
    (void)sig;
    if (g_shm_id != -1) {
        if (shmctl(g_shm_id, IPC_RMID, NULL) == -1)
            perror("shmctl IPC_RMID");
        else
            fprintf(stderr, "shmctl: marcado shm %d para eliminación\n", g_shm_id);
    }
    if (g_sem_id != -1) {
        if (semctl(g_sem_id, 0, IPC_RMID) == -1)
            perror("semctl IPC_RMID");
        else
            fprintf(stderr, "semctl: eliminado sem %d\n", g_sem_id);
    }
    _exit(1);
}

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
    // shmat coloca la memoria compartida en la memoria del proceso 
    // para poder leer/escribir la estructura Registro que luego lee el coordinador.
    if (reg == (void *)-1) { perror("shmat generador"); exit(1); }
    srand(getpid());
    for (int i = 0; i < cantidad; i++)
    {
        sem_wait(sem_id, 0); // Espera permiso para escribir
        generar_registro(reg, inicio_id + i);
        sem_signal(sem_id, 1); // Señala que hay registro listo

        usleep(100000 + (rand() % 5000000)); // 0.1s a 5s
    }
    shmdt(reg);
    //generador y coordinador llaman shmdt() cuando terminan de 
    //usar la memoria compartida para liberar el mapping antes de exit().
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
    FILE *csv = fopen("alumnos.csv", "w"); /* archivo en el cwd */
    if (!csv) { perror("fopen alumnos.csv"); exit(1); }
    fprintf(csv, "ID,DNI,Nombre,Apellido,Carrera,Materias\n");

    Registro *reg = (Registro *)shmat(shm_id, NULL, 0);
    if (reg == (void *)-1) { perror("shmat coordinador"); fclose(csv); exit(1); }

    int recibidos = 0;
    (void)generadores;
    while (recibidos < total_registros)
    {
        sem_wait(sem_id, 1); // Espera registro listo
        fprintf(csv, "%d,%d,%s,%s,%s,%d\n",
                reg->id, reg->dni, reg->nombre, reg->apellido, reg->carrera, reg->materias);
        /* Forzar flush al disco para que tail -f / ls muestren cambios */
        fflush(csv);
        fsync(fileno(csv));

        recibidos++;
        sem_signal(sem_id, 0); // Permite escribir al generador

        /* opcional: breve sleep para visualizar consumo en htop/vmstat */
        usleep(50000); // 50 ms
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

    printf("Generadores: %d | Registros: %d\n", generadores, total_registros);
    if (generadores <= 0 || total_registros <= 0)
    {
        printf("Parámetros inválidos. total_registros debe ser múltiplo de generadores.\n");
        return 1;
    }

    int shm_id = shmget(SHM_KEY, sizeof(Registro), IPC_CREAT | 0666);
    if (shm_id == -1) { perror("shmget"); return 1; }
    /*
    Crea/abre un segmento de memoria compartida con clave SHM_KEY y tamaño sizeof(Registro).
    Flags: IPC_CREAT -> crear si no existe; 0666 -> permisos lectura/escritura para user/group/other.
    Devuelve un identificador (shmid) >= 0 o -1 en error.*/

    int sem_id = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    if (sem_id == -1) { perror("semget"); shmctl(shm_id, IPC_RMID, NULL); return 1; }
    /*Crea (o abre) un conjunto de 2 semáforos identificado por SEM_KEY.
    El segundo argumento (=2) indica que el conjunto tendrá dos semáforos (índices 0 y 1).
    Flags iguales a shmget (crear si hace falta, permisos 0666).
    Devuelve el id del conjunto de semáforos o -1 en error.*/

    g_shm_id = shm_id;
    g_sem_id = sem_id;
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);

/* union semun requerido por semctl */
    union semun {
        int val; // usado para SETVAL/GETVAL (valor de un semáforo)
        struct semid_ds *buf; // usado con IPC_STAT / IPC_SET para info del conjunto
        unsigned short *array; // usado con GETALL / SETALL (valores de todos los semáforos)
        struct seminfo *__buf; // usado por IPC_INFO / SEM_INFO (raro, implementación/so específica)
    } su; //declara una variable llamada su de ese tipo; 
    //el código la usa como contenedor para pasar parámetros a semctl 
    //(por ejemplo su.val = 1; semctl(sem_id, 0, SETVAL, su);).

    su.val = 1;
    semctl(sem_id, 0, SETVAL, su); // Generador puede escribir
    su.val = 0;
    semctl(sem_id, 1, SETVAL, su); // Coordinador espera

    pid_t coord_pid = fork();
    if (coord_pid == 0)
    {
        coordinador(shm_id, sem_id, total_registros, generadores);
        exit(0);
    }

    int registros_por_gen = total_registros / generadores;
    int resto = total_registros % generadores;
    int id_actual = 1;

    for (int i = 0; i < generadores; i++) {
        int cantidad = registros_por_gen + (i < resto ? 1 : 0);
        pid_t gen_pid = fork();
        if (gen_pid == 0) {
            generador(shm_id, sem_id, id_actual, cantidad);
        }
        id_actual += cantidad;
    }

    for (int i = 0; i < generadores + 1; i++)
        wait(NULL);

    shmctl(shm_id, IPC_RMID, NULL);
    /*
    Si hay procesos aún adjuntos, el segmento se eliminará 
    cuando el último proceso haga shmdt; si no hay adjuntos, 
    se elimina de inmediato.
    */
    semctl(sem_id, 0, IPC_RMID);
    /*
    Elimina el conjunto de semáforos identificado por sem_id 
    (libera el recurso en el kernel).
    */

    printf("Generación finalizada. Ver alumnos.csv\n");
    return 0;
}