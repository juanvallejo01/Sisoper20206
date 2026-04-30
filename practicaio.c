#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_LINEA 256

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