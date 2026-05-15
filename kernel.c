/* kernel.c
   Núcleo coordinador del sistema operativo simulado
   Compilar:  gcc -std=c89 -Wall files.c kernel.c -o kernel
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─────────────────────────────────────────
   Prototipos del módulo files.c
   ───────────────────────────────────────── */
void init_filesystem(void);
void listar_archivos(void);
void crear_archivo(char nombre[]);
void abrir_archivo(char nombre[]);
void escribir_archivo(char nombre[], char texto[]);
void leer_archivo(char nombre[]);
void cerrar_archivo(char nombre[]);
void eliminar_archivo(char nombre[]);

/* ─────────────────────────────────────────
   Módulo de Memoria (simulado)
   ───────────────────────────────────────── */
#define MEM_TOTAL 1024

static int mem_disponible;

void init_memoria(void)
{
    mem_disponible = MEM_TOTAL;
    printf("Modulo de memoria iniciado (%d bytes disponibles).\n", MEM_TOTAL);
}

/* ─────────────────────────────────────────
   Módulo de Procesos (simulado)
   ───────────────────────────────────────── */
#define MAX_PROC         8
#define ESTADO_LIBRE     0
#define ESTADO_CREADO    1
#define ESTADO_CORRIENDO 2
#define ESTADO_FINALIZADO 3

typedef struct {
    int  pid;
    char nombre[32];
    int  estado;
    int  mem_asignada;
} PCB;

static PCB tabla_proc[MAX_PROC];
static int next_pid;

void init_procesos(void)
{
    int i;
    for (i = 0; i < MAX_PROC; i++)
        tabla_proc[i].estado = ESTADO_LIBRE;
    next_pid = 1;
    printf("Modulo de procesos iniciado (%d ranuras).\n", MAX_PROC);
}

static void crear_proceso(void)
{
    int i;
    char nombre[32];
    printf("Nombre del proceso: ");
    fgets(nombre, sizeof(nombre), stdin);
    nombre[strcspn(nombre, "\n")] = '\0';

    for (i = 0; i < MAX_PROC; i++) {
        if (tabla_proc[i].estado == ESTADO_LIBRE) {
            tabla_proc[i].pid = next_pid++;
            strncpy(tabla_proc[i].nombre, nombre, 31);
            tabla_proc[i].nombre[31]  = '\0';
            tabla_proc[i].estado      = ESTADO_CREADO;
            tabla_proc[i].mem_asignada = 0;
            printf("Proceso '%s' creado con PID %d.\n", nombre, tabla_proc[i].pid);
            return;
        }
    }
    printf("Error: tabla de procesos llena (max %d).\n", MAX_PROC);
}

static void asignar_memoria_proceso(void)
{
    int pid, mem, i;
    printf("PID del proceso: ");
    if (scanf("%d", &pid) != 1) { getchar(); return; }
    printf("Bytes a asignar: ");
    if (scanf("%d", &mem) != 1) { getchar(); return; }
    getchar();

    if (mem <= 0 || mem > mem_disponible) {
        printf("Error: memoria insuficiente o cantidad inválida.\n");
        return;
    }
    for (i = 0; i < MAX_PROC; i++) {
        if (tabla_proc[i].pid == pid && tabla_proc[i].estado != ESTADO_LIBRE) {
            tabla_proc[i].mem_asignada += mem;
            tabla_proc[i].estado        = ESTADO_CORRIENDO;
            mem_disponible             -= mem;
            printf("Asignados %d bytes al PID %d. Memoria libre: %d bytes.\n",
                   mem, pid, mem_disponible);
            return;
        }
    }
    printf("Error: PID %d no encontrado.\n", pid);
}

static void liberar_proceso(void)
{
    int pid, i;
    printf("PID a finalizar: ");
    if (scanf("%d", &pid) != 1) { getchar(); return; }
    getchar();

    for (i = 0; i < MAX_PROC; i++) {
        if (tabla_proc[i].pid == pid && tabla_proc[i].estado != ESTADO_LIBRE) {
            mem_disponible            += tabla_proc[i].mem_asignada;
            tabla_proc[i].mem_asignada = 0;
            tabla_proc[i].estado       = ESTADO_LIBRE;
            printf("PID %d finalizado. Recursos liberados. Memoria libre: %d bytes.\n",
                   pid, mem_disponible);
            return;
        }
    }
    printf("Error: PID %d no encontrado.\n", pid);
}

/* ─────────────────────────────────────────
   Estado global del sistema
   ───────────────────────────────────────── */
