# Reporte Técnico: Implementación de un Ensamblador de Dos Pasadas y Mini Linker para la Arquitectura IA-32

**Autor:** Carlos Alberto Guadarrama Dávila  
**Institución:** Facultad de Ingeniería, Universidad Nacional Autónoma de México  
**Asignatura:** Estructura y Programación de Computadoras  
**Fecha:** Junio 2026  

---

## 1. Introducción y Objetivos

El presente reporte documenta el diseño, la arquitectura y la implementación de un sistema de desarrollo nativo compuesto por un **Ensamblador de Dos Pasadas** y un **Mini Enlazador (Linker)** para un subconjunto de la arquitectura Intel IA-32 (x86 de 32 bits). El sistema ha sido construido íntegramente desde cero en lenguaje C, siguiendo una filosofía de desarrollo modular y prescindiendo de herramientas de generación automática de código como Flex, Bison o Yacc.

### Objetivos del Proyecto:
* Comprender la mecánica interna de la traducción de mnemónicos de lenguaje ensamblador a opcodes binarios ejecutables por un procesador real.
* Implementar un esquema de resolución de **referencias adelantadas (forward references)** mediante un algoritmo de doble pasada sobre el código fuente.
* Diseñar y codificar un motor de empaquetado de bits a nivel de registros para la construcción estructural de los bytes modificadores de direccionamiento: **ModR/M** y **SIB (Scale-Index-Base)**.
* Definir un formato de archivo objeto propietario estructurado para encapsular metadatos, tablas de símbolos y bytes de código máquina.
* Construir un enlazador independiente capaz de validar el archivo objeto, aislar la sección de código ejecutable y generar un binario plano puro (.bin).

---

## 2. Arquitectura General del Sistema

El flujo de procesamiento del sistema sigue una estructura lineal y modular dividida en dos grandes componentes independientes: el Ensamblador (assembler) y el Enlazador (linker). El flujo global de transformación de datos se describe a continuación:

    [ Archivo ASM (.asm) ]
              |
              v
       +-------------+
       |    Lexer    |  ---> Descompone en flujo de tokens lógicos
       +-------------+
              |
              v
       +-------------+
       |   Parser    |  ---> Genera representaciones internas (Instruction)
       +-------------+
              |
          +---+------------------------+
          v                            v
    +-----------+                +-----------+
    |  Pase 1   |                |  Pase 2   |
    |           |                |           |
    | 1. Mapeo  |                | 1. Re-read|
    | 2. SymTab |                | 2. Encoder|
    +-----------+                +-----------+
          |                            |
          +-----------+----------------+
                      v
           [ Archivo Objeto (.o) ]
                      |
                      v
             +-----------------+
             |   Mini Linker   | ---> Valida metadatos y extrae código puro
             +-----------------+
                      |
                      v
           [ Binario Final (.bin) ]
---

## 3. Implementación del Frontend (Lexer y Parser)

### 3.1. Analizador Léxico (Lexer)
El Lexer realiza la lectura carácter por carácter del archivo fuente, agrupando las cadenas de texto en unidades lógicas llamadas `Token`. El lexer ignora comentarios (`;`) y espacios en blanco redundantes. Los tipos de tokens soportados corresponden a la especificación estricta de la arquitectura x86:
* `TOKEN_IDENTIFIER` / `TOKEN_MNEMONIC`: Mnemónicos de instrucción (`MOV`, `ADD`, `JMP`) y directivas (`SECTION`, `DB`).
* `TOKEN_REGISTER`: Registros generales de 32 bits (`EAX`, `ECX`, `EDX`, `EBX`, `ESP`, `EBP`, `ESI`, `EDI`).
* `TOKEN_NUMBER`: Valores numéricos inmediatos o desplazamientos en base 10.
* `TOKEN_LBRACKET` / `TOKEN_RBRACKET`: Delimitadores de indirección de memoria (`[` y `]`).
* Operadores aritméticos (`TOKEN_PLUS`, `TOKEN_STAR`) y delimitadores de sintaxis (`TOKEN_COMMA`, `TOKEN_COLON`, `TOKEN_NEWLINE`).

### 3.2. Analizador Sintáctico (Parser) y Parche de Cero Operandos
El Parser procesa los tokens de forma secuencial para poblar una estructura interna de control denominada `Instruction`. El parser evalúa si una línea contiene una etiqueta (detectando el token `:`), un mnemónico o una directiva.

Un reto crítico de diseño emergió al procesar instrucciones sin operandos como `NOP` (No Operation) o `RET` (Return). Inicialmente, la función de parsing asumía de forma genérica que cualquier palabra clave posterior al mnemónico formaba parte de los operandos. Cuando se encontraba una instrucción de cero operandos seguida de una etiqueta o una nueva instrucción en la línea subsecuente, el parser erróneamente "consumía" el mnemónico de la siguiente línea confundiéndolo con un operando simbólico.

