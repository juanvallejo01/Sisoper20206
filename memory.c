#include <stdio.h>
#include <stdbool.h>

/* ============================================================
   CONFIGURACIÓN GENERAL
   ============================================================ */
#define MEMORY_SIZE 1024
#define MAX_BLOCKS  64   /* máximo de bloques dinámicos posibles */

typedef struct
{
    int  process_id;   /* -1 si está libre */
    int  size;
    bool occupied;
} Block;

Block memory[MAX_BLOCKS];
int   block_count = 0;


/* ============================================================
   INICIALIZAR
   ============================================================ */
void initialize_memory()
{
    memory[0].process_id = -1;
    memory[0].size       = MEMORY_SIZE;
    memory[0].occupied   = false;
    block_count          = 1;
}


/* ============================================================
   IMPRIMIR
   ============================================================ */
void print_memory()
{
    printf("\n======= ESTADO DE MEMORIA =======\n");
    for (int i = 0; i < block_count; i++)
    {
        if (memory[i].occupied)
            printf("  Bloque %d: Proceso %d  (%d bytes)\n",
                   i, memory[i].process_id, memory[i].size);
        else
            printf("  Bloque %d: LIBRE       (%d bytes)\n",
                   i, memory[i].size);
    }
    printf("=================================\n");
}


/* ============================================================
   FUSIONAR BLOQUES LIBRES CONTIGUOS  (coalescencia)
   ============================================================ */
void merge_free_blocks()
{
    for (int i = 0; i < block_count - 1; i++)
    {
        if (!memory[i].occupied && !memory[i + 1].occupied)
        {
            memory[i].size += memory[i + 1].size;

            /* desplazar todos los bloques posteriores una posición */
            for (int k = i + 1; k < block_count - 1; k++)
                memory[k] = memory[k + 1];

            block_count--;
            i--;   /* revisar el mismo índice de nuevo */
        }
    }
}


/* ============================================================
   DIVIDIR BLOQUE  (particionamiento dinámico)
   ============================================================ */
void split_block(int index, int size)
{
    int remaining = memory[index].size - size;

    memory[index].size = size;   /* ajustar bloque asignado */

    if (remaining > 0 && block_count < MAX_BLOCKS)
    {
        /* hacer hueco desplazando bloques a la derecha */
        for (int j = block_count; j > index + 1; j--)
            memory[j] = memory[j - 1];

        memory[index + 1].process_id = -1;
        memory[index + 1].size       = remaining;
        memory[index + 1].occupied   = false;

        block_count++;
    }
}


/* ============================================================
   BEST FIT  — asigna el bloque libre más pequeño que alcance
   ============================================================ */
void allocate_best_fit(int pid, int size)
{
    int best_index = -1;
    int min_diff   = MEMORY_SIZE + 1;

    for (int i = 0; i < block_count; i++)
    {
        if (!memory[i].occupied && memory[i].size >= size)
        {
            int diff = memory[i].size - size;
            if (diff < min_diff)
            {
                min_diff   = diff;
                best_index = i;
            }
        }
    }

    if (best_index == -1)
    {
        printf("BEST FIT: Sin espacio para proceso %d (%d bytes).\n", pid, size);
        return;
    }

    split_block(best_index, size);          /* <-- particionamiento dinámico */

    memory[best_index].process_id = pid;
    memory[best_index].occupied   = true;

    printf("BEST FIT: Proceso %d asignado al bloque %d (%d bytes).\n",
           pid, best_index, size);
}


/* ============================================================
   WORST FIT  — asigna el bloque libre más grande disponible
   ============================================================ */
void allocate_worst_fit(int pid, int size)
{
    int worst_index = -1;
    int max_size    = -1;

    for (int i = 0; i < block_count; i++)
    {
        if (!memory[i].occupied && memory[i].size >= size)
        {
            if (memory[i].size > max_size)
            {
                max_size    = memory[i].size;
                worst_index = i;
            }
        }
    }

    if (worst_index == -1)
    {
        printf("WORST FIT: Sin espacio para proceso %d (%d bytes).\n", pid, size);
        return;
    }

    split_block(worst_index, size);         /* <-- particionamiento dinámico */

    memory[worst_index].process_id = pid;
    memory[worst_index].occupied   = true;

    printf("WORST FIT: Proceso %d asignado al bloque %d (%d bytes).\n",
           pid, worst_index, size);
}


/* ============================================================
   LIBERAR PROCESO
   ============================================================ */
void free_memory(int pid)
{
    for (int i = 0; i < block_count; i++)
    {
        if (memory[i].occupied && memory[i].process_id == pid)
        {
            printf("Liberando proceso %d del bloque %d.\n", pid, i);
            memory[i].process_id = -1;
            memory[i].occupied   = false;
            merge_free_blocks();   /* coalescencia tras liberar */
            return;
        }
    }

    printf("Error: proceso %d no encontrado.\n", pid);
}


/* ============================================================
   MAIN
   ============================================================ */
int main()
{
    printf("====== SIMULADOR DE PARTICIONAMIENTO DINAMICO ======\n");
    printf("Memoria total: %d bytes | Max bloques: %d\n\n",
           MEMORY_SIZE, MAX_BLOCKS);

    /* ---------- BEST FIT demo ---------- */
    printf("===== PRUEBA: BEST FIT =====\n");
    initialize_memory();

    printf("\n1. Memoria inicial:\n");
    print_memory();

    printf("\n2. Asignando procesos con Best Fit...\n");
    allocate_best_fit(101, 200);
    allocate_best_fit(102, 150);
    allocate_best_fit(103, 250);
    print_memory();

    printf("\n3. Liberando proceso 102...\n");
    free_memory(102);
    print_memory();

    printf("\n4. Asignando proceso 104 (100 bytes) — debe usar el hueco exacto mas pequeño...\n");
    allocate_best_fit(104, 100);
    print_memory();

    printf("\n5. Intentando asignar proceso muy grande (900 bytes)...\n");
    allocate_best_fit(105, 900);
    print_memory();

    /* ---------- WORST FIT demo ---------- */
    printf("\n\n===== PRUEBA: WORST FIT =====\n");
    initialize_memory();

    printf("\n1. Memoria inicial:\n");
    print_memory();

    printf("\n2. Asignando procesos con Worst Fit...\n");
    allocate_worst_fit(201, 200);
    allocate_worst_fit(202, 150);
    allocate_worst_fit(203, 250);
    print_memory();

    printf("\n3. Liberando proceso 202...\n");
    free_memory(202);
    print_memory();

    printf("\n4. Asignando proceso 204 (100 bytes) — debe usar el bloque libre mas grande...\n");
    allocate_worst_fit(204, 100);
    print_memory();

    printf("\n5. Intentando asignar proceso muy grande (900 bytes)...\n");
    allocate_worst_fit(205, 900);
    print_memory();

    return 0;
}