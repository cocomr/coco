//
// Created by pippo on 14/07/16.
//

#pragma once

#ifdef __linux__

#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(__i386__)

#ifndef __NR_sched_setattr
#define __NR_sched_setattr		351
#endif
#ifndef __NR_sched_getattr
#define __NR_sched_getattr		352
#endif

#elif defined(__x86_64__)

#ifndef __NR_sched_setattr
#define __NR_sched_setattr		314
#endif
#ifndef __NR_sched_getattr
#define __NR_sched_getattr		315
#endif

#endif /* i386 or x86_64 */

#if !defined(__NR_sched_setattr)
# error "Your arch does not support sched_setattr()"
#endif

#if !defined(__NR_sched_getattr)
# error "Your arch does not support sched_getattr()"
#endif

/* If not included in the headers, define sched deadline policy numbe */
#ifndef SCHED_DEADLINE
#define SCHED_DEADLINE		6
#endif


#define sched_setattr(pid, attr, flags) syscall(__NR_sched_setattr, pid, attr, flags)
#define sched_getattr(pid, attr, size, flags) syscall(__NR_sched_getattr, pid, attr, size, flags)

struct sched_attr
{
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    /* SCHED_NORMAL, SCHED_BATCH */
    int32_t sched_nice;
    /* SCHED_FIFO, SCHED_RR */
    uint32_t sched_priority;
    /* SCHED_DEADLINE */
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};


#endif