#include <stdio.h>
#include <stdbool.h>

#define MEMORY_SYZE 1024
#define PARTITIONS 4
#define PART_SYZE (MEMORY_SYZE / PARTITIONS) /* Tamaño de cada partición */

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
    
    for(int i = 0; i < PARTITIONS; i++)
    {
        if(!memory[i].occupied)
        {
            memory[i].process_id = pid;
            memory[i].size = size;
            memory[i].occupied = true;
            printf("Proceso %d asignado a la particion %d (%d bytes)\n", pid, i, size);
            return;
        }
    }
    
    /* No hay particiones libres */
    printf("\nError: No hay particiones libres para el proceso %d.\n", pid);
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
    printf("====== SIMULADOR DE PARTICIONES FIJAS ======\n");
    printf("Memoria total: %d bytes\n", MEMORY_SYZE);
    printf("Numero de particiones: %d\n", PARTITIONS);
    printf("Tamaño de cada particion: %d bytes\n\n", PART_SYZE);
    
    // INICIALIZAMOS
    printf("1. Inicializando memoria...\n");
    initalize_mem();
    print_mem();
    
    // ALOJAMOS 3 PROCESOS
    printf("\n2. Asignando 3 procesos...\n");
    allocate_mem(101, 200);  // Proceso 101, 200 bytes
    allocate_mem(102, 150);  // Proceso 102, 150 bytes
    allocate_mem(103, 250);  // Proceso 103, 250 bytes
    
    // IMPRIMIMOS
    printf("\n3. Estado despues de asignar procesos:\n");
    print_mem();
    
    // LIBERAMOS UN PROCESO
    printf("\n4. Liberando proceso 102...\n");
    free_mem(102);
    
    // IMPRIMIMOS DE NUEVO
    printf("\n5. Estado despues de liberar proceso:\n");
    print_mem();
    
    // PRUEBAS ADICIONALES
    printf("\n6. Intentando asignar un proceso muy grande...\n");
    allocate_mem(104, 300);  // Excede el tamaño de partición
    
    printf("\n7. Asignando proceso en particion liberada...\n");
    allocate_mem(105, 180);  // Debería ocupar la partición liberada
    
    printf("\n8. Estado final:\n");
    print_mem();
    
    return 0;
}
