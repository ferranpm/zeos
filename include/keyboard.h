#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#define KEYBOARD_BUFF_SIZE 256

struct kbd_buff {
    char buff[KEYBOARD_BUFF_SIZE];
    int a, b;
};

extern struct kbd_buff keyboard_buffer;

extern int remainder_chars;

void keyboard_buffer_init();
int keyboard_buffer_push(unsigned char);
char keyboard_buffer_pop();
int sys_read_keyboard(char *buff, int count);

#endif /* __KEYBOARD_H__ */

