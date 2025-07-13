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

FILES = ./build/forkdemo.o

all: ${FILES}
	i686-elf-gcc -g -T ./linker.ld -o ./forkdemo.elf -ffreestanding -O0 -nostdlib -fpic -g ${FILES} ../stdlib/stdlib.elf

./build/forkdemo.o: ./src/forkdemo.c
	i686-elf-gcc ${INCLUDES} -I./ $(FLAGS) -std=gnu99 -c ./src/forkdemo.c -o ./build/forkdemo.o

clean:
	rm -f ./build/*.o
	rm -f ./*.elf