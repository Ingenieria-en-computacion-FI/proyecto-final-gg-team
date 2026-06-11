#include <stdio.h>
#include <string.h>
#include "../include/encoder.h"
#include "../include/symtab.h" // Importamos la tabla de símbolos para los saltos

void encoder_init() {
    // Aquí inicializaremos tablas complejas si es necesario más adelante
}

// Función auxiliar para buscar el Opcode base según el mnemónico
static unsigned char get_base_opcode(const char *mnemonic) {
    // ---------------------------------------------------------
    // INSTRUCCIONES DE 2 OPERANDOS (Usan ModR/M)
    // ---------------------------------------------------------
    if (strcmp(mnemonic, "ADD") == 0) return 0x01;
    if (strcmp(mnemonic, "SUB") == 0) return 0x29;
    if (strcmp(mnemonic, "AND") == 0) return 0x21;
    if (strcmp(mnemonic, "OR")  == 0) return 0x09;
    if (strcmp(mnemonic, "XOR") == 0) return 0x31;
    if (strcmp(mnemonic, "CMP") == 0) return 0x39;
    if (strcmp(mnemonic, "MOV") == 0) return 0x89;
    
    // ---------------------------------------------------------
    // INSTRUCCIONES DE 1 OPERANDO (Se suman con el registro)
    // ---------------------------------------------------------
    if (strcmp(mnemonic, "INC") == 0) return 0x40; // 0x40 + reg
    if (strcmp(mnemonic, "DEC") == 0) return 0x48; // 0x48 + reg
    if (strcmp(mnemonic, "PUSH") == 0) return 0x50; // 0x50 + reg
    if (strcmp(mnemonic, "POP") == 0) return 0x58; // 0x58 + reg

    // ---------------------------------------------------------
    // SALTOS Y CONTROL
    // ---------------------------------------------------------
    if (strcmp(mnemonic, "JMP") == 0) return 0xE9; 
    if (strcmp(mnemonic, "CALL") == 0) return 0xE8; // Funciona igual que JMP (Relativo)
    if (strcmp(mnemonic, "RET") == 0) return 0xC3;  // Solo 1 byte, no lleva operandos
    if (strcmp(mnemonic, "NOP") == 0) return 0x90;  // Solo 1 byte
    
    return 0x00; // Desconocido
}

// Traduce el nombre del registro a su código IA-32 (3 bits)
static unsigned char get_register_code(const char *reg) {
    if (strcmp(reg, "EAX") == 0) return 0; // 000
    if (strcmp(reg, "ECX") == 0) return 1; // 001
    if (strcmp(reg, "EDX") == 0) return 2; // 010
    if (strcmp(reg, "EBX") == 0) return 3; // 011
    if (strcmp(reg, "ESP") == 0) return 4; // 100
    if (strcmp(reg, "EBP") == 0) return 5; // 101
    if (strcmp(reg, "ESI") == 0) return 6; // 110
    if (strcmp(reg, "EDI") == 0) return 7; // 111
    return 0; // Por defecto
}

// Convierte el valor numérico de la escala a su código de 2 bits
static unsigned char get_scale_code(int scale) {
    if (scale == 1) return 0; // 00
    if (scale == 2) return 1; // 01
    if (scale == 4) return 2; // 10
    if (scale == 8) return 3; // 11
    return 0;
}

// Construye el byte ModR/M empaquetando los bits
static unsigned char build_modrm(unsigned char mod, unsigned char reg, unsigned char rm) {
    // Desplazamos MOD 6 posiciones a la izquierda, REG 3 posiciones, y unimos con OR
    return (mod << 6) | (reg << 3) | rm; 
}

// Construye el byte SIB empaquetando los bits
static unsigned char build_sib(unsigned char scale, unsigned char index, unsigned char base) {
    // Desplazamos scale 6 posiciones, index 3 posiciones, y unimos con OR
    return (scale << 6) | (index << 3) | base;
}

