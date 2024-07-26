#ifndef _KERNEL_H_
#define _KERNEL_H_

/**
 * @brief Macro to represent an error value as a void pointer.
 *
 * This macro is used to standardize the way errors are represented in the kernel.
 * It allows an integer error code to be safely cast to a void pointer.
 *
 * @param value The error code to convert.
 * @return The error code represented as a void pointer.
 */
#define ERROR(value)    ((void*)(value))

/**
 * @brief Macro to check if a value indicates an error.
 *
 * This macro checks if an integer value represents an error, typically indicated
 * by a negative number.
 *
 * @param value The value to check.
 * @return True if the value represents an error, false otherwise.
 */
#define ISERROR(value)  (((int)(value)) < 0)

/**
 * @brief Macro to convert a void pointer to an integer error code.
 *
 * This macro extracts the integer error code from a void pointer that was
 * previously converted using the ERROR macro.
 *
 * @param value The void pointer to convert.
 * @return The original integer error code.
 */
#define ERROR_I(value)  ((int)(value))

/**
 * @brief Prints a string to the terminal.
 *
 * This function writes each character of the given string to the terminal, using a fixed
 * color attribute. It is typically used for kernel-level logging and debugging.
 *
 * @param str The null-terminated string to print.
 * @param fg The foreground color of the text.
 * @param bg The background color of the text.
 */
void printk(const char* str, unsigned char fg, unsigned char bg);

/**
 * @brief Halts the system and displays a panic message.
 *
 * This function is called when the kernel encounters a critical error from which
 * it cannot recover. It displays the provided message and then halts the system.
 *
 * @param str The null-terminated string describing the panic reason.
 */
void panick(const char* str);

/**
 * @brief Prints an alert message to the terminal.
 * 
 * This function prints an alert message to the terminal using a fixed color attribute.
 * 
 * @param str The null-terminated string to print.
 */
void alertk(const char* str);

#endif
