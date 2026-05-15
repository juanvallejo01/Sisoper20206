/*
    Ejemplo didáctico de Journaling e Inodes
    ANSI C compatible
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_LINEA 256

/* ---------------------------
   Journaling e Inodes
   --------------------------- */
#define MAX_LOG 10
#define BLOCK_SIZE 128

typedef struct {
    int inode_number;
    int size;
    int block;
    int used;
} Inode;

typedef struct {
    char operation[32];
    int inode;
    int committed;
} JournalEntry;

char disk_block[BLOCK_SIZE];
Inode inode_table[4];
JournalEntry journal[MAX_LOG];
int log_index = 0;

void log_start(const char *op, int inode)
{
    strcpy(journal[log_index].operation, op);
    journal[log_index].inode = inode;
    journal[log_index].committed = 0;
    log_index++;
}

void log_commit()
{
    journal[log_index - 1].committed = 1;
}

void show_journal()
{
    int i;
    printf("\nJOURNAL:\n");
    for(i = 0; i < log_index; i++)
    {
        printf("Op=%s inode=%d committed=%d\n",
            journal[i].operation,
            journal[i].inode,
            journal[i].committed);
    }
}

void fs_create(int inode_num)
{
    log_start("CREATE", inode_num);
    inode_table[inode_num].inode_number = inode_num;
    inode_table[inode_num].size = 0;
    inode_table[inode_num].block = inode_num;
    inode_table[inode_num].used = 1;
    log_commit();
}

void fs_write(int inode_num, const char *text)
{
    log_start("WRITE", inode_num);
    strcpy(disk_block, text);
    inode_table[inode_num].size = strlen(text);
    log_commit();
}

void fs_read(int inode_num)
{
    if(inode_table[inode_num].used)
    {
        printf("\nREAD inode %d:\n%s\n", inode_num, disk_block);
    }
}

void fs_close(int inode_num)
{
    log_start("CLOSE", inode_num);
    log_commit();
}

void demo_journaling()
{
    fs_create(0);
    fs_write(0, "Hola journaling con inodes");
    fs_read(0);
    fs_close(0);
    show_journal();
}

int contar_palabras(const char *linea);
void convertir_mayusculas(char *linea);

int main() {
    FILE *archivo_entrada;
    FILE *archivo_salida;

    char buffer[MAX_LINEA];
    int total_lineas = 0;
    int total_palabras = 0;
    int ret_code = 0;

    /* Abrir archivo de entrada en modo lectura */
    archivo_entrada = fopen("entrada.txt", "r");

    /* Verificar si el archivo se abrió correctamente */
    if (archivo_entrada == NULL) {
        perror("Error al abrir archivo de entrada");
        fprintf(stderr, "El archivo 'entrada.txt' no existe o no tiene permisos de lectura.\n");
        return 1;
    }

    /* Abrir archivo de salida en modo escritura */
    archivo_salida = fopen("salida.txt", "w");

    /* Verificar si el archivo de salida se abrió correctamente */
    if (archivo_salida == NULL) {
        perror("Error al abrir archivo de salida");
        fprintf(stderr, "No es posible escribir en 'salida.txt'.\n");
        fclose(archivo_entrada);
        return 1;
    }

    /* Procesar archivo de entrada */
    while (fgets(buffer, MAX_LINEA, archivo_entrada) != NULL) {
        total_lineas++;

        /* Contar palabras en la línea y acumular */
        total_palabras += contar_palabras(buffer);

        /* Convertir la línea a mayúsculas */
        convertir_mayusculas(buffer);

        /* Escribir la línea transformada en el archivo de salida */
        if (fputs(buffer, archivo_salida) == EOF) {
            perror("Error al escribir en archivo de salida");
            fprintf(stderr, "Error durante la escritura en 'salida.txt'.\n");
            ret_code = 1;
            break;
        }
    }

    /* Verificar si ocurrió error en lectura */
    if (ferror(archivo_entrada)) {
        perror("Error al leer archivo de entrada");
        fprintf(stderr, "Error durante la lectura de 'entrada.txt'.\n");
        ret_code = 1;
    }

    /* Escribir estadísticas en el archivo de salida */
    fprintf(archivo_salida, "\n\n--- ESTADÍSTICAS ---\n");
    fprintf(archivo_salida, "Total de líneas: %d\n", total_lineas);
    fprintf(archivo_salida, "Total de palabras: %d\n", total_palabras);

    /* Cerrar ambos archivos */
    if (fclose(archivo_entrada) == EOF) {
        perror("Error al cerrar archivo de entrada");
        ret_code = 1;
    }

    if (fclose(archivo_salida) == EOF) {
        perror("Error al cerrar archivo de salida");
        ret_code = 1;
    }

    if (ret_code == 0) {
        printf("Proceso completado exitosamente.\n");
        printf("Resultados guardados en 'salida.txt'.\n");
    }

    printf("\n--- DEMO JOURNALING E INODES ---\n");
    demo_journaling();

    return ret_code;
}

int contar_palabras(const char *linea) {
    int contador = 0;
    int en_palabra = 0;

    /* Implementar lógica */
    for (int i = 0; linea[i] != '\0'; i++) {
        if (isspace(linea[i])) {
            en_palabra = 0;
        } else if (en_palabra == 0) {
            contador++;
            en_palabra = 1;
        }
    }

    return contador;
}

void convertir_mayusculas(char *linea) {
    int i = 0;

    /* Implementar conversión */
    while (linea[i] != '\0') {
        linea[i] = toupper(linea[i]);
        i++;
    }
}