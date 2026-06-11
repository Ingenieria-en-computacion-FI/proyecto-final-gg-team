#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "../include/lexer.h"

static FILE *source_file;
static int current_line = 1;
static int current_char = ' ';

// Función interna para avanzar un carácter
static void advance() {
    current_char = fgetc(source_file);
    if (current_char == '\n') {
        current_line++;
    }
}

// Función interna para saltar espacios y tabulaciones
static void skip_whitespace() {
    while (current_char != EOF && isspace(current_char)) {
        advance();
    }
}

// Función interna para ignorar comentarios 
static void skip_comment() {
    if (current_char == ';') {
        while (current_char != EOF && current_char != '\n') {
            advance();
        }
    }
}

// Inicializa el lexer con el archivo abierto
void lexer_init(FILE *file) {
    source_file = file;
    advance(); // Leemos el primer carácter para arrancar
}

// Clasifica una cadena de texto en su tipo de token correspondiente
static TokenType classify_word(const char *word) {
    // Verificar si es un registro válido [cite: 36-51]
    const char *registers[] = {"EAX", "EBX", "ECX", "EDX", "ESI", "EDI", "EBP", "ESP"};
    for (int i = 0; i < 8; i++) {
        if (strcmp(word, registers[i]) == 0) return TOKEN_REGISTER;
    }

    // Verificar si es una directiva válida [cite: 114-136]
    const char *directives[] = {"SECTION", "GLOBAL", "EXTERN", "DB", "DW", "DD", 
                                "ORG", "EQU", "RESB", "RESW", "RESD", ".TEXT", ".DATA", ".BSS"};
    for (int i = 0; i < 14; i++) {
        if (strcmp(word, directives[i]) == 0) return TOKEN_DIRECTIVE;
    }

    // Si no es un registro ni una directiva, asumimos que es un mnemónico (ej. MOV, ADD)
    return TOKEN_MNEMONIC;
}
// Función principal que el Parser llamará para pedir el siguiente token
Token lexer_get_next_token() {
    Token token;
    token.line = current_line;
    token.value[0] = '\0'; // Limpiamos el valor por defecto

    while (current_char != EOF) {
        // Ignorar espacios en blanco
        if (isspace(current_char)) {
            skip_whitespace();
            continue;
        }

        // Ignorar comentarios
        if (current_char == ';') {
            skip_comment();
            continue;
        }

        // --- 1. EXTRACCIÓN DE SÍMBOLOS DE PUNTUACIÓN ---
        // Soportamos los caracteres necesarios para el direccionamiento SIB y operandos [cite: 144-154]
        switch (current_char) {
            case ',':
                token.type = TOKEN_COMMA;
                strcpy(token.value, ",");
                advance();
                return token;
            case '[':
                token.type = TOKEN_LBRACKET;
                strcpy(token.value, "[");
                advance();
                return token;
            case ']':
                token.type = TOKEN_RBRACKET;
                strcpy(token.value, "]");
                advance();
                return token;
            case '+':
                token.type = TOKEN_PLUS;
                strcpy(token.value, "+");
                advance();
                return token;
            case '*':
                token.type = TOKEN_STAR;
                strcpy(token.value, "*");
                advance();
                return token;
        }

        // --- 2. EXTRACCIÓN DE NÚMEROS (Inmediatos, desplazamientos, escalas) ---
        if (isdigit(current_char)) {
            int i = 0;
            // Extraemos todos los dígitos consecutivos
            while (current_char != EOF && isdigit(current_char)) {
                if (i < 63) {
                    token.value[i++] = current_char;
                }
                advance();
            }
            token.value[i] = '\0'; // Terminador de cadena nulo
            token.type = TOKEN_NUMBER;
            return token;
        }

        // --- 3. EXTRACCIÓN DE PALABRAS (Mnemónicos, Registros, Directivas, Etiquetas) ---
        // El '.' se incluye para soportar directivas de sección como ".text" o ".data" [cite: 210, 211]
        // --- 3. EXTRACCIÓN DE PALABRAS (Mnemónicos, Registros, Directivas, Etiquetas) ---
        if (isalpha(current_char) || current_char == '.') { 
            int i = 0;
            while (current_char != EOF && (isalnum(current_char) || current_char == '_' || current_char == '.')) {
                if (i < 63) {
                    token.value[i++] = toupper(current_char); 
                }
                advance();
            }
            token.value[i] = '\0';

            if (current_char == ':') {
                token.type = TOKEN_LABEL;
                advance(); 
                return token;
            }

            // ¡AQUÍ VA LA RECLASIFICACIÓN!
            token.type = classify_word(token.value); 
            return token;
        }

        // --- 4. MANEJO DE ERRORES ---
        token.type = TOKEN_ERROR;
        token.value[0] = current_char;
        token.value[1] = '\0';
        advance();
        return token;
    }

    // Fin de archivo (DEBE QUEDAR ASÍ)
    token.type = TOKEN_EOF;
    strcpy(token.value, "EOF");
    return token;
}