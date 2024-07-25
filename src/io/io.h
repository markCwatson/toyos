#ifndef IO_H
#define IO_H

/**
 * @brief Reads a byte from the specified port.
 * 
 * @param port The port to read from.
 * @return The byte read from the port.
 */
unsigned char insb(unsigned short port);

/**
 * @brief Reads a word (2 bytes) from the specified port.
 * 
 * @param port The port to read from.
 * @return The word read from the port.
 */
unsigned short insw(unsigned short port);

/**
 * @brief Writes a byte to the specified port.
 * 
 * @param port The port to write to.
 * @param val The byte value to write.
 */
void outb(unsigned short port, unsigned char val);

/**
 * @brief Writes a word (2 bytes) to the specified port.
 * 
 * @param port The port to write to.
 * @param val The word value to write.
 */
void outw(unsigned short port, unsigned short val);

#endif
