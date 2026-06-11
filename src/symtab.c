#include <stdio.h>
#include <string.h>
#include "../include/symtab.h"

#define MAX_SYMBOLS 500 // Límite razonable para nuestros programas de prueba

// Nuestro almacenamiento global interno para la tabla
Symbol symbol_table[MAX_SYMBOLS];
int symbol_count = 0;

// Inicializa o limpia la tabla
void symtab_init() {
    symbol_count = 0;
}

// Agrega un nuevo símbolo a la tabla. 
// Retorna 1 si tuvo éxito, o 0 si ya existía o la tabla está llena.
int symtab_add(const char *name, int address) {
    // 1. Verificamos si el símbolo ya existe (para evitar redefiniciones [cite: 195])
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            printf("Error: El símbolo '%s' ya está definido.\n", name);
            return 0; 
        }
    }

    // 2. Si hay espacio, lo guardamos
    if (symbol_count < MAX_SYMBOLS) {
        strcpy(symbol_table[symbol_count].name, name);
        symbol_table[symbol_count].address = address;
        symbol_table[symbol_count].is_defined = 1;
        symbol_count++;
        return 1;
    }

    printf("Error: Tabla de símbolos llena.\n");
    return 0; 
}

// Busca un símbolo por su nombre. Retorna el puntero si lo encuentra, o NULL si no existe.
Symbol* symtab_lookup(const char *name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return &symbol_table[i];
        }
    }
    return NULL; // No se encontró
}
// Imprime el contenido actual de la tabla de símbolos
void symtab_print() {
    printf("\n=== TABLA DE SÍMBOLOS ===\n");
    printf("%-15s | %-10s\n", "ETIQUETA", "DIRECCIÓN");
    printf("-----------------------------------------\n");
    for (int i = 0; i < symbol_count; i++) {
        // Imprimimos la dirección en formato hexadecimal (ej. 0x0000)
        printf("%-15s | 0x%04X\n", symbol_table[i].name, symbol_table[i].address);
    }
    printf("=========================================\n\n");
}