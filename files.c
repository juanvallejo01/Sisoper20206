/* files.c
   Simulación básica de sistema de archivos en memoria
   Compilar junto a kernel.c:  gcc -std=c89 -Wall files.c kernel.c -o kernel
*/

#include <stdio.h>
#include <string.h>

#define MAX_FILES   20
#define MAX_NAME    50
#define MAX_CONTENT 256

typedef struct {
    char nombre[MAX_NAME];
    char contenido[MAX_CONTENT];
    int  abierto;   /* 1 = en uso / bloqueado */
    int  usado;     /* 1 = entrada ocupada    */
} Archivo;

static Archivo disco[MAX_FILES];

/* ── Inicializa la tabla de archivos ── */
void init_filesystem(void)
{
    int i;
    for (i = 0; i < MAX_FILES; i++) {
        disco[i].usado   = 0;
        disco[i].abierto = 0;
        disco[i].nombre[0]    = '\0';
        disco[i].contenido[0] = '\0';
    }
}

/* ── Busca un archivo por nombre; devuelve índice o -1 ── */
static int buscar(const char *nombre)
{
    int i;
    for (i = 0; i < MAX_FILES; i++)
        if (disco[i].usado && strcmp(disco[i].nombre, nombre) == 0)
            return i;
    return -1;
}

/* ── Crear archivo ── */
void crear_archivo(char nombre[])
{
    int i;
    if (buscar(nombre) >= 0) {
        printf("Error: el archivo '%s' ya existe.\n", nombre);
        return;
    }
    for (i = 0; i < MAX_FILES; i++) {
        if (!disco[i].usado) {
            strncpy(disco[i].nombre, nombre, MAX_NAME - 1);
            disco[i].nombre[MAX_NAME - 1] = '\0';
            disco[i].contenido[0] = '\0';
            disco[i].abierto = 0;
            disco[i].usado   = 1;
            printf("Archivo '%s' creado.\n", nombre);
            return;
        }
    }
    printf("Error: disco lleno (máximo %d archivos).\n", MAX_FILES);
}

/* ── Abrir archivo (simula bloqueo si ya está abierto) ── */
void abrir_archivo(char nombre[])
{
    int idx = buscar(nombre);
    if (idx < 0) {
        printf("Error: archivo '%s' no existe.\n", nombre);
        return;
    }
    if (disco[idx].abierto) {
        printf("Aviso: archivo '%s' ya está abierto (bloqueado por otro proceso).\n", nombre);
        return;
    }
    disco[idx].abierto = 1;
    printf("Archivo '%s' abierto.\n", nombre);
}

/* ── Escribir en archivo ── */
void escribir_archivo(char nombre[], char texto[])
{
    int idx = buscar(nombre);
    if (idx < 0) {
        printf("Error: archivo '%s' no existe.\n", nombre);
        return;
    }
    if (!disco[idx].abierto) {
        printf("Error: debe abrir '%s' antes de escribir.\n", nombre);
        return;
    }
    strncpy(disco[idx].contenido, texto, MAX_CONTENT - 1);
    disco[idx].contenido[MAX_CONTENT - 1] = '\0';
    printf("Contenido guardado en '%s'.\n", nombre);
}

/* ── Leer archivo ── */
void leer_archivo(char nombre[])
{
    int idx = buscar(nombre);
    if (idx < 0) {
        printf("Error: archivo '%s' no existe.\n", nombre);
        return;
    }
    if (!disco[idx].abierto) {
        printf("Error: debe abrir '%s' antes de leer.\n", nombre);
        return;
    }
    printf("Contenido de '%s':\n  %s\n",
           nombre,
           disco[idx].contenido[0] ? disco[idx].contenido : "(vacío)");
}

/* ── Cerrar archivo ── */
void cerrar_archivo(char nombre[])
{
    int idx = buscar(nombre);
    if (idx < 0) {
        printf("Error: archivo '%s' no existe.\n", nombre);
        return;
    }
    if (!disco[idx].abierto) {
        printf("Aviso: archivo '%s' ya estaba cerrado.\n", nombre);
        return;
    }
    disco[idx].abierto = 0;
    printf("Archivo '%s' cerrado.\n", nombre);
}

/* ── Eliminar archivo ── */
void eliminar_archivo(char nombre[])
{
    int idx = buscar(nombre);
    if (idx < 0) {
        printf("Error: archivo '%s' no existe.\n", nombre);
        return;
    }
    if (disco[idx].abierto) {
        printf("Error: no se puede eliminar '%s', está abierto (en uso).\n", nombre);
        return;
    }
    disco[idx].usado        = 0;
    disco[idx].abierto      = 0;
    disco[idx].nombre[0]    = '\0';
    disco[idx].contenido[0] = '\0';
    printf("Archivo '%s' eliminado.\n", nombre);
}

/* ── Listar todos los archivos ── */
void listar_archivos(void)
{
    int i, hay = 0;
    printf("\nArchivos en sistema:\n");
    printf("  %-30s  Estado\n", "Nombre");
    printf("  %-30s  ------\n", "------");
    for (i = 0; i < MAX_FILES; i++) {
        if (disco[i].usado) {
            printf("  %-30s  %s\n",
                   disco[i].nombre,
                   disco[i].abierto ? "ABIERTO" : "CERRADO");
            hay = 1;
        }
    }
    if (!hay) printf("  (sin archivos)\n");
}
