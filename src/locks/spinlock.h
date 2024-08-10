#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <stdint.h>

/**
 * @brief Spinlock structure
 * 
 * This structure represents a spinlock.
 * 
 * @var locked The lock status.
 */
struct spinlock_t {
    volatile uint32_t locked;
};

/**
 * @brief Locks the spinlock
 * 
 * This function locks the spinlock.
 * 
 * @param lock The spinlock to lock.
 */
void spin_lock(struct spinlock_t *lock);

/**
 * @brief Unlocks the spinlock
 * 
 * This function unlocks the spinlock.
 * 
 * @param lock The spinlock to unlock.
 */
void spin_unlock(struct spinlock_t *lock);

#endif
