#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

// Clasificación del tipo de operando
typedef enum {
    OP_NONE,        // Para instrucciones sin operandos (ej. NOP, RET)
    OP_REGISTER,    // Registro a registro
    OP_IMMEDIATE,   // Valor inmediato
    OP_MEMORY,      // Acceso a memoria (cualquier variante con corchetes [])
    OP_LABEL        // Para referencias a etiquetas como JMP MIBUCLE
} OperandType;

// Estructura que define un operando individual
typedef struct {
    OperandType type;
    
    // Si es OP_REGISTER
    char reg[8]; 
    
    // Si es OP_IMMEDIATE
    int immediate_value; 
    
    // Si es OP_MEMORY
    char base_reg[8];    
    char index_reg[8];   
    int scale;           
    int displacement;    
    
    // Si es OP_LABEL
    char label_name[64]; 
    
} Operand;

// Estructura que define una línea de código completa (Instrucción)
typedef struct {
    char label[64];      // <--- ¡AQUÍ ESTÁ EL CAMPO QUE FALTABA!
    char mnemonic[16];   // El mnemónico (ej. "MOV", "ADD")
    int is_directive;
    Operand op1;         // Primer operando (Destino)
    Operand op2;         // Segundo operando (Origen)
    int operand_count;   // Cuántos operandos tiene (0, 1 o 2)
} Instruction;

// Prototipos de funciones
void parser_init();
Instruction parse_next_instruction();

#endif // PARSER_H