#include <stdio.h>
#include <stdbool.h>

/* ============================================================
   CONFIGURACIÓN GENERAL
   ============================================================ */
#define MEMORY_SIZE  1024
#define MAX_BLOCKS   64

/* ============================================================
   CONFIGURACIÓN DE PAGINACIÓN
   ============================================================ */
#define PAGE_SIZE    64          /* tamaño de cada página/marco en bytes  */
#define TOTAL_FRAMES (MEMORY_SIZE / PAGE_SIZE)  /* 1024/64 = 16 marcos   */
#define MAX_PAGES    16          /* máximo de páginas por proceso          */
#define MAX_PROCESS  10          /* máximo de procesos en tabla de páginas */

/* ============================================================
   ESTRUCTURAS — PARTICIONAMIENTO DINÁMICO
   ============================================================ */
typedef struct
{
    int  process_id;
    int  size;
    bool occupied;
} Block;

Block memory[MAX_BLOCKS];
int   block_count = 0;

/* ============================================================
   ESTRUCTURAS — PAGINACIÓN
   ============================================================ */

/* Tabla de páginas de UN proceso */
typedef struct
{
    int  process_id;
    int  page_count;                /* cuántas páginas usa este proceso  */
    int  page_table[MAX_PAGES];     /* page_table[i] = número de marco   */
} PageTableEntry;

bool            frames[TOTAL_FRAMES];       /* true = ocupado            */
PageTableEntry  page_processes[MAX_PROCESS];
int             page_process_count = 0;


/* ============================================================
   ==================  PARTICIONAMIENTO DINÁMICO  =============
   ============================================================ */

void initialize_memory()
{
    memory[0].process_id = -1;
    memory[0].size       = MEMORY_SIZE;
    memory[0].occupied   = false;
    block_count          = 1;
}

void print_memory()
{
    printf("\n======= ESTADO DE MEMORIA (Dinámico) =======\n");
    for (int i = 0; i < block_count; i++)
    {
        if (memory[i].occupied)
            printf("  Bloque %d: Proceso %d  (%d bytes)\n",
                   i, memory[i].process_id, memory[i].size);
        else
            printf("  Bloque %d: LIBRE       (%d bytes)\n",
                   i, memory[i].size);
    }
    printf("=============================================\n");
}

void merge_free_blocks()
{
    for (int i = 0; i < block_count - 1; i++)
    {
        if (!memory[i].occupied && !memory[i + 1].occupied)
        {
            memory[i].size += memory[i + 1].size;
            for (int k = i + 1; k < block_count - 1; k++)
                memory[k] = memory[k + 1];
            block_count--;
            i--;
        }
    }
}

void split_block(int index, int size)
{
    int remaining = memory[index].size - size;
    memory[index].size = size;

    if (remaining > 0 && block_count < MAX_BLOCKS)
    {
        for (int j = block_count; j > index + 1; j--)
            memory[j] = memory[j - 1];

        memory[index + 1].process_id = -1;
        memory[index + 1].size       = remaining;
        memory[index + 1].occupied   = false;
        block_count++;
    }
}

void allocate_best_fit(int pid, int size)
{
    int best_index = -1;
    int min_diff   = MEMORY_SIZE + 1;

    for (int i = 0; i < block_count; i++)
    {
        if (!memory[i].occupied && memory[i].size >= size)
        {
            int diff = memory[i].size - size;
            if (diff < min_diff) { min_diff = diff; best_index = i; }
        }
    }

    if (best_index == -1)
    {
        printf("BEST FIT: Sin espacio para proceso %d (%d bytes).\n", pid, size);
        return;
    }

    split_block(best_index, size);
    memory[best_index].process_id = pid;
    memory[best_index].occupied   = true;
    printf("BEST FIT: Proceso %d → bloque %d (%d bytes).\n", pid, best_index, size);
}

void allocate_worst_fit(int pid, int size)
{
    int worst_index = -1;
    int max_size    = -1;

    for (int i = 0; i < block_count; i++)
    {
        if (!memory[i].occupied && memory[i].size >= size)
        {
            if (memory[i].size > max_size) { max_size = memory[i].size; worst_index = i; }
        }
    }

    if (worst_index == -1)
    {
        printf("WORST FIT: Sin espacio para proceso %d (%d bytes).\n", pid, size);
        return;
    }

    split_block(worst_index, size);
    memory[worst_index].process_id = pid;
    memory[worst_index].occupied   = true;
    printf("WORST FIT: Proceso %d → bloque %d (%d bytes).\n", pid, worst_index, size);
}

void free_memory(int pid)
{
    for (int i = 0; i < block_count; i++)
    {
        if (memory[i].occupied && memory[i].process_id == pid)
        {
            printf("Liberando proceso %d del bloque %d.\n", pid, i);
            memory[i].process_id = -1;
            memory[i].occupied   = false;
            merge_free_blocks();
            return;
        }
    }
    printf("Error: proceso %d no encontrado.\n", pid);
}
