#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/parser.h"

// Mantenemos el token actual que estamos analizando
static Token current_token;

// Función interna para pedirle el siguiente token al lexer
static void advance_token() {
    current_token = lexer_get_next_token();
}

// Inicializa el parser obteniendo el primer token
void parser_init() {
    advance_token(); 
}

// Prototipo de la función interna que leerá un operando individual
static void parse_operand(Operand *op);

// Función principal: Lee una instrucción completa línea por línea
Instruction parse_next_instruction() {
    Instruction instr;
    
    // ¡EL SALVAVIDAS! Llenamos toda la estructura con ceros.
    // Esto asegura que cadenas estén vacías y operandos nazcan en OP_NONE (0).
    memset(&instr, 0, sizeof(Instruction)); 

    // Si llegamos al final o hay un error léxico, retornamos vacío
    if (current_token.type == TOKEN_EOF || current_token.type == TOKEN_ERROR) {
        return instr;
    }

    // 1. ESPERAMOS UNA ETIQUETA (Opcional)
    if (current_token.type == TOKEN_LABEL) {
        strcpy(instr.label, current_token.value);
        advance_token();
    }

    // 2. ESPERAMOS UN MNEMÓNICO (Ej. MOV, ADD)
    if (current_token.type == TOKEN_MNEMONIC) {
        strcpy(instr.mnemonic, current_token.value);
        instr.is_directive = 0; // Es instrucción normal
        advance_token();
    } else if (current_token.type == TOKEN_DIRECTIVE) {
        strcpy(instr.mnemonic, current_token.value);
        instr.is_directive = 1; // <-- ¡NUEVO! Es una directiva
        advance_token();
    } else {
        return instr; 
    }

    // 3. ESPERAMOS EL PRIMER OPERANDO (Destino)
    if (strcmp(instr.mnemonic, "NOP") != 0 && strcmp(instr.mnemonic, "RET") != 0) {
        
        if (current_token.type == TOKEN_REGISTER || 
            current_token.type == TOKEN_NUMBER || 
            current_token.type == TOKEN_LBRACKET ||
            current_token.type == TOKEN_MNEMONIC ||
            current_token.type == TOKEN_DIRECTIVE) { // <--- ¡AQUÍ ESTÁ LA SOLUCIÓN!
            
            parse_operand(&instr.op1);
            instr.operand_count = 1;

            // 4. ESPERAMOS UNA COMA Y EL SEGUNDO OPERANDO
            if (current_token.type == TOKEN_COMMA) {
                advance_token(); // Consumimos la coma
                parse_operand(&instr.op2);
                instr.operand_count = 2;
            }
        }
    }

    return instr;
}

// Función que analiza qué tipo de operando tenemos enfrente
static void parse_operand(Operand *op) {
    op->type = OP_NONE; // Inicializamos por defecto

    // Caso A: Operando de Registro (Ej. EAX)
    if (current_token.type == TOKEN_REGISTER) {
        op->type = OP_REGISTER;
        strcpy(op->reg, current_token.value);
        advance_token();
    } 
    // Caso B: Operando Inmediato (Ej. 10)
    else if (current_token.type == TOKEN_NUMBER) {
        op->type = OP_IMMEDIATE;
        op->immediate_value = atoi(current_token.value); // Convertimos el texto a número entero
        advance_token();
    }
    // Caso C: Operando de Memoria (Ej. [EBX+ECX*4+8])
    else if (current_token.type == TOKEN_LBRACKET) {
        op->type = OP_MEMORY;
        advance_token(); // Consumimos el '['
        
        // Inicializamos los valores por defecto
        op->base_reg[0] = '\0';
        op->index_reg[0] = '\0';
        op->scale = 1;
        op->displacement = 0;

        // 1er Elemento: Puede ser un número [1000], una variable [MIVARIABLE] o registro [EBX...]
        if (current_token.type == TOKEN_NUMBER) {
            op->displacement = atoi(current_token.value);
            advance_token();
        } 
        else if (current_token.type == TOKEN_MNEMONIC) { // <-- ¡NUEVO! Para variables
            strcpy(op->label_name, current_token.value);
            advance_token();
        }
        else if (current_token.type == TOKEN_REGISTER) {
            strcpy(op->base_reg, current_token.value);
            advance_token();

            // Ciclo para procesar todo lo que esté sumado (Índice y/o Desplazamiento)
            // (El resto de tu lógica del TOKEN_PLUS se queda igual...)

            // Ciclo para procesar todo lo que esté sumado (Índice y/o Desplazamiento)
            while (current_token.type == TOKEN_PLUS) {
                advance_token(); // Consumimos el '+'

                // Si después del '+' hay un número, es el desplazamiento final
                if (current_token.type == TOKEN_NUMBER) {
                    op->displacement = atoi(current_token.value);
                    advance_token();
                } 
                // Si después del '+' hay otro registro, es el registro índice
                else if (current_token.type == TOKEN_REGISTER) {
                    strcpy(op->index_reg, current_token.value);
                    advance_token();

                    // Revisamos si el índice tiene una escala multiplicadora (Ej. *4)
                    if (current_token.type == TOKEN_STAR) {
                        advance_token(); // Consumimos el '*'
                        if (current_token.type == TOKEN_NUMBER) {
                            op->scale = atoi(current_token.value);
                            
                            // Validación de seguridad: Soportar solo las escalas permitidas [cite: 157-165]
                            if (op->scale != 1 && op->scale != 2 && op->scale != 4 && op->scale != 8) {
                                printf("Error Sintáctico: Escala %d no válida. Debe ser 1, 2, 4 u 8.\n", op->scale);
                            }
                            advance_token();
                        }
                    }
                }
            }
        }

        // Finalmente, verificamos que la expresión cierre correctamente
        if (current_token.type == TOKEN_RBRACKET) {
            advance_token(); // Consumimos el ']'
        } else {
            printf("Error Sintáctico: Se esperaba ']'\n");
        }
    }
    // Caso D: Operando de Etiqueta o Nombre de Sección (Ej. JMP MIBUCLE o SECTION .text)
    else if (current_token.type == TOKEN_MNEMONIC || current_token.type == TOKEN_DIRECTIVE) { // <-- ¡NUEVO!
        op->type = OP_LABEL;
        strcpy(op->label_name, current_token.value);
        advance_token();
    }
}