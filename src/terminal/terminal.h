#ifndef TERMINAL_H
#define TERMINAL_H

#define VGA_WIDTH   80
#define VGA_HEIGHT  20

void terminal_init(void);
void terminal_writechar(char c, char color);

#endif
