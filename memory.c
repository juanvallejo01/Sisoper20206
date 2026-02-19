#include <stdio.h>
#include <stdbool.h>

#define MEMORY_SYZE 1024
#define PARTITIONS 4
#define PART_SYZE (MEMORY_SYZE / PARTITIONS) /* Tamaño de cada partición */

typedef struct
{
    int process_id;
    int size;
    bool occupied;
} Partition;

// Representación lógica de las particiones de memoria 
Partition memory[PARTITIONS];

void initalize_mem()
{
    for(int i = 0; i < PARTITIONS; i++)
    {
        memory[i].process_id = -1; /* -1 indica que la partición está libre */
        memory[i].size = 0;
        memory[i].occupied = false;
    }
}

void print_mem()
{
    printf("\n=== Estado de asignacion de memoria ===\n");
    for(int i = 0; i < PARTITIONS; i++)
    {
        if(memory[i].occupied)
        {
            printf("Particion %d: Proceso %d (%d bytes)\n",
                i, memory[i].process_id, memory[i].size);
        }
        else
        {
            printf("Particion %d: LIBRE (%d bytes disponibles)\n", i, PART_SYZE);
        }
    }
    printf("=======================================\n");
}
//usa worst fit y best fit para asignar memoria a los procesos
void allocate_mem(int pid, int size)
{
    if(size > PART_SYZE)
    {
        printf("\nError: El proceso %d (%d bytes) excede el tamaño de la partición (%d bytes).\n", 
            pid, size, PART_SYZE);
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

void free_mem(int pid)
{
    for(int i = 0; i < PARTITIONS; i++)
    {
        if(memory[i].process_id == pid)
        {
            printf("Liberando proceso %d de la particion %d\n", pid, i);
            memory[i].process_id = -1;
            memory[i].size = 0;
            memory[i].occupied = false;
            return;
        }
    }
    printf("\nError: Proceso %d no encontrado.\n", pid);
}

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