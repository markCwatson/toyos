all:
	nasm -f elf ./blank.asm -o ./build/blank.o
	i686-elf-gcc -g -T ./linker.ld -o ./blank.bin -ffreestanding -O0 -nostdlib -fpic -g ./build/blank.o

clean:
	rm -f ./build/blank.o
	rm -f ./blank.bin