static void estado_global(void)
{
    static const char *estados[] = { "LIBRE", "CREADO", "CORRIENDO", "FINALIZADO" };
    int i, activos = 0;

    printf("\n========== ESTADO GLOBAL DEL SISTEMA ==========\n");

    printf("\n-- Memoria --\n");
    printf("  Total   : %d bytes\n", MEM_TOTAL);
    printf("  En uso  : %d bytes\n", MEM_TOTAL - mem_disponible);
    printf("  Libre   : %d bytes\n", mem_disponible);

    printf("\n-- Procesos --\n");
    printf("  %-5s  %-20s  %-12s  %s\n", "PID", "Nombre", "Estado", "Memoria");
    printf("  %-5s  %-20s  %-12s  %s\n", "---", "------", "------", "-------");
    for (i = 0; i < MAX_PROC; i++) {
        if (tabla_proc[i].estado != ESTADO_LIBRE) {
            printf("  %-5d  %-20s  %-12s  %d bytes\n",
                   tabla_proc[i].pid,
                   tabla_proc[i].nombre,
                   estados[tabla_proc[i].estado],
                   tabla_proc[i].mem_asignada);
            activos++;
        }
    }
    if (!activos) printf("  (sin procesos activos)\n");

    printf("\n-- Archivos --\n");
    listar_archivos();

    printf("================================================\n");
}

/* ─────────────────────────────────────────
   Menú principal
   ───────────────────────────────────────── */
static void mostrar_menu(void)
{
    printf("\n============= Mini Kernel =============\n");
    printf("  --- Sistema de Archivos ---\n");
    printf("  1.  Crear archivo\n");
    printf("  2.  Abrir archivo\n");
    printf("  3.  Leer archivo\n");
    printf("  4.  Escribir en archivo\n");
    printf("  5.  Cerrar archivo\n");
    printf("  6.  Eliminar archivo\n");
    printf("  7.  Listar archivos\n");
    printf("  --- Procesos y Memoria ---\n");
    printf("  8.  Crear proceso\n");
    printf("  9.  Asignar memoria a proceso\n");
    printf("  10. Liberar proceso\n");
    printf("  --- Sistema ---\n");
    printf("  11. Estado global del sistema\n");
    printf("  12. Salir\n");
    printf("=======================================\n");
    printf("Seleccione: ");
}

/* ─────────────────────────────────────────
   Auxiliar: leer línea y limpiar '\n'
   ───────────────────────────────────────── */
static void leer_linea(char *buf, int sz)
{
    fgets(buf, sz, stdin);
    buf[strcspn(buf, "\n")] = '\0';
}

/* ─────────────────────────────────────────
   Punto de entrada
   ───────────────────────────────────────── */
int main(void)
{
    int  opcion;
    char nombre[50];
    char texto[256];

    printf("Iniciando sistema operativo simulado...\n");
    init_memoria();
    init_procesos();
    init_filesystem();
    printf("\nSistema listo.\n");

    do {
        mostrar_menu();
        if (scanf("%d", &opcion) != 1) {
            getchar();
            opcion = 0;
            printf("Entrada no válida.\n");
            continue;
        }
        getchar(); /* consume '\n' */

        switch (opcion) {
            case 1:
                printf("Nombre del archivo: ");
                leer_linea(nombre, sizeof(nombre));
                crear_archivo(nombre);
                break;

            case 2:
                printf("Nombre del archivo: ");
                leer_linea(nombre, sizeof(nombre));
                abrir_archivo(nombre);
                break;

            case 3:
                printf("Nombre del archivo: ");
                leer_linea(nombre, sizeof(nombre));
                leer_archivo(nombre);
                break;

            case 4:
                printf("Nombre del archivo: ");
                leer_linea(nombre, sizeof(nombre));
                printf("Texto a escribir: ");
                leer_linea(texto, sizeof(texto));
                escribir_archivo(nombre, texto);
                break;

            case 5:
                printf("Nombre del archivo: ");
                leer_linea(nombre, sizeof(nombre));
                cerrar_archivo(nombre);
                break;

            case 6:
                printf("Nombre del archivo: ");
                leer_linea(nombre, sizeof(nombre));
                eliminar_archivo(nombre);
                break;

            case 7:
                listar_archivos();
                break;

            case 8:
                crear_proceso();
                break;

            case 9:
                asignar_memoria_proceso();
                break;

            case 10:
                liberar_proceso();
                break;

            case 11:
                estado_global();
                break;

            case 12:
                printf("Apagando sistema... hasta luego.\n");
                break;

            default:
                printf("Opción inválida. Intente de nuevo.\n");
        }

    } while (opcion != 12);

    return 0;
}
