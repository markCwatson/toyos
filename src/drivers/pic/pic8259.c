#include "pic8259.h"
#include "io/io.h"

// PIC I/O port addresses (following OSDev wiki)
#define PIC1 0x20  // IO base address for master PIC
#define PIC2 0xA0  // IO base address for slave PIC
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

// PIC commands
#define PIC_EOI 0x20  // End-of-interrupt command code

// ICW1 (Initialization Command Word 1) bits
#define ICW1_ICW4 0x01       // Indicates that ICW4 will be present
#define ICW1_SINGLE 0x02     // Single (cascade) mode
#define ICW1_INTERVAL4 0x04  // Call address interval 4 (8)
#define ICW1_LEVEL 0x08      // Level triggered (edge) mode
#define ICW1_INIT 0x10       // Initialization - required!

// ICW4 (Initialization Command Word 4) bits
#define ICW4_8086 0x01        // 8086/88 (MCS-80/85) mode
#define ICW4_AUTO 0x02        // Auto (normal) EOI
#define ICW4_BUF_SLAVE 0x08   // Buffered mode/slave
#define ICW4_BUF_MASTER 0x0C  // Buffered mode/master
#define ICW4_SFNM 0x10        // Special fully nested (not)

// OCW3 (Operation Command Word 3) for reading ISR/IRR
#define PIC_READ_IRR 0x0a  // OCW3 irq ready next CMD read
#define PIC_READ_ISR 0x0b  // OCW3 irq service next CMD read

// Default PIC vector offsets (to avoid conflicts with CPU exceptions)
#define PIC_MASTER_OFFSET 0x20  // Master PIC: IRQs 0-7 -> vectors 32-39
#define PIC_SLAVE_OFFSET 0x28   // Slave PIC: IRQs 8-15 -> vectors 40-47

/**
 * @brief Port 0x80 delay for older machines.
 *
 * This provides a small delay that is sometimes necessary for older machines
 * to give the PIC time to react to commands. Port 0x80 is used for
 * "checkpoints" during POST but is unused after boot.
 */
static void io_wait(void) {
    outb(0x80, 0);
}

/**
 * @brief Remaps the PIC controllers to new vector offsets.
 *
 * Reinitialize the PIC controllers, giving them specified vector offsets
 * rather than 8h and 70h, as configured by default.
 *
 * @param offset1 Vector offset for master PIC (vectors become offset1..offset1+7)
 * @param offset2 Vector offset for slave PIC (vectors become offset2..offset2+7)
 */
static void pic_remap(int offset1, int offset2) {
    // Start the initialization sequence (in cascade mode)
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // ICW2: Master PIC vector offset
    outb(PIC1_DATA, offset1);
    io_wait();
    // ICW2: Slave PIC vector offset
    outb(PIC2_DATA, offset2);
    io_wait();

    // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(PIC1_DATA, 4);
    io_wait();
    // ICW3: tell Slave PIC its cascade identity (0000 0010)
    outb(PIC2_DATA, 2);
    io_wait();

    // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore saved masks (or clear them to unmask all interrupts)
    outb(PIC1_DATA, 0x00);  // Unmask all IRQs
    outb(PIC2_DATA, 0x00);  // Unmask all IRQs
}

/**
 * @brief Sends End of Interrupt (EOI) signal to the PIC.
 *
 * This function must be called at the end of every hardware interrupt
 * handler to signal that the interrupt has been processed.
 *
 * @param irq The IRQ number (0-15) that needs to be acknowledged
 */
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

/**
 * @brief Masks (disables) a specific IRQ line.
 *
 * @param irq_line The IRQ number (0-15) to mask
 */
static void irq_set_mask(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;

    if (irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }

    value = insb(port) | (1 << irq_line);
    outb(port, value);
}

/**
 * @brief Unmasks (enables) a specific IRQ line.
 *
 * @param irq_line The IRQ number (0-15) to unmask
 */
static void irq_clear_mask(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;

    if (irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }

    value = insb(port) & ~(1 << irq_line);
    outb(port, value);
}

/**
 * @brief Disables all PIC interrupts by masking all IRQ lines.
 */
static void pic_disable(void) {
    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}

/**
 * @brief Helper function to get IRQ register values.
 *
 * @param ocw3 The OCW3 command to send (PIC_READ_IRR or PIC_READ_ISR)
 * @return 16-bit combined value from both PICs
 */
static uint16_t __pic_get_irq_reg(int ocw3) {
    /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
    outb(PIC1_COMMAND, ocw3);
    outb(PIC2_COMMAND, ocw3);
    return (insb(PIC2_COMMAND) << 8) | insb(PIC1_COMMAND);
}

/**
 * @brief Returns the combined value of the cascaded PICs irq request register.
 *
 * @return 16-bit value representing IRR state for both PICs
 */
static uint16_t pic_get_irr(void) {
    return __pic_get_irq_reg(PIC_READ_IRR);
}

/**
 * @brief Returns the combined value of the cascaded PICs in-service register.
 *
 * @return 16-bit value representing ISR state for both PICs
 */
static uint16_t pic_get_isr(void) {
    return __pic_get_irq_reg(PIC_READ_ISR);
}

void pic_init(void) {
    pic_remap(PIC_MASTER_OFFSET, PIC_SLAVE_OFFSET);
}
