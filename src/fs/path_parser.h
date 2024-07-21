#ifndef PATH_PARSER_H
#define PATH_PARSER_H

/*
    example: 0:/tmp/file.txt

      first -> 0:/
      part  -> tmp/
      next  -> file.txt
*/

struct path_root {
    int drive_no;
    struct path_part* first;
};

struct path_part {
    const char* part;
    struct path_part* next;
};

struct path_root* path_parser_parse(const char* path, const char* current_directory_path);
void path_parser_free(struct path_root* root);

#endif
