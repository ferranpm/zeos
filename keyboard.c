#include <keyboard.h>
#include <errno.h>
#include <utils.h>
#include <sched.h>

struct keyboard_buffer kbd_buff;

void init_keyboard_buffer() {
    int i;
    for (i = 0; i < KBD_BUFFER_SIZE; i++) {
        kbd_buff.buff[i] = UNKNOWN_KEY;
    }
    kbd_buff.head = 0;
    kbd_buff.tail = 0;
}

/* Returns the number of keys read from the keyboard and
 * available to pass to user space memory
 */
int keyboard_buffer_avail()
{
    int dist = kbd_buff.tail - kbd_buff.head;
    int mask = -(kbd_buff.buff[kbd_buff.head] != UNKNOWN_KEY);
    return (-(kbd_buff.head >= kbd_buff.tail) & mask & KBD_BUFFER_SIZE) + dist;
}

void push_keyboard_buff(unsigned char key)
{
    kbd_buff.buff[kbd_buff.tail++] = key;
    kbd_buff.tail %= KBD_BUFFER_SIZE;
}

void pop_keyboard_buff(int count)
{
    if (kbd_buff.buff[kbd_buff.head] != UNKNOWN_KEY) {
        int i;
        for (i = 0; i < count; i++) {
            kbd_buff.buff[kbd_buff.head++] = UNKNOWN_KEY;
            kbd_buff.head %= KBD_BUFFER_SIZE;
        }
    }
}

/* Device-dependent routine that reads keys from the keyboard */
int sys_read_keyboard(char *buff, int count)
{
    struct task_struct *curr_task_pcb = current();
    int *remainder = &curr_task_pcb->remainder_reads;
    *remainder = count;
    int total_read_keys = 0;

    /* If there are already processes waiting for data, blocks the current
     * process at the end of keyboardqueue and schedule the next process */
    if (!list_empty(&keyboardqueue)) {
        update_stats(current(), RSYS_TO_BLOCKED);
        block_to_keyboardqueue(1);
    }

    while (*remainder > 0) {
        int avail_keys = keyboard_buffer_avail();
        int read_keys_per_iter = 0;
        if (*remainder <= avail_keys || avail_keys == KBD_BUFFER_SIZE) {
            char *start = &kbd_buff.buff[kbd_buff.head];
            int partial_count;
            if (kbd_buff.head >= kbd_buff.tail) {
                partial_count = KBD_BUFFER_SIZE - kbd_buff.head;
                if (copy_to_user(start, buff + total_read_keys, partial_count) < 0) {
                    update_stats(current(), RSYS_TO_RUSER);
                    return -1;
                }
                total_read_keys += partial_count;
                read_keys_per_iter += partial_count;
                *remainder -= partial_count;

                start = &kbd_buff.buff[0];
                partial_count = kbd_buff.tail;

                if (copy_to_user(start, buff + total_read_keys, partial_count) < 0) {
                    update_stats(current(), RSYS_TO_RUSER);
                    return -1;
                }
                total_read_keys += partial_count;
                read_keys_per_iter += partial_count;
                *remainder -= partial_count;
            }
            else {
                partial_count = kbd_buff.tail - kbd_buff.head;
                if (copy_to_user(start, buff + total_read_keys, count) < 0) {
                    update_stats(current(), RSYS_TO_RUSER);
                    return -1;
                }
                total_read_keys += partial_count;
                read_keys_per_iter += partial_count;
                *remainder -= total_read_keys;
            }

            /* Removes read keys from keyboard buffer kbd_buff */
            pop_keyboard_buff(read_keys_per_iter);

            /* If kbd_buff is full, once its whole content has been copied to the user
             * buffer, then blocks the process at the beginning of th keyboardqueue
             */
            if (avail_keys == KBD_BUFFER_SIZE) {
                update_stats(current(), RSYS_TO_BLOCKED);
                block_to_keyboardqueue(0);
            }
        }

        /* If the process requests more keys than those available from the keyboard
         * buffer and it's not full, blocks the process until the buffer will be
         * full or the available keys in keyboard buffer satisfy the process request
         */
        else {
            update_stats(current(), RSYS_TO_BLOCKED);
            block_to_keyboardqueue(0);
        }
    }
    return count;
}

