/**
 * processes.c
 * Universidad Santiago de Cali, 2026A         
 * Laboratorio #2 Sistemas Operativos - Planificación y Representación de Procesos
 * Basado en algoritmos: Round Robin, FCFS, Prioridad simple, esqueleto CFS-like.
 * COMPILAR: gcc -Wall -std=c99 processes.c -o scheduler_lab
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PROCESOS 10
#define QUANTUM 2      // para Round Robin

/* ------------------- 1. DEFINICIÓN DEL PCB ------------------- */
typedef enum { NUEVO, LISTO, EJECUCION, BLOQUEADO, TERMINADO } EstadoProceso;

typedef struct {
    int pid;
    char nombre[32];
    int prioridad;            // numérica: menor valor = mayor prioridad (para planificación prioritaria)
    int rafaga_total;         // tiempo total de CPU necesario
    int rafaga_restante;      // para SRTF o contabilidad
    int tiempo_llegada;
    int tiempo_respuesta;     // para estadísticas
    EstadoProceso estado;
    /* Datos específicos para CFS (simulación) */
    long vruntime;            // tiempo virtual (simulado)
    int nice;                 // rango -20..19 (más bajo = más prioridad en CFS)
    /* imagen del proceso: simulación de contexto */
    int *contexto_registros;  // array simple (simula guardado de contexto)
} PCB;

/* Cola de listos (FIFO para RR y FCFS, ordenable por prioridad/vruntime) */
typedef struct ColaListos {
    PCB* procesos[MAX_PROCESOS];
    int frente, final;
    int cantidad;
} ReadyQueue;

/* ------------------- 2. FUNCIONES AUXILIARES ------------------- */
void init_ready_queue(ReadyQueue *q) {
    q->frente = 0; q->final = 0; q->cantidad = 0;
    memset(q->procesos, 0, sizeof(q->procesos));
}

bool queue_vacia(ReadyQueue *q) { return q->cantidad == 0; }
bool queue_llena(ReadyQueue *q) { return q->cantidad == MAX_PROCESOS; }

void encolar(ReadyQueue *q, PCB *p) {
    if (queue_llena(q)) return;
    q->procesos[q->final] = p;
    q->final = (q->final + 1) % MAX_PROCESOS;
    q->cantidad++;
}

PCB* desencolar(ReadyQueue *q) {
    if (queue_vacia(q)) return NULL;
    PCB *p = q->procesos[q->frente];
    q->frente = (q->frente + 1) % MAX_PROCESOS;
    q->cantidad--;
    return p;
}

/* Para FIFO / Round Robin se usa orden FIFO normal */
/* Para SPN (SJF) y prioridad se requiere ordenación (implementación simple) */

/* ------------------- 3. ALGORITMOS DE PLANIFICACIÓN (TEMPLATES) ------------------- */

// A) FIFO (FCFS) - No preventivo
void planificar_fcfs(ReadyQueue *q) {
    printf("\n=== PLANIFICACIÓN FCFS ===\n");
    while (!queue_vacia(q)) {
        PCB *actual = desencolar(q);
        printf("Ejecutando proceso %s (PID %d) | ráfaga %d\n", actual->nombre, actual->pid, actual->rafaga_total);
        // Simula ejecución consumiendo rafaga_total
        actual->estado = TERMINADO;
        // TODO: calcular tiempo de retorno y espera
    }
}

// B) Round Robin ("preemptivo", apropiativo con quantum fijo)
void planificar_rr(ReadyQueue *q, int quantum) {
    printf("\n=== ROUND ROBIN (quantum=%d) ===\n", quantum);
    ReadyQueue cola_tmp;
    init_ready_queue(&cola_tmp);
    // copiar procesos a cola temporal
    while (!queue_vacia(q)) {
        encolar(&cola_tmp, desencolar(q));
    }
    int tiempo_global = 0;
    while (!queue_vacia(&cola_tmp)) {
        PCB *actual = desencolar(&cola_tmp);
        if (actual->rafaga_restante == 0) actual->rafaga_restante = actual->rafaga_total;
        printf("[t=%d] Ejecuta %s (restante=%d)\n", tiempo_global, actual->nombre, actual->rafaga_restante);
        if (actual->rafaga_restante <= quantum) {
            tiempo_global += actual->rafaga_restante;
            actual->rafaga_restante = 0;
            actual->estado = TERMINADO;
            printf("  -> %s terminado.\n", actual->nombre);
        } else {
            tiempo_global += quantum;
            actual->rafaga_restante -= quantum;
            encolar(&cola_tmp, actual);  // vuelve al final
        }
    }
    // TODO: completar estadísticas y permitir llegada de procesos en tiempo real.
}