// Genera el código máquina para una instrucción ya parseada
MachineCode encode_instruction(Instruction *instr, int current_address) {
    MachineCode mc;
    memset(&mc, 0, sizeof(MachineCode));
    
    if (instr->is_directive || instr->mnemonic[0] == '\0') {
        return mc;
    }

    // 1. Obtenemos el Opcode Base
    unsigned char opcode = get_base_opcode(instr->mnemonic);
    mc.bytes[0] = opcode;
    mc.length = 1;

    // 2A. Direccionamiento: Registro a Registro
    if (instr->op1.type == OP_REGISTER && instr->op2.type == OP_REGISTER) {
        
        unsigned char mod = 3; // 3 en decimal es 11 en binario (Modo Registro a Registro)
        unsigned char rm_code = get_register_code(instr->op1.reg); // Destino
        unsigned char reg_code = get_register_code(instr->op2.reg); // Origen
        
        // Empaquetamos el byte ModR/M
        mc.bytes[1] = build_modrm(mod, reg_code, rm_code);
        mc.length = 2; // Ahora nuestra instrucción mide 2 bytes
    }
    // 2B. Direccionamiento: Registro a Memoria (SIB)
    else if (instr->op1.type == OP_REGISTER && instr->op2.type == OP_MEMORY) {
        
        // En IA-32, si usamos SIB, el campo r/m del ModR/M siempre es 4 (100 en binario)
        unsigned char rm_code = 4; 
        unsigned char reg_code = get_register_code(instr->op1.reg); // El registro destino
        
        // Asumimos un desplazamiento de 8 bits para este ejemplo (mod = 1)
        unsigned char mod = 1; 
        if (instr->op2.displacement == 0) mod = 0; // Si no hay desplazamiento, mod es 0
        
        // 1. Generamos el ModR/M
        mc.bytes[1] = build_modrm(mod, reg_code, rm_code);
        mc.length = 2;

        // 2. Generamos el SIB si hay un registro base
        if (instr->op2.base_reg[0] != '\0') {
            unsigned char scale_code = get_scale_code(instr->op2.scale);
            unsigned char index_code = get_register_code(instr->op2.index_reg);
            unsigned char base_code  = get_register_code(instr->op2.base_reg);
            
            mc.bytes[2] = build_sib(scale_code, index_code, base_code);
            mc.length = 3;
        }

        // 3. Agregamos el desplazamiento numérico al final
        if (instr->op2.displacement > 0) {
            mc.bytes[mc.length] = (unsigned char)instr->op2.displacement;
            mc.length++;
        }
    }
    // 2C. Direccionamiento: 1 Solo Operando Registro (INC, DEC, PUSH, POP)
    else if (instr->op1.type == OP_REGISTER && instr->op2.type == OP_NONE) {
        // En IA-32, estas instrucciones suman el código del registro al Opcode base
        unsigned char reg_code = get_register_code(instr->op1.reg);
        
        // Modificamos el byte que ya habíamos guardado
        mc.bytes[0] = opcode + reg_code; 
        mc.length = 1; // Miden exactamente 1 byte
    }
    // 2D. Direccionamiento: Saltos a Etiquetas (Relativo)
    else if (instr->op1.type == OP_LABEL) {
        
        mc.length = 5; // Un JMP relativo de 32 bits usa 1 byte de opcode + 4 bytes de distancia
        
        // Buscamos la etiqueta en nuestra tabla de símbolos
        Symbol *sym = symtab_lookup(instr->op1.label_name);
        int offset = 0;
        
        if (sym != NULL) {
            // Etiqueta conocida (Salto hacia atrás). Aplicamos la fórmula matemática.
            offset = sym->address - (current_address + 5);
        } else {
            // Etiqueta desconocida (Referencia Adelantada).
            // Dejamos la distancia en 0. El Pase 2 se encargará de rellenar este "Fixup".
            offset = 0;
        }
        
        // Empaquetamos la distancia de 32 bits usando el formato Little Endian de Intel
        // (El byte menos significativo se guarda primero)
        mc.bytes[1] = offset & 0xFF;
        mc.bytes[2] = (offset >> 8) & 0xFF;
        mc.bytes[3] = (offset >> 16) & 0xFF;
        mc.bytes[4] = (offset >> 24) & 0xFF;
    }
    

    return mc;
}