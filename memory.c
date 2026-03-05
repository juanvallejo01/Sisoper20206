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
/* ============================================================
   ========================  PAGINACIÓN  ======================
   ============================================================ */

/* Inicializa todos los marcos como libres */
void initialize_paging()
{
    for (int i = 0; i < TOTAL_FRAMES; i++)
        frames[i] = false;
    page_process_count = 0;
    printf("Paginación inicializada: %d marcos de %d bytes cada uno.\n",
           TOTAL_FRAMES, PAGE_SIZE);
}

/* Muestra el estado de cada marco físico */
void print_frames()
{
    printf("\n======= ESTADO DE MARCOS (Paginación) =======\n");
    printf("  Tamaño de página/marco: %d bytes\n", PAGE_SIZE);
    printf("  Marcos totales: %d\n\n", TOTAL_FRAMES);

    for (int i = 0; i < TOTAL_FRAMES; i++)
    {
        if (frames[i])
            printf("  Marco %2d: OCUPADO\n", i);
        else
            printf("  Marco %2d: libre\n", i);
    }
    printf("=============================================\n");
}

/* Muestra la tabla de páginas de todos los procesos */
void print_page_tables()
{
    printf("\n======= TABLAS DE PÁGINAS =======\n");
    if (page_process_count == 0)
    {
        printf("  (sin procesos paginados)\n");
    }
    for (int i = 0; i < page_process_count; i++)
    {
        printf("  Proceso %d (%d páginas):\n",
               page_processes[i].process_id,
               page_processes[i].page_count);

        for (int p = 0; p < page_processes[i].page_count; p++)
            printf("    Página %d → Marco %d  (dir. física: %d)\n",
                   p,
                   page_processes[i].page_table[p],
                   page_processes[i].page_table[p] * PAGE_SIZE);
    }
    printf("=================================\n");
}

/* Asigna páginas a un proceso según su tamaño */
void allocate_pages(int pid, int size)
{
    /* calcular cuántas páginas necesita (redondeo hacia arriba) */
    int pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    if (page_process_count >= MAX_PROCESS)
    {
        printf("PAGINACIÓN: Límite de procesos alcanzado.\n");
        return;
    }

    if (pages_needed > MAX_PAGES)
    {
        printf("PAGINACIÓN: Proceso %d necesita %d páginas, supera MAX_PAGES=%d.\n",
               pid, pages_needed, MAX_PAGES);
        return;
    }

    /* contar marcos libres disponibles */
    int free_frames = 0;
    for (int i = 0; i < TOTAL_FRAMES; i++)
        if (!frames[i]) free_frames++;

    if (free_frames < pages_needed)
    {
        printf("PAGINACIÓN: Sin marcos suficientes para proceso %d "
               "(necesita %d, hay %d libres).\n",
               pid, pages_needed, free_frames);
        return;
    }

    /* registrar proceso en la tabla */
    PageTableEntry *entry = &page_processes[page_process_count];
    entry->process_id = pid;
    entry->page_count = pages_needed;

    /* asignar marcos libres uno a uno */
    int assigned = 0;
    for (int i = 0; i < TOTAL_FRAMES && assigned < pages_needed; i++)
    {
        if (!frames[i])
        {
            frames[i]                  = true;
            entry->page_table[assigned] = i;
            assigned++;
        }
    }

    page_process_count++;

    int wasted = (pages_needed * PAGE_SIZE) - size;  /* fragmentación interna */
    printf("PAGINACIÓN: Proceso %d asignado → %d página(s), "
           "%d bytes usados, %d bytes desperdiciados (frag. interna).\n",
           pid, pages_needed, size, wasted);
}

/* Libera todos los marcos de un proceso paginado */
void free_pages(int pid)
{
    for (int i = 0; i < page_process_count; i++)
    {
        if (page_processes[i].process_id == pid)
        {
            printf("PAGINACIÓN: Liberando proceso %d (%d marcos).\n",
                   pid, page_processes[i].page_count);

            /* liberar cada marco */
            for (int p = 0; p < page_processes[i].page_count; p++)
                frames[page_processes[i].page_table[p]] = false;

            /* eliminar entrada de la tabla compactando */
            for (int k = i; k < page_process_count - 1; k++)
                page_processes[k] = page_processes[k + 1];

            page_process_count--;
            return;
        }
    }
    printf("PAGINACIÓN: Proceso %d no encontrado.\n", pid);
}

/* Traduce dirección lógica → dirección física */
void translate_address(int pid, int logical_address)
{
    int page   = logical_address / PAGE_SIZE;   /* número de página  */
    int offset = logical_address % PAGE_SIZE;   /* desplazamiento    */

    for (int i = 0; i < page_process_count; i++)
    {
        if (page_processes[i].process_id == pid)
        {
            if (page >= page_processes[i].page_count)
            {
                printf("TRADUCCIÓN: Dirección lógica %d fuera de rango "
                       "(proceso %d tiene %d páginas).\n",
                       logical_address, pid, page_processes[i].page_count);
                return;
            }
            int frame    = page_processes[i].page_table[page];
            int physical = frame * PAGE_SIZE + offset;
            printf("TRADUCCIÓN: Proceso %d | Dir. lógica %d → "
                   "Página %d + Offset %d → Marco %d → Dir. física %d\n",
                   pid, logical_address, page, offset, frame, physical);
            return;
        }
    }
    printf("TRADUCCIÓN: Proceso %d no encontrado.\n", pid);
}
