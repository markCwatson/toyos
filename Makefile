# This variable is used to enable or disable test-related compilation flags.
RUN_TESTS_FLAG =

# List of object files that need to be linked together to create the kernel.
FILES = ./build/kernel.asm.o \
		./build/kernel.o \
		./build/idt/idt.asm.o \
		./build/stdlib/printf.o \
		./build/idt/idt.o \
		./build/memory/memory.o \
		./build/io/io.asm.o \
		./build/memory/heap/heap.o \
		./build/memory/heap/kheap.o \
		./build/memory/paging/paging.o \
		./build/memory/paging/paging.asm.o \
		./build/disk/disk.o \
		./build/disk/streamer.o \
		./build/terminal/terminal.o \
		./build/fs/file.o \
		./build/fs/path_parser.o \
		./build/fs/fat/fat16.o \
		./build/stdlib/string.o \
		./build/gdt/gdt.asm.o \
		./build/gdt/gdt.o \
		./build/task/tss.asm.o \
		./build/task/task.asm.o \
		./build/task/process.o \
		./build/task/task.o \
		./build/sys/sys.o \
		./build/sys/io/io.o \
		./build/sys/memory/heap.o \
		./build/keyboard/keyboard.o \
		./build/drivers/keyboards/ps2.o \
		./build/drivers/pci/pci.o \
		./build/loader/formats/elf.o \
		./build/loader/formats/elfloader.o \
		./build/sys/task/process.o \
		./build/locks/spinlock.o

# Include paths for the compiler to find header files.
INCLUDES = -I./src

# Compilation flags used for GCC to specify various settings.
# Explanation of each flag:
# -g: Include debugging information in the output. This is useful for debugging purposes.
# -ffreestanding: Indicate that the program does not have access to standard library facilities.
# -falign-jumps, -falign-functions, -falign-labels, -falign-loops: These flags align the specified types of code 
#    structures, which can improve performance on certain architectures by aligning them on memory boundaries.
# -fstrength-reduce: Perform strength reduction optimizations, which replace expensive operations with cheaper ones.
# -fomit-frame-pointer: Do not keep the frame pointer in a register for functions that don't need one, freeing up a register.
# -finline-functions: Enable function inlining, which can improve performance by embedding the function code directly.
# -Wno-unused-function: Suppress warnings about unused static functions.
# -fno-builtin: Do not recognize built-in functions that do not begin with '__builtin_'.
# -Werror: Treat all warnings as errors, which helps to ensure code quality by preventing code with warnings from compiling.
# -Wno-unused-label: Suppress warnings about labels that are defined but not used.
# -Wno-cpp: Suppress warnings about the usage of #warning and #error directives in preprocessor code.
# -Wno-unused-parameter: Suppress warnings about parameters that are set but not used within a function.
# -nostdlib: Do not use the standard system startup files or libraries when linking.
# -nostartfiles: Do not use the standard system startup files when linking.
# -nodefaultlibs: Do not use the standard system libraries when linking.
# -Wall: Enable most warning messages, which helps to catch potential issues in the code.
# -O0: Do not optimize the code. This is useful for debugging because it keeps the compiled output close to the source code.
# -Iinc: Add the 'inc' directory to the list of directories to be searched for header files.
# $(RUN_TESTS_FLAG): A placeholder for additional flags to be set when running tests.
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
        -Iinc \
        $(RUN_TESTS_FLAG)

# The 'all_tests' target sets the RUN_TESTS_FLAG to enable test compilation and then calls 'all'.
all_tests: RUN_TESTS_FLAG = -DRUN_TESTS
all_tests: all

# The 'all' target creates the final os.bin by combining boot.bin and kernel.bin.
# This target produces the complete operating system binary, 'os.bin', which includes both the bootloader and the kernel.
all: ./bin/boot.bin ./bin/kernel.bin user_programs

	# Combines the bootloader and kernel into a single OS image.
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=1048576 count=16 >> ./bin/os.bin

	# Mounts the generated OS image, adds a test user program and test file, the shell program, and then unmounts.
	sudo mkdir /mnt/d
	sudo mount -t vfat ./bin/os.bin /mnt/d
	sudo cp ./programs/test/test.bin /mnt/d
	sudo cp ./programs/shell/shell.elf /mnt/d
	sudo cp ./programs/echo/echo.elf /mnt/d
	sudo cp ./programs/clear/clear.elf /mnt/d
	sudo cp ./programs/ps/ps.elf /mnt/d

	# Create a test file on the mounted OS image.
	touch test.txt
	echo "01234" > test.txt
	sudo cp test.txt /mnt/d
	rm -f test.txt

	sudo umount /mnt/d
	sudo rm -rf /mnt/d

# The 'kernel.bin' target links all object files into a final binary.
./bin/kernel.bin: $(FILES)
	i686-elf-ld -g -relocatable $(FILES) -o ./build/kernelfull.o
	i686-elf-gcc ${FLAGS} -T ./src/linker.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/kernelfull.o -Wl,-Map=./bin/kernel.map

