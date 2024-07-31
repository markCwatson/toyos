INCLUDES=-I./src
FLAGS = -g \
		-ffreestanding \
		-falign-jumps \
		-falign-functions \
		-falign-labels \
		-falign-loops \
		-fstrength-reduce \
		-fomit-frame-pointer \
		-finline-functions \
		-Wno-unused-function \
		-fno-builtin \
		-Werror \
		-Wno-unused-label \
		-Wno-cpp \
		-Wno-unused-parameter \
		-nostdlib \
		-nostartfiles \
		-nodefaultlibs \
		-Wall \
		-O0 \
		-Iinc

FILES = ./build/start.o \
		./build/toyos.o \
		./build/toyos.asm.o \
		./build/stdlib.o \
		./build/stdio.o

all: ${FILES}
	i686-elf-ld -m elf_i386 -relocatable ${FILES} -o ./stdlib.elf

./build/start.o: ./src/start.asm
	nasm -f elf ./src/start.asm -o ./build/start.o

./build/toyos.o: ./src/toyos.c
	nasm -f elf ./src/toyos.c -o ./build/toyos.o

./build/toyos.asm.o: ./src/toyos.asm
	nasm -f elf ./src/toyos.asm -o ./build/toyos.asm.o

./build/toyos.o: ./src/toyos.c
	i686-elf-gcc ${INCLUDES} $(FLAGS) -std=gnu99 -c ./src/toyos.c -o ./build/toyos.o

./build/stdlib.o: ./src/stdlib.c
	i686-elf-gcc ${INCLUDES} $(FLAGS) -std=gnu99 -c ./src/stdlib.c -o ./build/stdlib.o

./build/stdio.o: ./src/stdio.c
	i686-elf-gcc ${INCLUDES} $(FLAGS) -std=gnu99 -c ./src/stdio.c -o ./build/stdio.o

clean:
	rm -rf ./build/*.o
	rm -rf ./*.elf
