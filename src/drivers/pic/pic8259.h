#ifndef PIC8259_H
#define PIC8259_H

#include <stdint.h>

/**
 * @brief Initializes and remaps the PIC controllers.
 *
 * This function remaps the PIC interrupt vectors to avoid conflicts with
 * CPU exceptions (vectors 0-31). The master PIC is remapped to vectors 32-39
 * and the slave PIC to vectors 40-47.
 */
void pic_init(void);

/**
 * @brief Sends End of Interrupt (EOI) signal to the PIC.
 *
 * This function must be called at the end of every hardware interrupt
 * handler to signal that the interrupt has been processed.
 *
 * @param irq The IRQ number (0-15) that needs to be acknowledged
 */
void pic_send_eoi(uint8_t irq);

#endif  // PIC8259_H
