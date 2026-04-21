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
    int tiempo_espera;        // tiempo total en cola
    int tiempo_retorno;       // tiempo desde llegada hasta terminación
    int tiempo_primera_ejecucion; // para calcular respuesta
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

// Estructuras para Árbol Rojo-Negro para CFS
typedef enum { RED, BLACK } Color;

typedef struct RBNode {
    PCB *process;
    struct RBNode *left, *right, *parent;
    Color color;
} RBNode;

typedef struct {
    RBNode *root;
    RBNode *nil; // nodo sentinela
} RBTree;

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

/* Funciones para Árbol Rojo-Negro */
RBNode* create_node(PCB *p, RBNode *nil) {
    RBNode *node = (RBNode*)malloc(sizeof(RBNode));
    node->process = p;
    node->left = nil;
    node->right = nil;
    node->parent = nil;
    node->color = RED;
    return node;
}

void init_rb_tree(RBTree *tree) {
    tree->nil = (RBNode*)malloc(sizeof(RBNode));
    tree->nil->color = BLACK;
    tree->nil->left = tree->nil->right = tree->nil->parent = tree->nil;
    tree->root = tree->nil;
}

void left_rotate(RBTree *tree, RBNode *x) {
    RBNode *y = x->right;
    x->right = y->left;
    if (y->left != tree->nil) y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == tree->nil) tree->root = y;
    else if (x == x->parent->left) x->parent->left = y;
    else x->parent->right = y;
    y->left = x;
    x->parent = y;
}

void right_rotate(RBTree *tree, RBNode *x) {
    RBNode *y = x->left;
    x->left = y->right;
    if (y->right != tree->nil) y->right->parent = x;
    y->parent = x->parent;
    if (x->parent == tree->nil) tree->root = y;
    else if (x == x->parent->right) x->parent->right = y;
    else x->parent->left = y;
    y->right = x;
    x->parent = y;
}

