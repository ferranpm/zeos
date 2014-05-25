/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>
#include <entry.h>
#include <sched.h>
#include <zeos_interrupt.h>
#include <keyboard.h>

Gate idt[IDT_ENTRIES];
Register    idtR;

char char_map[] =
{
    '\0','\0','1','2','3','4','5','6',
    '7','8','9','0','\'','¡','\0','\0',
    'q','w','e','r','t','y','u','i',
    'o','p','`','+','\0','\0','a','s',
    'd','f','g','h','j','k','l','ñ',
    '\0','º','\0','ç','z','x','c','v',
    'b','n','m',',','.','-','\0','*',
    '\0','\0','\0','\0','\0','\0','\0','\0',
    '\0','\0','\0','\0','\0','\0','\0','7',
    '8','9','-','4','5','6','+','1',
    '2','3','0','\0','\0','\0','<','\0',
    '\0','\0','\0','\0','\0','\0','\0','\0',
    '\0','\0'
};

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
    /***********************************************************************/
    /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
    /* ***************************                                         */
    /* flags = x xx 0x110 000 ?????                                        */
    /*         |  |  |                                                     */
    /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
    /*         |   \ DPL = Num. higher PL from which it is accessible      */
    /*          \ P = Segment Present bit                                  */
    /***********************************************************************/
    Word flags = (Word)(maxAccessibleFromPL << 13);
    flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

    idt[vector].lowOffset       = lowWord((DWord)handler);
    idt[vector].segmentSelector = __KERNEL_CS;
    idt[vector].flags           = flags;
    idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
    /***********************************************************************/
    /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
    /* ********************                                                */
    /* flags = x xx 0x111 000 ?????                                        */
    /*         |  |  |                                                     */
    /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
    /*         |   \ DPL = Num. higher PL from which it is accessible      */
    /*          \ P = Segment Present bit                                  */
    /***********************************************************************/
    Word flags = (Word)(maxAccessibleFromPL << 13);

    //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
    /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
       the system calls will be thread-safe. */
    flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

    idt[vector].lowOffset       = lowWord((DWord)handler);
    idt[vector].segmentSelector = __KERNEL_CS;
    idt[vector].flags           = flags;
    idt[vector].highOffset      = highWord((DWord)handler);
}


void setIdt()
{
    /* Program interrups/exception service routines */
    idtR.base  = (DWord)idt;
    idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;

    set_handlers();

    /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
    setInterruptHandler(33, keyboard_handler, 0);
    setInterruptHandler(32, clock_handler, 0);

    setTrapHandler(0x80, system_call_handler, 3);

    set_idt_reg(&idtR);
}

void keyboard_routine()
{
    update_stats(current(), RUSER_TO_RSYS);

    unsigned char key = inb(0x60);
    if (key < 0x80) {
        key = char_map[key];

        /* TODO: Is it needed? */
        /* if (key == '\0') key = 'C'; */

        int avail_keys = keyboard_buffer_avail();
        if (avail_keys < KBD_BUFFER_SIZE) {
            push_keyboard_buff(key);
            avail_keys = keyboard_buffer_avail();
        }
        //printk("Tecles premudes: ");
        //printc_xy(10, 10, avail_keys + '0');
        if (!list_empty(&keyboardqueue)) {
            struct list_head *first = list_first(&keyboardqueue);
            struct task_struct *task_to_unblock = list_head_to_task_struct(first);
            int last_read_req = task_to_unblock->remainder_reads;
            if (last_read_req <= avail_keys || avail_keys == KBD_BUFFER_SIZE) {
                unblock_from_keyboardqueue();
                /* TODO: When unblocks the process, do we add it at the end or
                 * start of readyqueue?
                 */
                
                /* TODO: After unblocks the process, must we perform a
                 * sched_next_rr()?
                 */
            }
        }
    }
    update_stats(current(), RSYS_TO_RUSER);
}

void clock_routine()
{
    update_stats(current(), RUSER_TO_RSYS);

    ++zeos_ticks;

    /* Generic scheduling algorithm */
    update_sched_data_rr();
    if (needs_sched_rr()) {
        update_current_state_rr(&readyqueue);
        sched_next_rr();
    }
    update_stats(current(), RSYS_TO_RUSER);
}