Para solucionar este comportamiento anómalo, se integró una validación condicional explícita que intercepta las instrucciones de cero operandos e interrumpe la fase de parsing de operandos de manera inmediata:

    // Validación sintáctica para instrucciones de 0 operandos
    if (strcmp(instr.mnemonic, "NOP") != 0 && strcmp(instr.mnemonic, "RET") != 0) {
        if (current_token.type == TOKEN_REGISTER || 
            current_token.type == TOKEN_NUMBER || 
            current_token.type == TOKEN_LBRACKET ||
            current_token.type == TOKEN_MNEMONIC) {
            
            parse_operand(&instr.op1);
            instr.operand_count = 1;

            if (current_token.type == TOKEN_COMMA) {
                advance_token();
                parse_operand(&instr.op2);
                instr.operand_count = 2;
            }
        }
    }

Este control garantiza el aislamiento sintáctico de las instrucciones de un solo byte, permitiendo que el flujo léxico continúe sin alterar la semántica de las líneas de código adyacentes.

---

## 4. Algoritmo de Dos Pasadas y Tabla de Símbolos

La resolución de saltos y referencias cruzadas en un ensamblador presenta una problemática intrínseca: el uso de etiquetas antes de su declaración explícita (**Referencias Adelantadas**). Para mitigar este problema sin recurrir a estructuras complejas de re-parcheo dinámico en memoria (*backpatching*), se implementó una arquitectura de **Dos Pasadas independientes**.

### 4.1. Pase 1: Mapeo de Direcciones y Construcción de la Tabla de Símbolos
En la primera iteración, el ensamblador recorre linealmente el archivo con un contador de localidad (`location_counter`) inicializado en cero. El propósito de este pase es calcular el desplazamiento en bytes de cada instrucción y registrar la ubicación de todas las etiquetas del programa.

Las directivas de asignación estática incrementan el contador según el tamaño específico del tipo de dato: `DB` suma 1 byte, `DW` suma 2 bytes, `DD` suma 4 bytes, y `RESB` suma el número de bytes indicado por su operando inmediato. Para las instrucciones lógicas y aritméticas, el sistema interactúa de forma predictiva con el sub-módulo **Encoder** para determinar cuántos bytes ocupará la instrucción en formato máquina (evaluando los modos de direccionamiento detectados). Al completarse el Pase 1, se consolida una tabla global de estructuras `Symbol`, haciendo público el inventario de etiquetas y sus direcciones absolutas de memoria.

### 4.2. Pase 2: Generación Definitiva de Código Máquina
Una vez estructurada la tabla de símbolos, el ensamblador ejecuta la función primitiva `rewind(file)` de la biblioteca estándar de C para restablecer el puntero de lectura al inicio físico del archivo fuente. 

Durante esta segunda pasada, se realiza un nuevo análisis sintáctico. Sin embargo, en este punto, el codificador posee visibilidad total sobre todas las direcciones de memoria del programa. Al encontrarse con instrucciones de salto (`JMP` o `CALL`) orientadas hacia una etiqueta, el sistema busca el símbolo en la tabla ya existente y aplica la ecuación matemática de distancia relativa de la arquitectura x86:

Distancia Relativa = Dirección Destino - (Dirección Actual + Tamaño de la Instrucción)

Los 4 bytes resultantes de esta operación matemática se formatean en formato de ordenamiento de bytes invertido (**Little Endian**) y se empaquetan en el flujo binario final.
---

## 5. Codificación de Instrucciones (Encoder, ModR/M y SIB)

La fase de codificación representa el núcleo matemático del backend. El procesador Intel IA-32 requiere que la información de los registros involucrados y el modo de acceso a la memoria se empaqueten de manera densa en estructuras binarias específicas de un solo byte.

### 5.1. El Byte ModR/M
El byte **ModR/M** (Modificador de Registro/Memoria) subdivide sus 8 bits en tres campos estructurados mediante operadores de desplazamiento aritmético (`<<`) y compuertas lógicas OR (`|`):
* **Mod (Bits 7-6):** Determina el modo de direccionamiento. El valor `11` (3 decimal) le indica al procesador que ambos operandos son registros puros.
* **Reg/Opcode (Bits 5-3):** Almacena el código binario del registro origen de la operación.
* **R/M (Bits 2-0):** Almacena el código binario del registro destino (o modificador de memoria).

El empaquetado exacto se ejecuta en la siguiente función del sistema:

    static unsigned char build_modrm(unsigned char mod, unsigned char reg, unsigned char rm) {
        return (mod << 6) | (reg << 3) | rm; 
    }

### 5.2. El Byte SIB (Scale-Index-Base) Obligatorio
Para expresiones de memoria avanzadas como `[EBX+ECX*4+8]`, la especificación IA-32 dictamina el uso mandatorio de un byte secundario llamado **SIB**. Cuando el parser detecta un operando de tipo memoria indexada y escalada, el campo `R/M` del byte ModR/M se fija estáticamente en el valor clave `4` (`100` binario), señalando al procesador la presencia inmediata del byte SIB.