// C) Shortest Process Next (SPN) no preventivo: ordenar por ráfaga total
int cmp_rafaga(const void *a, const void *b) {
    PCB *pa = *(PCB**)a;
    PCB *pb = *(PCB**)b;
    return (pa->rafaga_total - pb->rafaga_total);
}

void planificar_spn(ReadyQueue *q) {
    printf("\n=== SPN (SJF no preventivo) ===\n");
    // Convertir cola a array temporal
    PCB *lista[MAX_PROCESOS];
    int count = 0;
    while (!queue_vacia(q)) {
        lista[count++] = desencolar(q);
    }
    qsort(lista, count, sizeof(PCB*), cmp_rafaga);
    for (int i = 0; i < count; i++) {
        printf("Ejecutando %s (ráfaga=%d)\n", lista[i]->nombre, lista[i]->rafaga_total);
        lista[i]->estado = TERMINADO;
    }
    // TODO: Reconstruir estadísticas
}

// D) Plantilla para simulación de CFS usando vruntime y árbol (simulación lineal)
void planificar_cfs_simple(ReadyQueue *q) {
    printf("\n=== CFS SIMULADO (basado en vruntime mínimo) ===\n");
    PCB *procs[MAX_PROCESOS];
    int n = 0;
    while (!queue_vacia(q)) procs[n++] = desencolar(q);
    // Ordenar por vruntime (menor primero)
    for (int i = 0; i < n-1; i++) {
        for (int j = i+1; j < n; j++) {
            if (procs[i]->vruntime > procs[j]->vruntime) {
                PCB *tmp = procs[i];
                procs[i] = procs[j];
                procs[j] = tmp;
            }
        }
    }
    // Simulación: cada proceso recibe una tajada proporcional a su nice (factor ficticio)
    for (int i = 0; i < n; i++) {
        int slice = 2; // base
        if (procs[i]->nice < 0) slice = 4;   // más prioridad -> más tiempo
        else if (procs[i]->nice > 0) slice = 1;
        printf("Ejecuta %s (nice=%d, vruntime=%ld) slice=%d\n", procs[i]->nombre, procs[i]->nice, procs[i]->vruntime, slice);
        procs[i]->vruntime += slice;  // incrementa vruntime según tiempo ejecutado
        procs[i]->rafaga_restante -= slice;
        if (procs[i]->rafaga_restante <= 0) procs[i]->estado = TERMINADO;
        else encolar(q, procs[i]);   // realimentar
    }
    /* 
     * NOTA: En CFS real se usa un árbol rojo-negro para seleccionar el mínimo vruntime O(log n).
     * El estudiante debe implementar una estructura RBT o usar una cola de prioridad basada en heap.
     * El nice afecta el incremento de vruntime: delta = tiempo_ejecucion * (NICE_0_WEIGHT / peso(nice))
     */
}

// ------------------- 4. FUNCIÓN PARA CREAR PCB DE PRUEBA -------------------
PCB* crear_proceso(int pid, const char *nombre, int rafaga, int nice_val, int prioridad) {
    PCB *nuevo = (PCB*)malloc(sizeof(PCB));
    nuevo->pid = pid;
    strcpy(nuevo->nombre, nombre);
    nuevo->rafaga_total = rafaga;
    nuevo->rafaga_restante = rafaga;
    nuevo->prioridad = prioridad;
    nuevo->nice = nice_val;
    nuevo->vruntime = 0;
    nuevo->estado = NUEVO;
    nuevo->contexto_registros = NULL;  // no implementado
    return nuevo;
}

void liberar_pcb(PCB *p) { free(p); }

// ------------------- MAIN DE PRUEBAS -------------------
int main() {
    ReadyQueue cola;
    init_ready_queue(&cola);
    
    // Procesos de ejemplo (pid, nombre, ráfaga, nice, prioridad)
    encolar(&cola, crear_proceso(1, "EditorTexto", 8, 0, 3));
    encolar(&cola, crear_proceso(2, "Compilador", 4, -5, 1));
    encolar(&cola, crear_proceso(3, "Navegador", 12, 5, 4));
    encolar(&cola, crear_proceso(4, "Servidor", 6, -2, 2));
    
    // Seleccione algoritmo descomentando:
    // planificar_fcfs(&cola); 
    // Reinicializar cola para cada prueba (en laboratorio hacer copias)
    
    printf("\n👉 LABORATORIO: Completar implementación de:\n");
    printf("   - Colas multinivel (Feedback)\n");
    printf("   - Algoritmo del banquero (evitación)\n");
    printf("   - Detección de deadlocks con matriz de asignación\n");
    printf("   - Mejora del CFS con árbol Rojo-Negro real (inserción/extracción min)\n");
    printf("   - Cálculo de métricas: turnaround, waiting time, response time.\n");
    
    // Limpiar (evitar leaks en implementación final)
    // En un entorno real se libera toda la cola.
    return 0;