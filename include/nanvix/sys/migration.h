/*
 * MIT License
 *
 * Copyright(c) 2011-2020 The Maintainers of Nanvix
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef NANVIX_SYS_MIGRATION_H_
#define NANVIX_SYS_MIGRATION_H_
    
    #include <nanvix/kernel/syscall.h>

    #define MSTATE_SECTIONS       0
    #define MSTATE_UAREA          1
    #define MSTATE_SYSBOARD       2
    #define MSTATE_PAGEDIR        3
    #define MSTATE_PAGETAB        4
    #define MSTATE_KSTACKSIDS     5
    #define MSTATE_KSTACKSPHYS    6
    #define MSTATE_FRAMES_BITMAP  7
    #define MSTATE_FRAMES_PHYS    8
    #define MSTATE_FINISH         9
    #define MSTATE_INIT           MSTATE_SECTIONS

    #define MIGRATION_MAILBOX_NULL -1
    #define MIGRATION_PORTAL_NULL  -1

    #define MIGRATION_MAILBOX_PORTNUM 0
    #define MIGRATION_PORTAL_PORTNUM  0

    #define MOPCODE_MIGRATE_TO   0
    #define MOPCODE_MIGRATE_FROM 1

    #define NOTIFIED     0
    #define NOT_NOTIFIED 1

    struct migration_message {
        int opcode;
        int receiver;
        int sender;
    };
    
    static inline void migration_message_clear(struct migration_message *msg)
    {
        msg->opcode = -1;
        msg->receiver = -1;
        msg->sender = -1;
    }

    EXTERN int kmigrate_to(int receiver_nodenum);
    EXTERN void kmigration_init(void);

    static inline void kfreeze(void)
    {
        kcall0(NR_freeze);
    }
    
    static inline void kunfreeze(void)
    {
        kcall0(NR_unfreeze);
    }

    static inline int ksysboard_user_syscall_lookup(void)
    {
        return kcall0(NR_user_syscall_lookup);
    }

#endif