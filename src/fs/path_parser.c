#include "path_parser.h"
#include "kernel.h"
#include "config.h"
#include "stdlib/string.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"

/**
 * @brief Checks if the given filename has a valid path format.
 *
 * A valid path format starts with a digit representing the drive number,
 * followed by ":/". This function ensures the filename conforms to this pattern.
 *
 * @param filename The path string to check.
 * @return 1 if the path is valid, 0 otherwise.
 */
static int path_parser_path_valid_format(const char* filename) {
    int len = strnlen(filename, TOYOS_MAX_PATH);
    return (len >= 3 && is_digit(filename[0]) && memcmp((void*)&filename[1], ":/", 2) == 0);
}

/**
 * @brief Extracts the drive number from the path.
 *
 * This function assumes that the path has been validated and contains a valid
 * drive number format. It returns the drive number as an integer.
 *
 * @param path A pointer to the path string.
 * @return The drive number, or -EBADPATH if the format is invalid.
 */
static int path_parser_get_drive_by_path(const char** path) {
    if (!path_parser_path_valid_format(*path)) {
        return -EBADPATH;
    }

    return c_to_i(*path[0]);
}

/**
 * @brief Creates a new path_root structure with the specified drive number.
 *
 * This function allocates memory for a path_root structure and initializes it
 * with the given drive number. The `first` field is set to NULL.
 *
 * @param drive_number The drive number to set in the path_root.
 * @return A pointer to the newly created path_root, or NULL if memory allocation fails.
 */
static struct path_root* path_parser_create_root(int drive_number) {
    struct path_root* path_r = kzalloc(sizeof(struct path_root));
    if (!path_r) {
        return NULL;
    }

    path_r->drive_no = drive_number;
    path_r->first = NULL;

    return path_r;
}

/**
 * @brief Extracts the next part of the path.
 *
 * This function reads the next segment of the path up to the next '/' or end of the string.
 * It allocates memory for the path segment and returns it as a string.
 *
 * @param path A pointer to the path string. This pointer is updated to point to the next segment.
 * @return A string representing the next path part, or NULL if allocation fails or the path part is empty.
 */
static const char* path_parser_get_path_part(const char** path) {
    int i = 0;
    char* result_path_part = kzalloc(TOYOS_MAX_PATH);
    if (!result_path_part) {
        return NULL;
    }

    while (**path != '/' && **path != 0x00) {
        result_path_part[i] = **path;
        *path += 1;
        i++;
    }

    if (**path == '/') {
        // Skip the forward slash to avoid problems
        *path += 1;
    }

    if (i == 0) {
        kfree(result_path_part);
        result_path_part = 0;
    }

    return result_path_part;
}

/**
 * @brief Parses a path part and links it to the previous part.
 *
 * This function extracts the next part of the path and creates a new path_part
 * structure for it. The new part is linked to the previous part, if provided.
 *
 * @attention This function modifies the path pointer to advance it.
 *
 * @param last_part The previous path_part to link from.
 * @param path A pointer to the path string. This pointer is updated to point to the next segment.
 * @return A pointer to the newly created path_part, or NULL if parsing or allocation fails.
 */
struct path_part* path_parser_parse_path_part(struct path_part* last_part, const char** path) {
    const char* path_part_str = path_parser_get_path_part(path);
    if (!path_part_str) {
        return NULL;
    }

    struct path_part* part = kzalloc(sizeof(struct path_part));
    if (!part) {
        return NULL;
    }

    part->part = path_part_str;
    part->next = NULL;

    if (last_part) {
        last_part->next = part;
    }

    return part;
}

/**
 * @brief Frees the memory allocated for a parsed path.
 *
 * This function deallocates the memory used by a path_root structure and its associated
 * path_part structures. It should be called to avoid memory leaks after the parsed path
 * is no longer needed.
 *
 * @param root The path_root structure to free.
 */
void path_parser_free(struct path_root* root) {
    struct path_part* part = root->first;

    while (part) {
        struct path_part* next_part = part->next;
        kfree((void*) part->part);
        kfree(part);
        part = next_part;
    }

    kfree(root);
}

/**
 * @brief Parses a file path into its components.
 *
 * This function takes a full file path and optionally a current directory path,
 * and parses it into its individual components, separating the drive, directories,
 * and file name. It returns a path_root structure representing the parsed path.
 *
 * @param path The full file path to parse.
 * @param current_directory_path The current directory path, used if the provided path is relative.
 * @return A pointer to a path_root structure representing the parsed path, or NULL if parsing fails.
 */
struct path_root* path_parser_parse(const char* path, const char* current_directory_path) {
    const char* tmp_path = path;
    struct path_root* path_root = NULL;

    if (strlen(path) > TOYOS_MAX_PATH) {
        goto out;
    }

    int drive_no = path_parser_get_drive_by_path(&tmp_path);
    if (drive_no < 0) {
        goto out;
    }

    path_root = path_parser_create_root(drive_no);
    if (!path_root) {
        goto out;
    }

    // Add 3 bytes to skip drive number 0:/ 1:/ 2:/
    tmp_path += 3;
    struct path_part* first_part = path_parser_parse_path_part(NULL, &tmp_path);
    if (!first_part) {
        goto out;
    }

    path_root->first = first_part;
    struct path_part* part = path_parser_parse_path_part(first_part, &tmp_path);
    while (part) {
        part = path_parser_parse_path_part(part, &tmp_path);
    }
    
out:
    return path_root;
}
