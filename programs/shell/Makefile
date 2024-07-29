all:
	nasm -f elf ./shell.asm -o ./build/shell.o
	i686-elf-gcc -g -T ./linker.ld -o ./shell.bin -ffreestanding -O0 -nostdlib -fpic -g ./build/shell.o

clean:
	rm -f ./build/*.o
	rm -f ./*.bin