INCLUDES= -I../stdlib/src
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

FILES = ./build/clear.o

all: ${FILES}
	i686-elf-gcc -g -T ./linker.ld -o ./clear.elf -ffreestanding -O0 -nostdlib -fpic -g ${FILES} ../stdlib/stdlib.elf

./build/clear.o: ./src/clear.c
	i686-elf-gcc ${INCLUDES} -I./ $(FLAGS) -std=gnu99 -c ./src/clear.c -o ./build/clear.o

clean:
	rm -f ./build/*.o
	rm -f ./*.elf