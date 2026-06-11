#ifndef ENCODER_H
#define ENCODER_H

#include "parser.h"

// Estructura para almacenar el código máquina generado de una instrucción
typedef struct {
    unsigned char bytes[15]; // IA-32 tiene un límite máximo de 15 bytes por instrucción
    int length;              // Cuántos bytes ocupa realmente esta instrucción
} MachineCode;

// Prototipos
void encoder_init();
MachineCode encode_instruction(Instruction *instr, int current_address);
#endif // ENCODER_H