#include <stdio.h>
#include <stdbool.h>

#define MEMORY_SIZE 1024
#define MAX_BLOCKS  50

typedef struct
{
    int  process_id;  /* -1 si esta libre */
    int  size;
    bool occupied;
} Block;

Block memory[MAX_BLOCKS];
int   block_count = 0;

/* ================== INICIALIZAR ================== */
void initialize_memory()
{
    memory[0].process_id = -1;
    memory[0].size       = MEMORY_SIZE;
    memory[0].occupied   = false;
    block_count          = 1;
}

/* ================== IMPRIMIR ================== */
void print_memory()
{
    printf("\n======= ESTADO DE MEMORIA =======\n");
    for (int i = 0; i < block_count; i++)
    {
        if (memory[i].occupied)
            printf("Bloque %d: Proceso %d (%d bytes)\n",
                   i, memory[i].process_id, memory[i].size);
        else
            printf("Bloque %d: LIBRE (%d bytes)\n",
                   i, memory[i].size);
    }
    printf("=================================\n");
}

/* ================== FUSIONAR BLOQUES LIBRES ================== */
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

/* ================== DIVIDIR BLOQUE ================== */
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

/* ================== BEST FIT ================== */
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

    memory[best_index].process_id = pid;
    memory[best_index].occupied   = true;
    split_block(best_index, size);

    printf("BEST FIT: Proceso %d (%d bytes) -> Bloque %d.\n", pid, size, best_index);
}

/* ================== WORST FIT ================== */
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

    memory[worst_index].process_id = pid;
    memory[worst_index].occupied   = true;
    split_block(worst_index, size);

    printf("WORST FIT: Proceso %d (%d bytes) -> Bloque %d.\n", pid, size, worst_index);
}

/* ================== LIBERAR ================== */
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

/* ================== MAIN ================== */
int main()
{
    printf("====== SIMULADOR DE MEMORIA DINAMICA ======\n");
    printf("Memoria total: %d bytes\n", MEMORY_SIZE);

    initialize_memory();
    print_memory();

    /* --- BEST FIT --- */
    printf("\n--- Pruebas BEST FIT ---\n");
    allocate_best_fit(101, 200);
    allocate_best_fit(102, 300);
    allocate_best_fit(103, 100);
    print_memory();

    printf("\n--- Liberando proceso 102 ---\n");
    free_memory(102);
    print_memory();

    /* --- WORST FIT --- */
    printf("\n--- Pruebas WORST FIT ---\n");
    allocate_worst_fit(104, 250);
    print_memory();

    /* --- CASOS DE ERROR --- */
    printf("\n--- Casos de error ---\n");
    allocate_worst_fit(105, 900);  /* Sin espacio */
    free_memory(999);              /* PID inexistente */

    printf("\n--- Estado final ---\n");
    print_memory();

    return 0;
}
