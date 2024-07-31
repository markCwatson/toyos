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

FILES = ./build/echo.o

all: ${FILES}
	i686-elf-gcc -g -T ./linker.ld -o ./echo.elf -ffreestanding -O0 -nostdlib -fpic -g ${FILES} ../stdlib/stdlib.elf

./build/echo.o: ./src/echo.c
	i686-elf-gcc ${INCLUDES} -I./ $(FLAGS) -std=gnu99 -c ./src/echo.c -o ./build/echo.o

clean:
	rm -f ./build/*.o
	rm -f ./*.elf