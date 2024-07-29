all:
	nasm -f elf ./test.asm -o ./build/test.o
	i686-elf-gcc -g -T ./linker.ld -o ./test.bin -ffreestanding -O0 -nostdlib -fpic -g ./build/test.o

clean:
	rm -f ./build/*.o
	rm -f ./*.bin