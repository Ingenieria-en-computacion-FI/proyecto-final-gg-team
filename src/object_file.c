#include <stdio.h>
#include <stdlib.h>
#include "../include/object_file.h"

static FILE *obj_file = NULL;

void object_file_init(const char *filename) {
    obj_file = fopen(filename, "wb"); 
    if (!obj_file) {
        printf("Error: No se pudo crear el archivo objeto %s\n", filename);
    } else {
        // Dejamos un espacio en blanco al principio del archivo del tamaño de nuestra cabecera.
        // Lo rellenaremos al final cuando sepamos el tamaño total del código.
        fseek(obj_file, sizeof(ObjectHeader), SEEK_SET);
    }
}

void object_file_write_code(MachineCode *mc) {
    if (obj_file && mc->length > 0) {
        fwrite(mc->bytes, 1, mc->length, obj_file);
    }
}

// ACTUALIZADO: Escribe los metadatos y cierra
void object_file_close(int total_code_size) {
    if (obj_file) {
        // 1. Escribimos la tabla de símbolos al final del archivo
        fwrite(symbol_table, sizeof(Symbol), symbol_count, obj_file);
        
        // 2. Preparamos nuestra cabecera
        ObjectHeader header;
        header.magic[0] = 'O'; header.magic[1] = 'B'; header.magic[2] = 'J'; header.magic[3] = '1';
        header.code_size = total_code_size;
        header.symbol_count = symbol_count;

        // 3. Regresamos al inicio exacto del archivo (el espacio que dejamos vacío)
        fseek(obj_file, 0, SEEK_SET);
        
        // 4. Escribimos la cabecera real
        fwrite(&header, sizeof(ObjectHeader), 1, obj_file);

        fclose(obj_file);
    }
}