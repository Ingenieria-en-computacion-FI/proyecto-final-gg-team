# Bitácora de Uso de Inteligencia Artificial
**Proyecto:** IA-32 Assembler & Mini Linker
**Fecha:** Junio 2026

## 1. Organización del Equipo
* **Integrante 1 (PM):** Carlos Alberto Guadarrama Dávila
* **Integrante 2:** [Nombre]
* **Integrante 3:** [Nombre]
* **Integrante 4:** [Nombre]
* **Integrante 5:** [Nombre]

---

## 2. Filosofía y Herramientas Utilizadas
Para cumplir con el rigor académico de la materia, el equipo determinó que ninguna herramienta de IA (como Gemini) se utilizaría para la generación masiva de código o la creación de módulos completos desde cero. 

Las IA se utilizaron exclusivamente como **herramientas de consulta técnica y validación**, enfocándose en:
1.  Comprensión de la documentación técnica de la arquitectura IA-32 (estructuras de Opcodes).
2.  Validación de operaciones matemáticas a nivel de bits (Bitwise) para la construcción de los bytes ModR/M y SIB.
3.  Asistencia en la depuración (debugging) de errores de segmentación y lógica de punteros en C.

---

## 3. Registro de Interacciones y Prompts Destacados

### Caso 1: Comprensión del Byte SIB (Scale-Index-Base)
* **Objetivo:** Implementar la lógica para traducir operandos de memoria complejos como `[EBX+ECX*4+8]`.
* **Prompt principal utilizado:** *"¿Cómo se empaquetan a nivel de bits los valores de Scale, Index y Base para formar el byte SIB en la arquitectura IA-32 en C?"*
* **Código generado por IA:** La IA proporcionó un ejemplo aislado usando desplazamientos lógicos `(scale << 6) | (index << 3) | base`.
* **Modificaciones realizadas:** El equipo no copió una función completa. Tomamos la fórmula de empaquetado de bits y la integramos manualmente dentro de nuestra función `encode_instruction`, adaptándola a nuestra estructura personalizada `Instruction` y a nuestros propios diccionarios de registros.

### Caso 2: Arquitectura del Ensamblador de 2 Pasadas
* **Objetivo:** Resolver el problema de las referencias adelantadas (`Forward References`) en los saltos `JMP`.
* **Prompt principal utilizado:** *"¿Cuál es la mejor práctica en C para regresar el puntero de lectura al inicio de un archivo para implementar el Pase 2 de un ensamblador, conservando la tabla de símbolos en memoria?"*
* **Información obtenida:** Se nos sugirió el uso de la función estándar `rewind(file)` de la biblioteca `<stdio.h>`.
* **Modificaciones realizadas:** Diseñamos la lógica del ciclo `do-while` en `main.c` por nuestra cuenta, separando la lógica de la directiva de la lógica del encoder, utilizando `rewind` como puente entre ambas pasadas.

### Caso 3: Debugging del Parser (Instrucciones sin operandos)
* **Objetivo:** Corregir un bug donde el ensamblador se saltaba instrucciones al procesar mnemónicos de cero operandos como `NOP` o `RET`.
* **Proceso de IA:** Se le proporcionó a la IA la salida de la terminal (`[0x000C] NOP : 90 00 00 00 00`) junto con el fragmento problemático de nuestro `parser.c`.
* **Análisis de la IA:** La IA identificó que la función `parse_operand` asumía por defecto que la siguiente línea pertenecía a los operandos si la instrucción actual no llevaba ninguno, consumiendo el token incorrecto.
* **Solución aplicada:** El equipo implementó una validación condicional `if (strcmp(instr.mnemonic, "NOP") != 0 && ...)` para aislar estas instrucciones de un solo byte y evitar la lectura secuencial de operandos inexistentes.

---

## 4. Conclusión del Uso de IA
El uso de modelos de lenguaje fue fundamental para acelerar la comprensión de la aritmética de punteros y los desplazamientos a nivel de bits, reduciendo el tiempo de investigación en manuales antiguos de Intel. Sin embargo, **el 100% del diseño de las estructuras de datos (`Token`, `Symbol`, `Instruction`, `MachineCode`), el flujo de integración y el formateo de los archivos objeto fue desarrollado de manera manual** por los integrantes del equipo para garantizar una comprensión profunda de los procesos internos de un compilador.