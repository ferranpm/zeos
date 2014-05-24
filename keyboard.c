#include <keyboard.h>
#include <errno.h>
#include <utils.h>

#include <sched.h>

struct kbd_buff keyboard_buffer;
int remainder_chars;

void keyboard_buffer_init() {
    keyboard_buffer.a = 0;
    keyboard_buffer.b = 0;
    remainder_chars = 0;
}

int keyboard_buffer_avail() {
    if (keyboard_buffer.a < keyboard_buffer.b)
        return (keyboard_buffer.b - keyboard_buffer.a);
    else if (keyboard_buffer.a == keyboard_buffer.b)
        return KEYBOARD_BUFF_SIZE;
    return (KEYBOARD_BUFF_SIZE - keyboard_buffer.a + keyboard_buffer.b);
}

int keyboard_buffer_full() {
    return keyboard_buffer_avail() == 0;
}

// Returns -1 if the buffer is full
int keyboard_buffer_push(unsigned char key) {
    if (keyboard_buffer_full()) return -1;
    keyboard_buffer.buff[keyboard_buffer.b] = key;
    keyboard_buffer.b = (keyboard_buffer.b + 1)%KEYBOARD_BUFF_SIZE;
    return 1;
}

char keyboard_buffer_pop() {

    if (keyboard_buffer.a == keyboard_buffer.b)
        return -1; // No hi ha re per llegir

    unsigned char c = keyboard_buffer.buff[keyboard_buffer.a];
    keyboard_buffer.a = (keyboard_buffer.a + 1)%KEYBOARD_BUFF_SIZE;
    /* printc_xy(1,1,'0'+keyboard_buffer.a); */
    return c;
}

int sys_read_keyboard(char *buff, int count) {
    int i;
    int read;
    char available = keyboard_buffer_avail();
    int pos = 0;

    if (!list_empty(&keyboardqueue)) block();

    if (count > available) {
        if (available == 0) {
            for (i = pos; i < available; i++)
                buff[i] = keyboard_buffer_pop();
            pos = available;
            /* count -= available; */
            remainder_chars = count - available;
        }
        block();
        /* printc_xy(1,1,'X'); */
    }

    /* printc_xy(1,1,'0'+available); */
    for (i = pos; i < remainder_chars; i++)
        buff[i] = keyboard_buffer_pop();

    return pos + count;
}
