#ifndef KERNEL_H
#define KERNEL_H

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
 * @brief Prints a string to the kernel console.
 *
 * This function is used for displaying messages on the kernel's console,
 * typically for debugging or logging purposes.
 *
 * @param str The null-terminated string to print.
 */
void printk(const char* str);

/**
 * @brief Halts the system and displays a panic message.
 *
 * This function is called when the kernel encounters a critical error from which
 * it cannot recover. It displays the provided message and then halts the system.
 *
 * @param str The null-terminated string describing the panic reason.
 */
void panick(const char* str);

#endif