void rb_insert_fixup(RBTree *tree, RBNode *z) {
    while (z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            RBNode *y = z->parent->parent->right;
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                right_rotate(tree, z->parent->parent);
            }
        } else {
            RBNode *y = z->parent->parent->left;
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                left_rotate(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK;
}

void rb_insert(RBTree *tree, PCB *p) {
    RBNode *z = create_node(p, tree->nil);
    RBNode *y = tree->nil;
    RBNode *x = tree->root;
    while (x != tree->nil) {
        y = x;
        if (z->process->vruntime < x->process->vruntime) x = x->left;
        else x = x->right;
    }
    z->parent = y;
    if (y == tree->nil) tree->root = z;
    else if (z->process->vruntime < y->process->vruntime) y->left = z;
    else y->right = z;
    rb_insert_fixup(tree, z);
}

RBNode* tree_minimum(RBTree *tree, RBNode *x) {
    while (x->left != tree->nil) x = x->left;
    return x;
}

void rb_delete_fixup(RBTree *tree, RBNode *x) {
    while (x != tree->root && x->color == BLACK) {
        if (x == x->parent->left) {
            RBNode *w = x->parent->right;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                left_rotate(tree, x->parent);
                w = x->parent->right;
            }
            if (w->left->color == BLACK && w->right->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->right->color == BLACK) {
                    w->left->color = BLACK;
                    w->color = RED;
                    right_rotate(tree, w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                left_rotate(tree, x->parent);
                x = tree->root;
            }
        } else {
            RBNode *w = x->parent->left;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                right_rotate(tree, x->parent);
                w = x->parent->left;
            }
            if (w->right->color == BLACK && w->left->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left->color == BLACK) {
                    w->right->color = BLACK;
                    w->color = RED;
                    left_rotate(tree, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                right_rotate(tree, x->parent);
                x = tree->root;
            }
        }
    }
    x->color = BLACK;
}

void rb_delete(RBTree *tree, RBNode *z) {
    RBNode *y = z;
    RBNode *x;
    Color y_original_color = y->color;
    if (z->left == tree->nil) {
        x = z->right;
        if (z->parent == tree->nil) tree->root = x;
        else if (z == z->parent->left) z->parent->left = x;
        else z->parent->right = x;
        x->parent = z->parent;
    } else if (z->right == tree->nil) {
        x = z->left;
        if (z->parent == tree->nil) tree->root = x;
        else if (z == z->parent->left) z->parent->left = x;
        else z->parent->right = x;
        x->parent = z->parent;
    } else {
        y = tree_minimum(tree, z->right);
        y_original_color = y->color;
        x = y->right;
        if (y->parent == z) x->parent = y;
        else {
            if (y->parent == tree->nil) tree->root = x;
            else if (y == y->parent->left) y->parent->left = x;
            else y->parent->right = x;
            x->parent = y->parent;
            y->right = z->right;
            y->right->parent = y;
        }
        if (z->parent == tree->nil) tree->root = y;
        else if (z == z->parent->left) z->parent->left = y;
        else z->parent->right = y;
        y->parent = z->parent;
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }
    if (y_original_color == BLACK) rb_delete_fixup(tree, x);
    free(z);
}

PCB* extract_min(RBTree *tree) {
    if (tree->root == tree->nil) return NULL;
    RBNode *min = tree_minimum(tree, tree->root);
    PCB *p = min->process;
    rb_delete(tree, min);
    return p;
}

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
        if (actual->tiempo_primera_ejecucion == -1) {
            actual->tiempo_primera_ejecucion = tiempo_global;
            actual->tiempo_respuesta = tiempo_global - actual->tiempo_llegada;
        }
        printf("[t=%d] Ejecuta %s (restante=%d)\n", tiempo_global, actual->nombre, actual->rafaga_restante);
        if (actual->rafaga_restante <= quantum) {
            tiempo_global += actual->rafaga_restante;
            actual->rafaga_restante = 0;
            actual->estado = TERMINADO;
            actual->tiempo_retorno = tiempo_global - actual->tiempo_llegada;
            actual->tiempo_espera = actual->tiempo_retorno - actual->rafaga_total;
            printf("  -> %s terminado. Retorno: %d, Espera: %d, Respuesta: %d\n", actual->nombre, actual->tiempo_retorno, actual->tiempo_espera, actual->tiempo_respuesta);
        } else {
            tiempo_global += quantum;
            actual->rafaga_restante -= quantum;
            encolar(&cola_tmp, actual);  // vuelve al final
        }
    }
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
    printf("\n=== CFS CON ÁRBOL ROJO-NEGRO ===\n");
    RBTree tree;
    init_rb_tree(&tree);
    // Insertar todos los procesos en el árbol
    while (!queue_vacia(q)) {
        PCB *p = desencolar(q);
        rb_insert(&tree, p);
    }
    int tiempo_global = 0;
    while (tree.root != tree.nil) {
        PCB *actual = extract_min(&tree);
        if (actual->tiempo_primera_ejecucion == -1) {
            actual->tiempo_primera_ejecucion = tiempo_global;
            actual->tiempo_respuesta = tiempo_global - actual->tiempo_llegada;
        }
        // Calcular slice basado en nice (simplificado)
        int slice = 2; // base
        if (actual->nice < 0) slice = 4;
        else if (actual->nice > 0) slice = 1;
        if (slice > actual->rafaga_restante) slice = actual->rafaga_restante;
        printf("Ejecuta %s (nice=%d, vruntime=%ld) slice=%d\n", actual->nombre, actual->nice, actual->vruntime, slice);
        tiempo_global += slice;
        actual->vruntime += slice;
        actual->rafaga_restante -= slice;
        if (actual->rafaga_restante > 0) {
            rb_insert(&tree, actual); // reinsertar
        } else {
            actual->estado = TERMINADO;
            actual->tiempo_retorno = tiempo_global - actual->tiempo_llegada;
            actual->tiempo_espera = actual->tiempo_retorno - actual->rafaga_total;
            printf("  -> %s terminado. Retorno: %d, Espera: %d, Respuesta: %d\n", actual->nombre, actual->tiempo_retorno, actual->tiempo_espera, actual->tiempo_respuesta);
        }
    }
    // Liberar memoria del árbol
    free(tree.nil);
}

/* ------------------- 5. PLANIFICACIÓN POR COLAS MULTINIVEL (Feedback) ------------------- */
#define NIVELES 3
#define QUANTUM_NIVEL1 2
#define QUANTUM_NIVEL2 4
#define QUANTUM_NIVEL3 8

typedef struct {
    ReadyQueue queues[NIVELES];
    int quantums[NIVELES];
} MultilevelQueue;

void init_multilevel_queue(MultilevelQueue *mq) {
    for (int i = 0; i < NIVELES; i++) {
        init_ready_queue(&mq->queues[i]);
    }
    mq->quantums[0] = QUANTUM_NIVEL1;
    mq->quantums[1] = QUANTUM_NIVEL2;
    mq->quantums[2] = QUANTUM_NIVEL3;
}

