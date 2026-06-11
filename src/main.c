#include <stdio.h>
#include <string.h>
#include "../include/parser.h"
#include "../include/symtab.h"
#include "../include/encoder.h"
#include "../include/object_file.h"

int main(int argc, char *argv[]) {
    // Si el usuario no escribe el nombre del archivo, le mostramos cómo usarlo
    if (argc < 2) {
        printf("Uso: ./assembler <archivo.asm>\n");
        return 1;
    }

    // Abrimos el archivo que el usuario especificó en la terminal
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        printf("Error: No se pudo abrir el archivo %s\n", argv[1]);
        return 1;
    }

    // PASE 1: CONSTRUCCIÓN DE TABLA DE SÍMBOLOS
    printf("--- INICIANDO PASE 1 (Mapeo de Memoria) ---\n");
    
    lexer_init(file);
    parser_init(); 
    symtab_init(); 
    
    Instruction instr;
    int location_counter = 0; 
    
    do {
        instr = parse_next_instruction();
        
        // Si encontramos una etiqueta, la registramos con la dirección actual
        if (instr.label[0] != '\0') {
            symtab_add(instr.label, location_counter);
        }
        
        // Simulamos la generación para saber cuánto medirá la instrucción y avanzar la memoria
        if (instr.mnemonic[0] != '\0') {
            if (instr.is_directive) {
                if (strcmp(instr.mnemonic, "DB") == 0) location_counter += 1;
                else if (strcmp(instr.mnemonic, "DW") == 0) location_counter += 2;
                else if (strcmp(instr.mnemonic, "DD") == 0) location_counter += 4;
                else if (strcmp(instr.mnemonic, "RESB") == 0) location_counter += instr.op1.immediate_value;
            } else {
                MachineCode mc = encode_instruction(&instr, location_counter);
                location_counter += (mc.length > 0) ? mc.length : 3; 
            }
        }
    } while (instr.label[0] != '\0' || instr.mnemonic[0] != '\0');

    // Imprimimos la tabla para verificar que el Pase 1 fue un éxito
    symtab_print();

    // ==========================================
    // PASE 2: GENERACIÓN DE CÓDIGO MÁQUINA
    // ==========================================
    printf("\n--- INICIANDO PASE 2 (Generación Final) ---\n");
    object_file_init("output.o"); // <--- NUEVO: Creamos el archivo binario
    
    // Regresamos el puntero del archivo al inicio y reiniciamos herramientas
    rewind(file);
    lexer_init(file);
    parser_init();
    location_counter = 0; // La memoria vuelve a empezar en 0

    do {
        instr = parse_next_instruction();
        
        if (instr.mnemonic[0] != '\0') {
            
            if (instr.is_directive) {
                // Las directivas de datos avanzan la memoria
                if (strcmp(instr.mnemonic, "DB") == 0) location_counter += 1;
                else if (strcmp(instr.mnemonic, "DW") == 0) location_counter += 2;
                else if (strcmp(instr.mnemonic, "DD") == 0) location_counter += 4;
                else if (strcmp(instr.mnemonic, "RESB") == 0) location_counter += instr.op1.immediate_value;
                
                printf("[0x%04X] Directiva   : %s\n", location_counter, instr.mnemonic);
            } else {
                // AHORA SÍ: Generamos el código máquina definitivo con símbolos resueltos
                MachineCode mc = encode_instruction(&instr, location_counter);
                object_file_write_code(&mc); // <--- NUEVO: Escribimos los bytes al .o
                // Imprimimos la dirección, la instrucción y los bytes generados
                printf("[0x%04X] %-10s : ", location_counter, instr.mnemonic);
                for (int i = 0; i < mc.length; i++) {
                    printf("%02X ", mc.bytes[i]);
                }
                printf("\n");
                
                location_counter += (mc.length > 0) ? mc.length : 3;
            }
        }
    } while (instr.label[0] != '\0' || instr.mnemonic[0] != '\0');

    fclose(file);
    object_file_close(location_counter); //Cerramos el archivo con el tamaño final del código
    printf("\nEnsamblado exitoso. Código guardado en 'output.o'\n");
    return 0;
}