El byte SIB se estructura de la siguiente manera:
* **Scale (Bits 7-6):** Código binario que representa el multiplicador de escala (1=00, 2=01, 4=10, 8=11).
* **Index (Bits 5-3):** Código numérico del registro que actúa como índice (ej. `ECX`).
* **Base (Bits 2-0):** Código numérico del registro base de la dirección física (ej. `EBX`).

La construcción binaria se realiza mediante el siguiente bloque algorítmico:

    static unsigned char build_sib(unsigned char scale, unsigned char index, unsigned char base) {
        return (scale << 6) | (index << 3) | base;
    }

### 5.3. Optimización de Opcodes Compactos de Un Solo Byte
Para instrucciones unarias que operan directamente sobre registros de propósito general (tales como `PUSH`, `POP`, `INC`, `DEC`), la arquitectura IA-32 permite omitir el uso del byte ModR/M. Para minimizar la huella de memoria del código máquina, los ingenieros de Intel integraron el código identificador del registro (0 a 7) directamente dentro de los bits de menor peso del propio Opcode base de la instrucción.

El encoder implementa este comportamiento sumando el código entero del registro al byte de la operación:

    // Caso de 1 solo operando registro (PUSH, POP, INC, DEC)
    unsigned char reg_code = get_register_code(instr->op1.reg);
    mc.bytes[0] = opcode + reg_code; 
    mc.length = 1;

Esto permite que operaciones como `PUSH EBP` se traduzcan de manera ultra-compacta al byte único `55` (0x50 + 5), optimizando el rendimiento de la caché del procesador.

---

## 6. Formato de Archivo Objeto Personalizado y Enlazador

### 6.1. Estructura del Archivo Objeto Structured (`output.o`)
Un archivo binario plano no contiene metadatos suficientes para que un sistema operativo o un enlazador combine o valide el código. Por ende, se diseñó un formato binario estructurado compuesto por tres secciones secuenciales:

1.  **Cabecera del Objeto (`ObjectHeader`):** Una estructura fija de 12 bytes que encapsula:
    * `magic`: Cadena identificadora de 4 bytes (`'O'`, `'B'`, `'J'`, `'1'`) para validación de firma.
    * `code_size`: Entero de 32 bits que delimita el tamaño exacto de la sección de código máquina puro.
    * `symbol_count`: Número de símbolos grabados en el archivo.
2.  **Sección de Código Máquina (Code Segment):** Los bytes crudos de instrucciones generados durante el Pase 2.
3.  **Sección de Tabla de Símbolos:** Un arreglo estructurado de elementos `Symbol` con los nombres y offsets de las etiquetas detectadas, exportados con el fin de permitir futuras relocaciones o resolución de símbolos externos (`EXTERN`/`GLOBAL`).

Al iniciar la escritura física en disco, el generador realiza un desplazamiento inicial mediante `fseek(file, sizeof(ObjectHeader), SEEK_SET)` para reservar el espacio de la cabecera. Una vez finalizado el Pase 2 y conocidos los valores totales de tamaño del programa, el sistema retorna al byte cero físico, estampa la cabecera real y escribe la tabla de símbolos al final, garantizando la consistencia estructural del binario.

### 6.2. El Mini Linker
El enlazador (`linker`) opera como una entidad de software totalmente independiente del ensamblador. Su función primordial es abrir el archivo objeto estructurado (`output.o`) en modo binario de lectura (`"rb"`), extraer los metadatos de la cabecera y realizar un control de firmas lógicas.

Tras validar que la firma corresponda a `OBJ1`, el linker reserva memoria dinámica (`malloc`) proporcional a la variable `code_size` especificada en la cabecera, lee los bytes de código máquina aislándolos de las estructuras metadata de la tabla de símbolos, y los vuelca de forma compactada en un archivo de salida final denominado `ejecutable.bin`. Este archivo resultante representa un **Flat Binary** puro, libre de cabeceras de compilación, listo para ser cargado directamente en un sector de arranque, un emulador o un entorno de ejecución en modo real de hardware.
---

## 7. Conclusiones y Resultados de Pruebas

La correcta ejecución de las pruebas unitarias integradas en el repositorio valida empíricamente el éxito del diseño lógico:
* La traducción sintáctica y binaria de direccionamientos SIB complejos genera la secuencia hexadecimal exacta dictaminada por la arquitectura Intel (`89 44 8B 08` para `MOV EAX, [EBX+ECX*4+8]`).
* La segmentación del flujo en dos pasadas procesa de manera transparente saltos y bucles iterativos condicionales hacia adelante y hacia atrás, calculando correctamente complementos a dos para offsets negativos (`F2 FF FF FF`).
* La modularidad del código fuente y el uso estricto de estructuras nativas en C sin depender de librerías de parsing automáticas cumple a cabalidad con la filosofía y las restricciones impuestas por la cátedra. El sistema provee una base robusta, extensible y escalable para futuras implementaciones de mecanismos de relocación dinámicos complejos y soporte multitabla para módulos de código distribuidos.