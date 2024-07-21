#include "path_parser.h"
#include "kernel.h"
#include "config.h"
#include "string/string.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"

static int path_parser_path_valid_format(const char* filename) {
    int len = strnlen(filename, TOYOS_MAX_PATH);

    return (len >= 3 && is_digit(filename[0]) && memcmp((void*)&filename[1], ":/", 2) == 0);
}

static int path_parser_get_drive_by_path(const char** path) {
    if (!path_parser_path_valid_format(*path)) {
        return -EBADPATH;
    }

    return c_to_i(*path[0]);
}

static struct path_root* path_parser_create_root(int drive_number) {
    struct path_root* path_r = kzalloc(sizeof(struct path_root));
    if (!path_r) {
        return NULL;
    }

    path_r->drive_no = drive_number;
    path_r->first = NULL;

    return path_r;
}

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

/*
 * @attention this function modifies the path pointer
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
