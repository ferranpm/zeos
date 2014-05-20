#include <keyboard.h>

struct kbd_buff keyboard_buffer;
int primera = 1;

void keyboard_buffer_init() {
    keyboard_buffer.a = 0;
    keyboard_buffer.b = 0;
}

int keyboard_buffer_size() {
    if (keyboard_buffer.a < keyboard_buffer.b)
        return (keyboard_buffer.b - keyboard_buffer.a);
    return (KEYBOARD_BUFF_SIZE - keyboard_buffer.a - keyboard_buffer.b);
}

// Returns -1 if the buffer is full
int keyboard_buffer_push(unsigned char key) {
    int bmu = keyboard_buffer.b + 1 >= KEYBOARD_BUFF_SIZE? 0 : keyboard_buffer.b + 1;
    if (primera) {
        primera = 0;
        keyboard_buffer_init();
    }
    if (bmu == keyboard_buffer.a) return -1; // Buffer ple

    if (++keyboard_buffer.b > KEYBOARD_BUFF_SIZE)
        keyboard_buffer.b = 0;

    keyboard_buffer.buff[keyboard_buffer.b] = key;
}

unsigned char keyboard_buffer_pop() {
    if (keyboard_buffer.a == keyboard_buffer.b)
        return -1; // No hi ha re per llegir
    unsigned char c = keyboard_buffer.buff[keyboard_buffer.a];
    if (++keyboard_buffer.a >= KEYBOARD_BUFF_SIZE)
        keyboard_buffer.a = 0;
    return c;
}

int sys_read_keyboard(int fd, char *buff, int count) {

    int i;
    // TODO: CHECK PARAMS!!!

    /* int available = keyboard_buffer_size(); */
    int read;
    /* if (count < available) read = count; */
    /* // If the buffer is full and count is greater */
    /* // than the buffer size, read all the buffer */
    /* else if (available >= KEYBOARD_BUFF_SIZE-1 && count > available) */
    /*     read = KEYBOARD_BUFF_SIZE - 1; */
    /* else { */
    /*     #<{(| block(keyboardqueue); |)}># */
    /*     #<{(| unblock(keyboardqueue); |)}># */
    /* } */

    printc_xy(10, 10, keyboard_buffer_pop());
    /* for (i = 0; i < read; i++) */
    /*     buff[i] = keyboard_buffer_pop(); */

    return read;
}
