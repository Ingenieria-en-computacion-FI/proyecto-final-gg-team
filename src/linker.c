#include <stdio.h>
#include <stdlib.h>
#include "../include/object_file.h"
#include "../include/symtab.h"

int main() {
    printf("=========================================\n");
    printf("--- INICIANDO MINI LINKER ---\n");
    printf("=========================================\n");

    // 1. Abrimos el archivo objeto en modo lectura binaria ("rb")
    FILE *obj_file = fopen("output.o", "rb");
    if (!obj_file) {
        printf("Error: No se pudo abrir 'output.o'. ¿Ya corriste el ensamblador?\n");
        return 1;
    }

    // 2. Leemos la cabecera para saber qué contiene el archivo
    ObjectHeader header;
    fread(&header, sizeof(ObjectHeader), 1, obj_file);

    // Verificamos que sea un archivo válido generado por nosotros
    if (header.magic[0] != 'O' || header.magic[1] != 'B' || header.magic[2] != 'J' || header.magic[3] != '1') {
        printf("Error: Archivo objeto corrupto o no reconocido.\n");
        fclose(obj_file);
        return 1;
    }

    printf("Cabecera leída exitosamente:\n");
    printf("  - Firma: %c%c%c%c\n", header.magic[0], header.magic[1], header.magic[2], header.magic[3]);
    printf("  - Tamaño del código: %d bytes\n", header.code_size);
    printf("  - Símbolos exportados: %d\n", header.symbol_count);

    // 3. Extraemos el código máquina puro
    unsigned char *machine_code = (unsigned char *)malloc(header.code_size);
    fread(machine_code, 1, header.code_size, obj_file);

    // 4. Extraemos la tabla de símbolos (En un linker avanzado, aquí se usaría para unir varios .o)
    Symbol *symbols = (Symbol *)malloc(sizeof(Symbol) * header.symbol_count);
    fread(symbols, sizeof(Symbol), header.symbol_count, obj_file);

    printf("\nSímbolos detectados en el archivo objeto:\n");
    for (int i = 0; i < header.symbol_count; i++) {
        printf("  -> %s (en dirección 0x%04X)\n", symbols[i].name, symbols[i].address);
    }

    fclose(obj_file);

    // 5. Generamos el Binario Final (Puro código máquina ejecutable)
    FILE *bin_file = fopen("ejecutable.bin", "wb");
    if (bin_file) {
        fwrite(machine_code, 1, header.code_size, bin_file);
        fclose(bin_file);
        printf("\nLinkeo exitoso. Archivo final generado: 'ejecutable.bin'\n");
    }

    // Liberamos la memoria dinámica
    free(machine_code);
    free(symbols);

    return 0;
}