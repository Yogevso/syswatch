#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

#define MAX_PROCESSES 1024
#define PROC_NAME_LEN 256

typedef struct {
    pid_t pid;
    char name[PROC_NAME_LEN];
    double cpu_usage;    /* percentage */
    unsigned long mem_kb; /* memory in KB */
    unsigned long utime;  /* user time (raw ticks, for delta calc) */
    unsigned long stime;  /* system time (raw ticks, for delta calc) */
    int conn_count;       /* number of network connections (filled by correlation) */
} proc_info_t;

typedef struct {
    proc_info_t procs[MAX_PROCESSES];
    int count;
    unsigned long long total_cpu_prev; /* previous total CPU ticks */
} proc_snapshot_t;

/* Take a snapshot of all running processes.
 * If |prev| is non-NULL, CPU percentages are computed as deltas. */
int proc_snapshot(proc_snapshot_t *snap, const proc_snapshot_t *prev);

/* Sort snapshot by CPU usage descending. */
void proc_sort_by_cpu(proc_snapshot_t *snap);

/* Sort snapshot by memory usage descending. */
void proc_sort_by_mem(proc_snapshot_t *snap);

#endif /* PROCESS_H */
