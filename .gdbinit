# .gdbinit file for specific project
# you must add "add-auto-load-safe-path <path-to-project>/.gdbinit" to the ~/.config/gdb/gdbinit file

# Load the symbol file
add-symbol-file "./build/kernelfull.o" 0x100000

# Connect to the remote target
target remote | qemu-system-i386 -hda ./bin/os.bin -S -gdb stdio