# The 'boot.bin' target assembles the bootloader.
./bin/boot.bin: ./src/boot/boot.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin

# Individual compilation targets for each source file.
./build/kernel.asm.o: ./src/kernel.asm
	nasm -f elf -g ./src/kernel.asm -o ./build/kernel.asm.o

./build/kernel.o: ./src/kernel.c
	i686-elf-gcc ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/kernel.c -o ./build/kernel.o

./build/idt/idt.asm.o: ./src/idt/idt.asm
	nasm -f elf -g ./src/idt/idt.asm -o ./build/idt/idt.asm.o

./build/io/io.asm.o: ./src/io/io.asm
	nasm -f elf -g ./src/io/io.asm -o ./build/io/io.asm.o

./build/idt/idt.o: ./src/idt/idt.c
	i686-elf-gcc ${INCLUDES} -I./src/idt ${FLAGS} -std=gnu99 -c ./src/idt/idt.c -o ./build/idt/idt.o

./build/memory/memory.o: ./src/memory/memory.c
	i686-elf-gcc ${INCLUDES} -I./src/memory ${FLAGS} -std=gnu99 -c ./src/memory/memory.c -o ./build/memory/memory.o

./build/memory/heap/heap.o: ./src/memory/heap/heap.c
	i686-elf-gcc ${INCLUDES} -I./src/memory/heap ${FLAGS} -std=gnu99 -c ./src/memory/heap/heap.c -o ./build/memory/heap/heap.o

./build/memory/heap/kheap.o: ./src/memory/heap/kheap.c
	i686-elf-gcc ${INCLUDES} -I./src/memory/heap ${FLAGS} -std=gnu99 -c ./src/memory/heap/kheap.c -o ./build/memory/heap/kheap.o

./build/memory/paging/paging.o: ./src/memory/paging/paging.c
	i686-elf-gcc ${INCLUDES} -I./src/memory/paging ${FLAGS} -std=gnu99 -c ./src/memory/paging/paging.c -o ./build/memory/paging/paging.o

./build/memory/paging/paging.asm.o: ./src/memory/paging/paging.asm
	nasm -f elf -g ./src/memory/paging/paging.asm -o ./build/memory/paging/paging.asm.o

./build/disk/disk.o: ./src/disk/disk.c
	i686-elf-gcc ${INCLUDES} -I./src/disk ${FLAGS} -std=gnu99 -c ./src/disk/disk.c -o ./build/disk/disk.o

./build/disk/streamer.o: ./src/disk/streamer.c
	i686-elf-gcc ${INCLUDES} -I./src/disk ${FLAGS} -std=gnu99 -c ./src/disk/streamer.c -o ./build/disk/streamer.o

./build/stdlib/string.o: ./src/stdlib/string.c
	i686-elf-gcc ${INCLUDES} -I./src/string ${FLAGS} -std=gnu99 -c ./src/stdlib/string.c -o ./build/stdlib/string.o

./build/fs/path_parser.o: ./src/fs/path_parser.c
	i686-elf-gcc ${INCLUDES} -I./src/fs ${FLAGS} -std=gnu99 -c ./src/fs/path_parser.c -o ./build/fs/path_parser.o

./build/fs/file.o: ./src/fs/file.c
	i686-elf-gcc ${INCLUDES} -I./src/fs ${FLAGS} -std=gnu99 -c ./src/fs/file.c -o ./build/fs/file.o

./build/fs/fat/fat16.o: ./src/fs/fat/fat16.c
	i686-elf-gcc ${INCLUDES} -I./src/fs -I./src/fat ${FLAGS} -std=gnu99 -c ./src/fs/fat/fat16.c -o ./build/fs/fat/fat16.o

./build/terminal/terminal.o: ./src/terminal/terminal.c
	i686-elf-gcc ${INCLUDES} -I./src/terminal ${FLAGS} -std=gnu99 -c ./src/terminal/terminal.c -o ./build/terminal/terminal.o

# Tests disabled because it is causing a runtime error not related to the tests themselves.
# Just the inclusing of the tests.o file is causing the error.
# The filesystem setup fails but not sure why. I think it is related to memory??
# ... in map file, I see size of .text section is 26,995 bytes without tests, and 29,259 bytes with tests (diff of 2,264 bytes)
# ... that memory increase doesn't seem like it should be causing a problem, but maybe it is something else?
# Maybe move these to user programs?
# ./build/tests/tests.o: ./tests/tests.c
# 	i686-elf-gcc ${INCLUDES} -I./tests ${FLAGS} -std=gnu99 -c ./tests/tests.c -o ./build/tests/tests.o