void planificar_multinivel(MultilevelQueue *mq) {
    printf("\n=== PLANIFICACIÓN POR COLAS MULTINIVEL ===\n");
    int tiempo_global = 0;
    while (1) {
        bool idle = true;
        for (int nivel = 0; nivel < NIVELES; nivel++) {
            if (!queue_vacia(&mq->queues[nivel])) {
                idle = false;
                PCB *actual = desencolar(&mq->queues[nivel]);
                if (actual->tiempo_primera_ejecucion == -1) {
                    actual->tiempo_primera_ejecucion = tiempo_global;
                    actual->tiempo_respuesta = tiempo_global - actual->tiempo_llegada;
                }
                int quantum = mq->quantums[nivel];
                printf("[t=%d] Nivel %d: Ejecuta %s (restante=%d)\n", tiempo_global, nivel+1, actual->nombre, actual->rafaga_restante);
                if (actual->rafaga_restante <= quantum) {
                    tiempo_global += actual->rafaga_restante;
                    actual->rafaga_restante = 0;
                    actual->estado = TERMINADO;
                    actual->tiempo_retorno = tiempo_global - actual->tiempo_llegada;
                    actual->tiempo_espera = actual->tiempo_retorno - actual->rafaga_total;
                    printf("  -> %s terminado. Retorno: %d, Espera: %d, Respuesta: %d\n", actual->nombre, actual->tiempo_retorno, actual->tiempo_espera, actual->tiempo_respuesta);
                } else {
                    tiempo_global += quantum;
                    actual->rafaga_restante -= quantum;
                    // Degradar a nivel inferior si no es el último
                    if (nivel < NIVELES - 1) {
                        encolar(&mq->queues[nivel + 1], actual);
                    } else {
                        encolar(&mq->queues[nivel], actual); // último nivel, RR
                    }
                }
                break; // procesar un proceso por ciclo
            }
        }
        if (idle) break;
    }
}

/* ------------------- 6. ALGORITMO DEL BANQUERO ------------------- */
#define NUM_RECURSOS 3
#define NUM_PROCESOS MAX_PROCESOS

typedef struct {
    int disponible[NUM_RECURSOS];
    int asignado[NUM_PROCESOS][NUM_RECURSOS];
    int maximo[NUM_PROCESOS][NUM_RECURSOS];
    int necesidad[NUM_PROCESOS][NUM_RECURSOS];
} EstadoSistema;

void calcular_necesidad(EstadoSistema *estado, int num_procesos) {
    for (int i = 0; i < num_procesos; i++) {
        for (int j = 0; j < NUM_RECURSOS; j++) {
            estado->necesidad[i][j] = estado->maximo[i][j] - estado->asignado[i][j];
        }
    }
}

bool es_seguro(EstadoSistema *estado, int num_procesos, int solicitud[], int proceso) {
    // Verificar si la solicitud puede ser concedida
    for (int j = 0; j < NUM_RECURSOS; j++) {
        if (solicitud[j] > estado->necesidad[proceso][j] || solicitud[j] > estado->disponible[j]) {
            return false;
        }
    }
    // Simular asignación
    int disponible_temp[NUM_RECURSOS];
    int asignado_temp[NUM_PROCESOS][NUM_RECURSOS];
    int necesidad_temp[NUM_PROCESOS][NUM_RECURSOS];
    memcpy(disponible_temp, estado->disponible, sizeof(disponible_temp));
    memcpy(asignado_temp, estado->asignado, sizeof(asignado_temp));
    memcpy(necesidad_temp, estado->necesidad, sizeof(necesidad_temp));

    for (int j = 0; j < NUM_RECURSOS; j++) {
        disponible_temp[j] -= solicitud[j];
        asignado_temp[proceso][j] += solicitud[j];
        necesidad_temp[proceso][j] -= solicitud[j];
    }

    // Verificar si hay secuencia segura
    bool terminado[NUM_PROCESOS] = {false};
    int trabajo[NUM_RECURSOS];
    memcpy(trabajo, disponible_temp, sizeof(trabajo));

    bool progreso = true;
    while (progreso) {
        progreso = false;
        for (int i = 0; i < num_procesos; i++) {
            if (!terminado[i]) {
                bool puede = true;
                for (int j = 0; j < NUM_RECURSOS; j++) {
                    if (necesidad_temp[i][j] > trabajo[j]) {
                        puede = false;
                        break;
                    }
                }
                if (puede) {
                    for (int j = 0; j < NUM_RECURSOS; j++) {
                        trabajo[j] += asignado_temp[i][j];
                    }
                    terminado[i] = true;
                    progreso = true;
                }
            }
        }
    }

    for (int i = 0; i < num_procesos; i++) {
        if (!terminado[i]) return false;
    }
    return true;
}

