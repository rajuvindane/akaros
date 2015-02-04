/* Copyright (c) 2015 Google Inc.
 *
 * Dumping ground for converting between Akaros and other OSs. */

#ifndef ROS_KERN_AKAROS_COMPAT_H
#define ROS_KERN_AKAROS_COMPAT_H

/* Common headers that most driver files will need */

#include <assert.h>
#include <error.h>
#include <ip.h>
#include <kmalloc.h>
#include <kref.h>
#include <pmap.h>
#include <slab.h>
#include <smp.h>
#include <stdio.h>
#include <string.h>
#include <bitmap.h>
#include <mii.h>
#include <umem.h>

#define __rcu
typedef unsigned long dma_addr_t;

#define DEFINE_SEMAPHORE(name)  \
    struct semaphore name = SEMAPHORE_INITIALIZER_IRQSAVE(name, 1)
#define sema_init(sem, val) sem_init_irqsave(sem, val)
#define up(sem) sem_up(sem)
#define down(sem) sem_down(sem)
#define down_trylock(sem) ({!sem_trydown(sem);})
/* In lieu of spatching, I wanted to keep the distinction between down and
 * down_interruptible/down_timeout.  Akaros doesn't have the latter. */
#define down_interruptible(sem) ({sem_down(sem); 0;})
#define down_timeout(sem, timeout) ({sem_down(sem); 0;})


#endif /* ROS_KERN_AKAROS_COMPAT_H */