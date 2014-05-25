#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

/* TODO: Is it better to use a specific keyboard header file for it
 * or we use device header file instead?
 */

#define KBD_BUFFER_SIZE 256
#define UNKNOWN_KEY 0x80

struct keyboard_buffer {
    unsigned char buff[KBD_BUFFER_SIZE];
    int head;
    int tail;
};

extern struct keyboard_buffer kbd_buff;

void init_keyboard_buffer();
int keyboard_buffer_avail();
void push_keyboard_buff(unsigned char key);
void pop_keyboard_buff(int count);
int sys_read_keyboard(char *buff, int count);

#endif /* __KEYBOARD_H__ */