void algoritmo_banquero(EstadoSistema *estado, int num_procesos) {
    printf("\n=== ALGORITMO DEL BANQUERO ===\n");
    calcular_necesidad(estado, num_procesos);
    // Ejemplo de solicitud
    int solicitud[NUM_RECURSOS] = {1, 0, 2}; // ejemplo para proceso 0
    int proceso = 0;
    printf("Solicitud de proceso %d: ", proceso);
    for (int j = 0; j < NUM_RECURSOS; j++) printf("%d ", solicitud[j]);
    printf("\n");
    if (es_seguro(estado, num_procesos, solicitud, proceso)) {
        printf("Solicitud concedida. Estado seguro.\n");
        for (int j = 0; j < NUM_RECURSOS; j++) {
            estado->disponible[j] -= solicitud[j];
            estado->asignado[proceso][j] += solicitud[j];
            estado->necesidad[proceso][j] -= solicitud[j];
        }
    } else {
        printf("Solicitud denegada. Provocaría estado inseguro.\n");
    }
}

/* ------------------- 7. DETECCIÓN DE DEADLOCKS (Grafo de Espera) ------------------- */
typedef struct {
    int num_procesos;
    int num_recursos;
    int **grafo; // grafo[i][j] = 1 si proceso i espera recurso j
} GrafoEspera;

void init_grafo(GrafoEspera *g, int num_p, int num_r) {
    g->num_procesos = num_p;
    g->num_recursos = num_r;
    g->grafo = (int**)malloc(num_p * sizeof(int*));
    for (int i = 0; i < num_p; i++) {
        g->grafo[i] = (int*)calloc(num_r, sizeof(int));
    }
}

void liberar_grafo(GrafoEspera *g) {
    for (int i = 0; i < g->num_procesos; i++) {
        free(g->grafo[i]);
    }
    free(g->grafo);
}

bool dfs_ciclo(GrafoEspera *g, int proceso, bool *visitado, bool *recursion) {
    if (recursion[proceso]) return true;
    if (visitado[proceso]) return false;

    visitado[proceso] = true;
    recursion[proceso] = true;

    // Para simplificar, asumir que recursos son nodos, y procesos esperan recursos
    // En un grafo de espera real, nodos son procesos y recursos, pero aquí simplificamos
    // Simular ciclo si hay dependencia
    for (int r = 0; r < g->num_recursos; r++) {
        if (g->grafo[proceso][r]) {
            // Simular que otro proceso tiene el recurso
            int otro_proceso = (proceso + 1) % g->num_procesos; // simplificación
            if (dfs_ciclo(g, otro_proceso, visitado, recursion)) return true;
        }
    }

    recursion[proceso] = false;
    return false;
}

bool detectar_ciclo(GrafoEspera *g) {
    bool visitado[MAX_PROCESOS] = {false};
    bool recursion[MAX_PROCESOS] = {false};

    for (int i = 0; i < g->num_procesos; i++) {
        if (dfs_ciclo(g, i, visitado, recursion)) {
            return true;
        }
    }
    return false;
}

void detectar_deadlocks(GrafoEspera *g) {
    printf("\n=== DETECCIÓN DE DEADLOCKS ===\n");
    if (detectar_ciclo(g)) {
        printf("Deadlock detectado: hay un ciclo en el grafo de espera.\n");
    } else {
        printf("No hay deadlock.\n");
    }
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
    nuevo->tiempo_llegada = 0; // asumir llegada en t=0 para simplicidad
    nuevo->tiempo_respuesta = -1; // no respondido aún
    nuevo->tiempo_espera = 0;
    nuevo->tiempo_retorno = 0;
    nuevo->tiempo_primera_ejecucion = -1;
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
    // planificar_rr(&cola, QUANTUM);
    // planificar_cfs_simple(&cola);
    
    // Para multinivel
    // MultilevelQueue mq;
    // init_multilevel_queue(&mq);
    // encolar(&mq.queues[0], crear_proceso(1, "P1", 8, 0, 3));
    // ... encolar procesos en niveles
    // planificar_multinivel(&mq);
    
    // Para banquero
    // EstadoSistema estado;
    // memset(&estado, 0, sizeof(EstadoSistema));
    // estado.disponible[0] = 3; estado.disponible[1] = 3; estado.disponible[2] = 2;
    // estado.maximo[0][0] = 7; // etc.
    // algoritmo_banquero(&estado, 4);
    
    // Para detección
    // GrafoEspera g;
    // init_grafo(&g, 4, 3);
    // g.grafo[0][0] = 1; // etc.
    // detectar_deadlocks(&g);
    // liberar_grafo(&g);
    
    // Limpiar (evitar leaks en implementación final)
    // En un entorno real se libera toda la cola.
    return 0;
}