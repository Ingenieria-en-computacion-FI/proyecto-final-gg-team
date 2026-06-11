# Compilador y banderas
CC = gcc
CFLAGS = -Wall -g

# Archivos fuente
ASM_SRCS = src/main.c src/lexer.c src/parser.c src/symtab.c src/encoder.c src/object_file.c
LINKER_SRCS = src/linker.c

# Nombres de los ejecutables finales
ASM_BIN = assembler
LINKER_BIN = linker

# Regla principal (compila todo)
all: $(ASM_BIN) $(LINKER_BIN)

# Cómo compilar el ensamblador
$(ASM_BIN): $(ASM_SRCS)
	$(CC) $(CFLAGS) -o $(ASM_BIN) $(ASM_SRCS)

# Cómo compilar el mini linker
$(LINKER_BIN): $(LINKER_SRCS)
	$(CC) $(CFLAGS) -o $(LINKER_BIN) $(LINKER_SRCS)

# Regla para limpiar archivos generados
clean:
	rm -f $(ASM_BIN) $(LINKER_BIN) output.o ejecutable.bin

.PHONY: all clean