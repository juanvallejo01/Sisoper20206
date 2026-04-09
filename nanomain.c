#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define FIFO_FILE "canal_fifo"
#define SHM_KEY 0x1234
#define BUFFER_SIZE 128
#define N_PARES 5

typedef struct {
    int a;
    int b;

    int resultado_mul;
    int resultado_sum;

    // sincronización simple (sin semáforos)
    int listo;       // padre publicó un nuevo (a,b)
    int mul_listo;   // cons1 terminó
    int sum_listo;   // cons2 terminó
    int fin;         // no hay más datos
} DatosCompartidos;

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    int shm_id;
    DatosCompartidos *mem;
    pid_t pid_prod, pid_cons1, pid_cons2;

    // Crear memoria compartida
    shm_id = shmget((key_t)SHM_KEY, sizeof(DatosCompartidos), IPC_CREAT | 0666);
    if (shm_id < 0) die("Error creando memoria compartida (shmget)");

    mem = (DatosCompartidos *)shmat(shm_id, NULL, 0);
    if (mem == (DatosCompartidos *)-1) die("Error asociando memoria compartida (shmat)");

    // Inicializar flags/variables
    mem->a = 0; mem->b = 0;
    mem->resultado_mul = 0;
    mem->resultado_sum = 0;
    mem->listo = 0;
    mem->mul_listo = 0;
    mem->sum_listo = 0;
    mem->fin = 0;

    // Crear FIFO si no existe (ignorar si ya existe)
    if (mkfifo(FIFO_FILE, 0666) < 0) {
        if (errno != EEXIST) die("Error creando FIFO (mkfifo)");
    }

    // PRODUCTOR
    pid_prod = fork();
    if (pid_prod < 0) die("Error en fork productor");

    if (pid_prod == 0) {
        int fd = open(FIFO_FILE, O_WRONLY); // se desbloquea cuando el padre abra O_RDONLY
        if (fd < 0) die("Productor: Error abriendo FIFO para escritura");

        for (int i = 1; i <= N_PARES; i++) {
            int a = i, b = i + 2;
            char buffer[BUFFER_SIZE];

            snprintf(buffer, sizeof(buffer), "%d %d\n", a, b);
            if (write(fd, buffer, strlen(buffer)) < 0) {
                die("Productor: Error escribiendo FIFO");
            }

            printf("[Productor] Enviado: %s", buffer);
            fflush(stdout);
            sleep(1);
        }

        close(fd);
        _exit(0);
    }

    // CONSUMIDOR 1 (MULTIPLICACIÓN)
    pid_cons1 = fork();
    if (pid_cons1 < 0) die("Error en fork consumidor 1");

    if (pid_cons1 == 0) {
        int *resultados = (int *)malloc(sizeof(int) * N_PARES);
        if (!resultados) die("Cons1: malloc");

        int idx = 0;

        while (1) {
            while (mem->listo == 0 && mem->fin == 0) usleep(1000);
            if (mem->fin) break;

            int a = mem->a, b = mem->b;
            int r = a * b;

            mem->resultado_mul = r;
            resultados[idx++] = r;
            mem->mul_listo = 1;

            printf("[Cons1-MUL] a=%d b=%d => %d\n", a, b, r);
            fflush(stdout);

            // esperar a que el padre baje listo (pase al siguiente)
            while (mem->listo == 1 && mem->fin == 0) usleep(1000);
        }

        printf("[Cons1-MUL] Resultados: ");
        for (int i = 0; i < idx; i++) printf("%d%s", resultados[i], (i == idx - 1) ? "" : ", ");
        printf("\n");
        fflush(stdout);

        free(resultados);
        _exit(0);
    }

    // CONSUMIDOR 2 (SUMA)
    pid_cons2 = fork();
    if (pid_cons2 < 0) die("Error en fork consumidor 2");

    if (pid_cons2 == 0) {
        int *resultados = (int *)malloc(sizeof(int) * N_PARES);
        if (!resultados) die("Cons2: malloc");

        int idx = 0;

        while (1) {
            while (mem->listo == 0 && mem->fin == 0) usleep(1000);
            if (mem->fin) break;

            int a = mem->a, b = mem->b;
            int r = a + b;

            mem->resultado_sum = r;
            resultados[idx++] = r;
            mem->sum_listo = 1;

            printf("[Cons2-SUM] a=%d b=%d => %d\n", a, b, r);
            fflush(stdout);

            while (mem->listo == 1 && mem->fin == 0) usleep(1000);
        }

        printf("[Cons2-SUM] Resultados: ");
        for (int i = 0; i < idx; i++) printf("%d%s", resultados[i], (i == idx - 1) ? "" : ", ");
        printf("\n");
        fflush(stdout);

        free(resultados);
        _exit(0);
    }

    // PADRE: lee FIFO y publica en SHM para que ambos consumidores procesen el mismo par
    int fd_r = open(FIFO_FILE, O_RDONLY);
    if (fd_r < 0) die("Padre: Error abriendo FIFO para lectura");

    FILE *fp = fdopen(fd_r, "r");
    if (!fp) die("Padre: fdopen");

    char line[BUFFER_SIZE];
    int count = 0;

    while (count < N_PARES && fgets(line, sizeof(line), fp) != NULL) {
        int a, b;
        if (sscanf(line, "%d %d", &a, &b) != 2) continue;

        // esperar que el par anterior esté consumido
        while (mem->listo == 1) usleep(1000);

        mem->a = a;
        mem->b = b;
        mem->mul_listo = 0;
        mem->sum_listo = 0;
        mem->listo = 1;

        printf("[Padre] Publicado: a=%d b=%d\n", a, b);
        fflush(stdout);

        // esperar que ambos terminen
        while (mem->mul_listo == 0 || mem->sum_listo == 0) usleep(1000);

        printf("[Padre] OK: mul=%d sum=%d\n", mem->resultado_mul, mem->resultado_sum);
        fflush(stdout);

        mem->listo = 0;
        count++;
    }

    mem->fin = 1;
    mem->listo = 0;

    fclose(fp);

    waitpid(pid_prod, NULL, 0);
    waitpid(pid_cons1, NULL, 0);
    waitpid(pid_cons2, NULL, 0);

    if (shmdt(mem) < 0) perror("Padre: shmdt");
    if (shmctl(shm_id, IPC_RMID, NULL) < 0) perror("Padre: shmctl IPC_RMID");
    if (unlink(FIFO_FILE) < 0) perror("Padre: unlink FIFO");

    printf("[Padre] Fin. Pares procesados: %d\n", count);
    return 0;
}