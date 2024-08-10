#include "spinlock.h"

/**
 * @brief Locks the spinlock
 * 
 * This function locks the spinlock.
 * 
 * __sync_lock_test_and_set() is an atomic operation that sets the value of the
 * lock to 1 and returns the previous value. It is part of the GCC atomic builtins.
 * 
 * @param lock The spinlock to lock.
 */
void spin_lock(struct spinlock_t *lock) {
    while (__sync_lock_test_and_set(&lock->locked, 1));
}

/**
 * @brief Unlocks the spinlock
 * 
 * This function unlocks the spinlock.
 * 
 * @param lock The spinlock to unlock.
 */
void spin_unlock(struct spinlock_t *lock) {
    __sync_lock_release(&lock->locked);
}