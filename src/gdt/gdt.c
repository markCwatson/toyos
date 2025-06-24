#include "gdt.h"
#include "kernel.h"

/**
 * @brief Encodes a structured GDT entry into a raw GDT format.
 *
 * This function takes a structured GDT entry and converts it into the raw
 * byte format required by the GDT. It encodes the segment limit, base address,
 * and access rights into the target array.
 *
 * @param target Pointer to the array where the encoded GDT entry will be stored.
 * @param source The structured GDT entry containing base, limit, and type.
 */
static void encode_gdt_entry(uint8_t *target, struct gdt_structured source) {
    if ((source.limit > 65536) && ((source.limit & 0xfff) != 0xfff)) {
        // checks limit for alignment
        panick("[encode_gdt_entry] Invalid argument\n");
    }

    // Sets the flags for the limit
    target[6] = 0x40;

    // Adjusts the limit if it is greater than 65536
    if (source.limit > 65536) {
        // Adjusts the limit and sets the granularity flag
        source.limit = source.limit >> 12;
        target[6] = 0xc0;  // With the granularity flag set, the descriptor’s 20‑bit limit is interpreted in 4 KB blocks
    }

    // Encodes the limit
    target[0] = source.limit & 0xff;
    target[1] = (source.limit >> 8) & 0xff;
    target[6] |= (source.limit >> 16) & 0x0f;

    // Encodes the base address
    target[2] = source.base & 0xff;
    target[3] = (source.base >> 8) & 0xff;
    target[4] = (source.base >> 16) & 0xff;
    target[7] = (source.base >> 24) & 0xff;

    // Sets the access rights and flags
    target[5] = source.type;
}

void gdt_structured_to_gdt(struct gdt *gdt, struct gdt_structured *structured_gdt, int total_entries) {
    for (int i = 0; i < total_entries; i++) {
        encode_gdt_entry((uint8_t *)&gdt[i], structured_gdt[i]);
    }
}
