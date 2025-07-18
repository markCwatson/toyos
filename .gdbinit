# .gdbinit file for ToyOS project
# This file is automatically loaded when GDB starts in this directory
# see ~/.config/gdb/gdbinit file

# Load the symbol file
add-symbol-file "./build/kernelfull.o" 0x100000

# Connect to the remote target
target remote localhost:1234

# Set useful breakpoints for networking debugging
# Uncomment these as needed:
# break pci_enumerate_devices
# break rtl8139_hw_start
break rtl8139_interrupt
# break rtl8139_open
# break task_run_first_ever_task
# break process_fork
# break process_load_for_slot
# break process_map_memory
# break process_map_elf
# directory->directory_entry = 0
# break paging_map_to
# ret -1 cuz directory = 0
# break paging_set 
break sys_command15_kill