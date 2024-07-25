#ifndef PATH_PARSER_H
#define PATH_PARSER_H

/**
 * @brief Structure representing the root of a parsed path.
 *
 * This structure holds the drive number and the first part of the path.
 * For example, in the path "0:/tmp/file.txt", "0:/" would be the first part.
 */
struct path_root {
    int drive_no;                 /**< The drive number (e.g., 0 for "0:/"). */
    struct path_part* first;      /**< Pointer to the first part of the path. */
};

/**
 * @brief Structure representing a part of a parsed path.
 *
 * This structure holds a single part of a path and a pointer to the next part.
 * For example, in the path "0:/tmp/file.txt", "tmp/" and "file.txt" are parts.
 */
struct path_part {
    const char* part;             /**< The current part of the path (e.g., "tmp/"). */
    struct path_part* next;       /**< Pointer to the next part of the path. */
};

/**
 * @brief Parses a file path into its components.
 *
 * This function takes a full file path and optionally a current directory path,
 * and parses it into its individual components, separating the drive, directories,
 * and file name. It returns a `path_root` structure representing the parsed path.
 *
 * @param path The full file path to parse.
 * @param current_directory_path The current directory path, used if the provided path is relative.
 * @return A pointer to a `path_root` structure representing the parsed path, or NULL if parsing fails.
 */
struct path_root* path_parser_parse(const char* path, const char* current_directory_path);

/**
 * @brief Frees the memory allocated for a parsed path.
 *
 * This function deallocates the memory used by a `path_root` structure and its associated
 * `path_part` structures. It should be called to avoid memory leaks after the parsed path
 * is no longer needed.
 *
 * @param root The `path_root` structure to free.
 */
void path_parser_free(struct path_root* root);

#endif
