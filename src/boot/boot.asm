ORG 0x7c00                 ; Origin: Set the code's starting point to memory address 0x7C00 (standard bootloader location).
BITS 16                   ; Tell the assembler to produce 16-bit code.

CODE_SEG equ gdt_code - gdt_start  ; Define segment offsets relative to GDT start.
DATA_SEG equ gdt_data - gdt_start

; BIOS Parameter Block
jmp short start            ; Jump to the start label, a standard boot sector entry.
nop                        ; No operation, used for alignment.

; FAT16 Header
; This section describes the file system format and is crucial for the OS to recognize the disk.
OEMIdentifier           db 'TOYOS   '  ; Identifies the system that formatted the volume.
BytesPerSector          dw 0x200       ; Standard sector size (512 bytes).
SectorsPerCluster       db 0x80        ; Number of sectors per cluster.
ReservedSectors         dw 200         ; Reserved sectors (includes bootloader, etc.).
FATCopies               db 0x02        ; Number of File Allocation Tables.
RootDirEntries          dw 0x40        ; Number of entries in the root directory.
NumSectors              dw 0x00        ; Total number of sectors (if zero, use SectorsBig).
MediaType               db 0xF8        ; Media type descriptor.
SectorsPerFat           dw 0x100       ; Number of sectors per FAT.
SectorsPerTrack         dw 0x20        ; Number of sectors per track.
NumberOfHeads           dw 0x40        ; Number of heads in the disk.
HiddenSectors           dd 0x00        ; Number of hidden sectors.
SectorsBig              dd 0x773594    ; Total number of sectors (32-bit, for large volumes).

; Extended BPB (BIOS Parameter Block for DOS 4.0 and higher)
DriveNumber             db 0x80        ; BIOS drive number (0x80 for the first hard disk).
WinNTBit                db 0x00        ; Reserved or used by Windows NT.
Signature               db 0x29        ; Extended boot signature to indicate presence of extended BPB.
VolumeID                dd 0xD105      ; Volume serial number.
VolumeIDString          db 'TOYOSFOOBAR' ; Volume label.
SystemIDString          db 'FAT16   '  ; File system type identifier.

; Bootloader Entry Point
start:
    jmp 0:step2          ; Far jump to 'step2', ensures execution continues from the start of the bootloader code.

step2:
    cli                  ; Clear interrupts to prevent interference during setup.
    mov ax, 0x00         ; Set data segment registers to 0.
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00       ; Set the stack pointer (stack grows downward).
    sti                  ; Enable interrupts.

.load_Protected:
    cli                  ; Disable interrupts again before switching to protected mode.
    lgdt[gdt_descriptor] ; Load the Global Descriptor Table (GDT).
    mov eax, cr0
    or eax, 0x1          ; Set the protected mode bit.
    mov cr0, eax
    jmp CODE_SEG:load32  ; Far jump to switch to protected mode using the new code segment.

; Global Descriptor Table (GDT)
; Defines memory segments for the processor (needed for protected mode).
gdt_start:
gdt_null:                ; Null descriptor (first entry, required to be zeroed).
    dd 0x0
    dd 0x0

gdt_code:                ; Code segment descriptor.
    dw 0xffff            ; Segment limit (low 16 bits).
    dw 0                 ; Base address (low 16 bits).
    db 0                 ; Base address (next 8 bits).
    db 0x9a              ; Access byte (present, ring 0, executable, readable).
    db 11001111b         ; Flags (limit high 4 bits, granularity, 32-bit).
    db 0                 ; Base address (high 8 bits).

gdt_data:                ; Data segment descriptor.
    dw 0xffff
    dw 0
    db 0
    db 0x92              ; Access byte (present, ring 0, writable).
    db 11001111b
    db 0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT size minus 1 (for descriptor size).
    dd gdt_start                ; GDT base address.

[BITS 32]
load32:                     ; Switch to 32-bit code and load the kernel.
    mov eax, 1               ; LBA (Logical Block Addressing) starting sector.
    mov ecx, 100             ; Number of sectors to load.
    mov edi, 0x0100000       ; Memory address to load the kernel to.
    call ata_lba_read        ; Call function to read from disk.
    jmp CODE_SEG:0x0100000   ; Jump to the loaded kernel's entry point.

; ATA LBA Read Function
; Reads data from the disk using ATA in LBA mode.
ata_lba_read:
    mov ebx, eax            ; Backup the LBA value.
    ; Send the highest 8 bits of the LBA to the hard disk controller.
    shr eax, 24
    or eax, 0xe0            ; Select the master drive (0xE0).
    mov dx, 0x1f6
    out dx, al

    ; Send the number of sectors to read.
    mov eax, ecx
    mov dx, 0x1f2
    out dx, al

    ; Send the lower 8 bits of the LBA.
    mov eax, ebx
    mov dx, 0x1f3
    out dx, al

    ; Send the next 8 bits of the LBA.
    mov dx, 0x1f4
    mov eax, ebx
    shr eax, 8
    out dx, al

    ; Send the next 8 bits of the LBA.
    mov dx, 0x1f5
    mov eax, ebx
    shr eax, 16
    out dx, al

    ; Command to read sectors from the disk.
    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

    ; Read the data from the disk into memory.
.next_sector:
    push ecx

.try_again:
    mov dx, 0x1f7
    in al, dx
    test al, 8
    jz .try_again

    mov ecx, 256           ; 256 words (512 bytes) per sector.
    mov dx, 0x1f0
    rep insw               ; Read from port into memory.
    pop ecx
    loop .next_sector      ; Repeat for all sectors.

    ret                    ; Return from the function.

times 510-($ - $$) db 0    ; Pad the boot sector to 510 bytes.
dw 0xAA55                  ; Boot sector signature (required for BIOS to recognize the disk as bootable).