./build/stdlib/printf.o: ./src/stdlib/printf.c
	i686-elf-gcc ${INCLUDES} -I./src/stdlib ${FLAGS} -std=gnu99 -c ./src/stdlib/printf.c -o ./build/stdlib/printf.o

./build/gdt/gdt.asm.o: ./src/gdt/gdt.asm
	nasm -f elf -g ./src/gdt/gdt.asm -o ./build/gdt/gdt.asm.o

./build/gdt/gdt.o: ./src/gdt/gdt.c
	i686-elf-gcc ${INCLUDES} -I./src/gdt ${FLAGS} -std=gnu99 -c ./src/gdt/gdt.c -o ./build/gdt/gdt.o

./build/task/tss.asm.o: ./src/task/tss.asm
	nasm -f elf -g ./src/task/tss.asm -o ./build/task/tss.asm.o
	
./build/task/task.asm.o: ./src/task/task.asm
	nasm -f elf -g ./src/task/task.asm -o ./build/task/task.asm.o

./build/task/task.o: ./src/task/task.c
	i686-elf-gcc ${INCLUDES} -I./src/task ${FLAGS} -std=gnu99 -c ./src/task/task.c -o ./build/task/task.o

./build/task/process.o: ./src/task/process.c
	i686-elf-gcc ${INCLUDES} -I./src/task ${FLAGS} -std=gnu99 -c ./src/task/process.c -o ./build/task/process.o

./build/sys/sys.o: ./src/sys/sys.c
	i686-elf-gcc ${INCLUDES} -I./src/task ${FLAGS} -std=gnu99 -c ./src/sys/sys.c -o ./build/sys/sys.o

./build/sys/io/io.o: ./src/sys/io/io.c
	i686-elf-gcc $(INCLUDES) -I./src/isr80h $(FLAGS) -std=gnu99 -c ./src/sys/io/io.c -o ./build/sys/io/io.o

./build/sys/memory/heap.o: ./src/sys/memory/heap.c
	i686-elf-gcc $(INCLUDES) -I./src/isr80h $(FLAGS) -std=gnu99 -c ./src/sys/memory/heap.c -o ./build/sys/memory/heap.o

./build/keyboard/keyboard.o: ./src/keyboard/keyboard.c
	i686-elf-gcc ${INCLUDES} -I./src/task ${FLAGS} -std=gnu99 -c ./src/keyboard/keyboard.c -o ./build/keyboard/keyboard.o

./build/drivers/keyboards/ps2.o: ./src/drivers/keyboards/ps2.c
	i686-elf-gcc $(INCLUDES) -I./src/drivers/keyboards $(FLAGS) -std=gnu99 -c ./src/drivers/keyboards/ps2.c -o ./build/drivers/keyboards/ps2.o

./build/drivers/pci/pci.o: ./src/drivers/pci/pci.c
	i686-elf-gcc $(INCLUDES) -I./src/drivers/pci $(FLAGS) -std=gnu99 -c ./src/drivers/pci/pci.c -o ./build/drivers/pci/pci.o

./build/loader/formats/elf.o: ./src/loader/formats/elf.c
	i686-elf-gcc $(INCLUDES) -I./src/loader/formats $(FLAGS) -std=gnu99 -c ./src/loader/formats/elf.c -o ./build/loader/formats/elf.o

./build/loader/formats/elfloader.o: ./src/loader/formats/elfloader.c
	i686-elf-gcc $(INCLUDES) -I./src/loader/formats $(FLAGS) -std=gnu99 -c ./src/loader/formats/elfloader.c -o ./build/loader/formats/elfloader.o

./build/sys/task/process.o: ./src/sys/task/process.c
	i686-elf-gcc $(INCLUDES) -I./src/sys/task $(FLAGS) -std=gnu99 -c ./src/sys/task/process.c -o ./build/sys/task/process.o

./build/locks/spinlock.o: ./src/locks/spinlock.c
	i686-elf-gcc $(INCLUDES) -I./src/locks $(FLAGS) -std=gnu99 -c ./src/locks/spinlock.c -o ./build/locks/spinlock.o

user_programs:
	cd ./programs/stdlib && make all
	cd ./programs/test && make all
	cd ./programs/shell && make all
	cd ./programs/echo && make all
	cd ./programs/clear && make all
	cd ./programs/ps && make all

user_programs_clean:
	cd ./programs/stdlib && make clean
	cd ./programs/test && make clean
	cd ./programs/shell && make clean
	cd ./programs/echo && make clean
	cd ./programs/clear && make clean
	cd ./programs/ps && make clean

# The 'clean' target removes all the compiled files and binaries.
clean: user_programs_clean
	rm -rf ./bin/*.bin
	rm -rf ./bin/*.map
	rm -rf ./build/*.o
	rm -rf ./build/*.asm.o
	rm -rf ./build/*/*.o
	rm -rf ./build/*/*.asm.o
	rm -rf ./build/*/*/*.o
	rm -rf ./build/*/*/*.asm.o
