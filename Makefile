# This variable is used to enable or disable test-related compilation flags.
RUN_TESTS_FLAG =

# List of object files that need to be linked together to create the kernel.
FILES = ./build/kernel.asm.o \
		./build/kernel.o \
		./build/idt/idt.asm.o \
		./build/stdlib/printf.o \
		./build/tests/tests.o \
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
		./build/string/string.o \
		./build/gdt/gdt.asm.o \
		./build/gdt/gdt.o \
		./build/task/tss.asm.o \
		./build/task/task.asm.o \
		./build/task/process.o \
		./build/task/task.o

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
# Steps:
# 1. Remove any existing os.bin file to ensure a clean build.
# 2. Append the contents of boot.bin to os.bin using the 'dd' command.
#    - 'boot.bin' typically contains the bootloader, which is the first code that runs when the system boots.
# 3. Append the contents of kernel.bin to os.bin using the 'dd' command.
#    - 'kernel.bin' contains the operating system kernel, which is loaded and executed by the bootloader.
# 4. Append zero-filled blocks to the end of os.bin using the 'dd' command with input from /dev/zero.
#    - This step pads the file to a fixed size or ensures it aligns with certain hardware requirements, typically
#      to create space for additional file system data or to meet the minimum disk size.
#    - bs=1048576 sets the block size to 1 MiB, and count=16 specifies that 16 blocks should be added, resulting in
#      16 MiB of zero-padding.
# 5. Builds the user programs and copies them to the mounted OS image.
# 6. Crates a test file on the mounted OS image.
all: ./bin/boot.bin ./bin/kernel.bin user_programs

	# Combines the bootloader and kernel into a single OS image.
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=1048576 count=16 >> ./bin/os.bin

	# Mounts the generated OS image, adds a user program and test file, and then unmounts.
	sudo mkdir /mnt/d
	sudo mount -t vfat ./bin/os.bin /mnt/d
	sudo cp ./programs/blank/blank.bin /mnt/d

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
	i686-elf-gcc ${FLAGS} -T ./src/linker.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/kernelfull.o

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

./build/string/string.o: ./src/string/string.c
	i686-elf-gcc ${INCLUDES} -I./src/string ${FLAGS} -std=gnu99 -c ./src/string/string.c -o ./build/string/string.o

./build/fs/path_parser.o: ./src/fs/path_parser.c
	i686-elf-gcc ${INCLUDES} -I./src/fs ${FLAGS} -std=gnu99 -c ./src/fs/path_parser.c -o ./build/fs/path_parser.o

./build/fs/file.o: ./src/fs/file.c
	i686-elf-gcc ${INCLUDES} -I./src/fs ${FLAGS} -std=gnu99 -c ./src/fs/file.c -o ./build/fs/file.o

./build/fs/fat/fat16.o: ./src/fs/fat/fat16.c
	i686-elf-gcc ${INCLUDES} -I./src/fs -I./src/fat ${FLAGS} -std=gnu99 -c ./src/fs/fat/fat16.c -o ./build/fs/fat/fat16.o

./build/terminal/terminal.o: ./src/terminal/terminal.c
	i686-elf-gcc ${INCLUDES} -I./src/terminal ${FLAGS} -std=gnu99 -c ./src/terminal/terminal.c -o ./build/terminal/terminal.o

./build/tests/tests.o: ./tests/tests.c
	i686-elf-gcc ${INCLUDES} -I./tests ${FLAGS} -std=gnu99 -c ./tests/tests.c -o ./build/tests/tests.o

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

user_programs:
	cd ./programs/blank && make all

user_programs_clean:
	cd ./programs/blank && make clean

# The 'clean' target removes all the compiled files and binaries.
clean: user_programs_clean
	rm -rf ./bin/*.bin
	rm -rf ./build/*.o
	rm -rf ./build/*.asm.o
	rm -rf ./build/*/*.o
	rm -rf ./build/*/*.asm.o
	rm -rf ./build/*/*/*.o
	rm -rf ./build/*/*/*.asm.o
