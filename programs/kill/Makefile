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

FILES = ./build/kill.o

all: ${FILES}
	i686-elf-gcc -g -T ./linker.ld -o ./kill.elf -ffreestanding -O0 -nostdlib -fpic -g ${FILES} ../stdlib/stdlib.elf

./build/kill.o: ./src/kill.c
	i686-elf-gcc ${INCLUDES} -I./ $(FLAGS) -std=gnu99 -c ./src/kill.c -o ./build/kill.o

clean:
	rm -f ./build/*.o
	rm -f ./*.elf