#ifndef _TASKS_H
#define _TASKS_H

/* This two MUST BE EQUAL and POWER OF TWO !!! */
#define THREAD_KSTACK_SIZE          8192
#define THREAD_KINFO_SIZE           THREAD_KSTACK_SIZE

#define THREAD_ESP_TO_PCB           THREAD_KINFO_SIZE - 1

#ifndef ASM

#include <stddef.h>
#include <stdint.h>

typedef struct
{
} process_pd;

// because we saves all register states in kernel stack,
//   here we do not repeat those states.
typedef struct _thread_pcb_t
{
    // Kernel stack state of current thread.
    uint32_t* esp0;
    process_pd* to_process;
    
    // Following is a simple list used by "scheduling"
    //    Simplest scheduling: process is paused and the next process
    //    to be executed is stored as current pcb->next.
    struct _thread_pcb_t *next, *prev;
} thread_pcb;

typedef union
{
    thread_pcb pcb;
    uint8_t kstack[THREAD_KSTACK_SIZE];
} thread_kinfo;

//!!!! thread_kinfo must be aligned in memory !!!!


#ifdef __cplusplus
extern "C" {
#endif
    thread_kinfo* getCurrentThreadInfo();
#ifdef __cplusplus
}
#endif

#endif

#endif /* _TASKS_H */