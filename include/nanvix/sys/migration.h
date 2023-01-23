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

    #define MSTATE_INIT           0
    #define MSTATE_SECTIONS       1
    #define MSTATE_UAREA          2
    #define MSTATE_THREAD_USTACKS 3
    #define MSTATE_THREAD_KSTACKS 4
    #define MSTATE_PAGEDIR        5
    #define MSTATE_PAGETAB        6
    #define MSTATE_KSTACKSIDS     7
    #define MSTATE_KSTACKSPHYS    8
    #define MSTATE_FRAMES_BITMAP  9
    #define MSTATE_FRAMES_PHYS    10
    #define MSTATE_FINISH         11


    EXTERN int kmigrate_to(int receiver_nodenum);
    
    EXTERN int kmigrate_from(int sender_nodenum);

    EXTERN void kmigration_init(void);

    static inline void kfreeze(void)
    {
        kcall0(NR_freeze);
    }
    
    static inline void kunfreeze(void)
    {
        kcall0(NR_unfreeze);
    }

#endif