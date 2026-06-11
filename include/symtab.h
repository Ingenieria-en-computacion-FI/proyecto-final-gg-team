#ifndef SYMTAB_H
#define SYMTAB_H

// Estructura que representa una etiqueta o símbolo en el código
typedef struct {
    char name[64];       // Nombre de la etiqueta (ej. "INICIO", "VARIABLES")
    int address;         // Dirección de memoria donde fue definida (Offset)
    int is_defined;      // Bandera: 1 si ya sabemos dónde está, 0 si es referencia adelantada
} Symbol;

// Prototipos del gestor de símbolos
void symtab_init();
int symtab_add(const char *name, int address);
Symbol* symtab_lookup(const char *name);
void symtab_print();
// Exponemos las variables para que el generador de archivos objeto pueda leerlas
extern Symbol symbol_table[];
extern int symbol_count;
#endif // SYMTAB_H
