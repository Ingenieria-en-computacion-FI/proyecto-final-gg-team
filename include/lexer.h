#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

// Definimos todos los tipos de tokens que nuestro ensamblador va a entender
typedef enum {
    TOKEN_MNEMONIC,    // Ej. MOV, ADD, PUSH
    TOKEN_REGISTER,    // Ej. EAX, EBX, ESP
    TOKEN_NUMBER,      // Ej. 10, 0x1A (Inmediatos)
    TOKEN_LABEL,       // Ej. ETIQUETA:
    TOKEN_DIRECTIVE,   // Ej. SECTION, GLOBAL, DB
    TOKEN_COMMA,       // ,
    TOKEN_LBRACKET,    // [
    TOKEN_RBRACKET,    // ]
    TOKEN_PLUS,        // +
    TOKEN_STAR,        // *
    TOKEN_EOF,         // Fin de archivo
    TOKEN_ERROR        // Error léxico
} TokenType;

// Estructura para almacenar la información de cada token encontrado
typedef struct {
    TokenType type;
    char value[64];    // El texto real, ej: "MOV" o "EAX"
    int line;          // Para saber en qué línea ocurrió un error léxico
} Token;

// Prototipos de funciones
void lexer_init(FILE *file);
Token lexer_get_next_token();

#endif // LEXER_H 