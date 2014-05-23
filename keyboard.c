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
    return (KEYBOARD_BUFF_SIZE - keyboard_buffer.a - keyboard_buffer.b);
}

int keyboard_buffer_full() {
    return keyboard_buffer_avail() == KEYBOARD_BUFF_SIZE;
}

// Returns -1 if the buffer is full
int keyboard_buffer_push(unsigned char key) {
    int bmu = (keyboard_buffer.b + 1)%KEYBOARD_BUFF_SIZE;
    if (bmu == keyboard_buffer.a) return -1; // Buffer ple

    keyboard_buffer.buff[keyboard_buffer.b] = key;

    if (++keyboard_buffer.b > KEYBOARD_BUFF_SIZE)
        keyboard_buffer.b = 0;
}

char keyboard_buffer_pop() {
    if (keyboard_buffer.a == keyboard_buffer.b)
        return -1; // No hi ha re per llegir
    unsigned char c = keyboard_buffer.buff[keyboard_buffer.a];
    if (++keyboard_buffer.a >= KEYBOARD_BUFF_SIZE)
        keyboard_buffer.a = 0;
    return c;
}

int sys_read_keyboard(char *buff, int count) {
    int i;
    int read;
    char available = keyboard_buffer_avail();
    int pos = 0;

    if (!list_empty(&keyboardqueue)) block();

    if (count > available) {
        if (available == KEYBOARD_BUFF_SIZE) {
            for (i = pos; i < available; i++)
                buff[i] = keyboard_buffer_pop();
            pos = available;
            count -= available;
            remainder_chars = count;
        }
        block();
    }

    for (i = pos; i < count; i++)
        buff[i] = keyboard_buffer_pop();

    return pos + count;
}
