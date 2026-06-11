#ifndef OBJECT_FILE_H
#define OBJECT_FILE_H

#include "encoder.h"
#include "symtab.h" // Necesitamos conocer los símbolos

// Estructura de nuestra cabecera personalizada
typedef struct {
    char magic[4];       // Firma para reconocer nuestro archivo (ej. "OBJ1")
    int code_size;       // Tamaño total del código máquina en bytes
    int symbol_count;    // Cuántos símbolos estamos exportando
} ObjectHeader;

void object_file_init(const char *filename);
void object_file_write_code(MachineCode *mc);

// ACTUALIZADO: Ahora le pasaremos el tamaño final al cerrar
void object_file_close(int total_code_size); 

#endif // OBJECT_FILE_H