#ifndef _GDT_H
#define _GDT_H

#include <stdint.h>

/**
 * @struct gdt
 * @brief Represents a Global Descriptor Table (GDT) entry in raw format.
 *
 * This structure is used to define a segment descriptor in the GDT, which
 * includes information about the segment's base address, limit, access rights,
 * and other properties.
 */
struct gdt {
    uint16_t segment;           /**< Segment limit (bits 0-15) */
    uint16_t base_first;        /**< Base address (bits 0-15) */
    uint8_t base;               /**< Base address (bits 16-23) */
    uint8_t access;             /**< Access byte defines the segment's access rights */
    uint8_t high_flags;         /**< High flags and limit (bits 16-19) */
    uint8_t base_24_31_bits;    /**< Base address (bits 24-31) */
} __attribute__((packed));

/**
 * @struct gdt_structured
 * @brief Represents a GDT entry in a structured format.
 *
 * This structure is used for easier manipulation and definition of GDT entries.
 * It abstracts the raw format into base address, limit, and type fields.
 */
struct gdt_structured {
    uintptr_t base;  /**< Base address of the segment */
    uint32_t limit;  /**< Limit of the segment */
    uint8_t type;    /**< Segment type and access flags */
};

/**
 * @brief Loads the GDT with the given entries.
 *
 * This function sets up the GDT with the entries provided in the `gdt` array and loads it.
 *
 * @param gdt Pointer to an array of GDT entries.
 * @param size The size of the GDT in bytes.
 */
void gdt_load(struct gdt* gdt, int size);

/**
 * @brief Converts structured GDT entries into raw GDT format.
 *
 * This function takes an array of structured GDT entries and converts them
 * into the raw GDT format that the hardware expects.
 *
 * @param gdt Pointer to the destination array of raw GDT entries.
 * @param structured_gdt Pointer to the source array of structured GDT entries.
 * @param total_entries The total number of GDT entries to convert.
 */
void gdt_structured_to_gdt(struct gdt* gdt, struct gdt_structured* structured_gdt, int total_entries);

#endif
