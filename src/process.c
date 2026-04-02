/*
 * process.c — Process monitoring via /proc
 *
 * Reads /proc/<pid>/stat and /proc/<pid>/status to gather per-process
 * CPU and memory information.  CPU percentages are computed as deltas
 * between two consecutive snapshots.
 */

#include "process.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ---------- internal helpers ---------- */

/* Read total CPU ticks from /proc/stat (first "cpu" line). */
static unsigned long long read_total_cpu(void)
{
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp)
        return 0;

    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    user = nice = system = idle = iowait = irq = softirq = steal = 0;

    if (fscanf(fp, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq,
               &steal) < 4) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

/* Read utime + stime from /proc/<pid>/stat.
 * Also extracts the process name (comm field). */
static int read_proc_stat(pid_t pid, char *name, size_t name_len,
                          unsigned long *utime, unsigned long *stime)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", (int)pid);

    FILE *fp = fopen(path, "r");
    if (!fp)
        return -1;

    char line[1024];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    /* The comm field is wrapped in parentheses and may contain spaces.
     * Find the last ')' to safely skip it. */
    char *start = strchr(line, '(');
    char *end   = strrchr(line, ')');
    if (!start || !end || end <= start)
        return -1;

    size_t comm_len = (size_t)(end - start - 1);
    if (comm_len >= name_len)
        comm_len = name_len - 1;
    memcpy(name, start + 1, comm_len);
    name[comm_len] = '\0';

    /* Fields after ')': state(3) ppid(4) ... utime(14) stime(15) */
    char *p = end + 2; /* skip ") " */
    int field = 3;
    while (*p && field < 14) {
        if (*p == ' ')
            field++;
        p++;
    }

    if (sscanf(p, "%lu %lu", utime, stime) != 2)
        return -1;

    return 0;
}

/* Read VmRSS from /proc/<pid>/status (resident memory in kB). */
static unsigned long read_proc_mem(pid_t pid)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", (int)pid);

    FILE *fp = fopen(path, "r");
    if (!fp)
        return 0;

    char line[256];
    unsigned long rss = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, " %lu", &rss);
            break;
        }
    }
    fclose(fp);
    return rss;
}

/* ---------- public API ---------- */

int proc_snapshot(proc_snapshot_t *snap, const proc_snapshot_t *prev)
{
    DIR *dir = opendir("/proc");
    if (!dir)
        return -1;

    snap->count = 0;
    snap->total_cpu_prev = read_total_cpu();

    unsigned long long total_delta = 0;
    if (prev)
        total_delta = snap->total_cpu_prev - prev->total_cpu_prev;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && snap->count < MAX_PROCESSES) {
        /* Only numeric directory names correspond to PIDs. */
        if (!isdigit((unsigned char)entry->d_name[0]))
            continue;

        pid_t pid = (pid_t)atoi(entry->d_name);
        if (pid <= 0)
            continue;

        proc_info_t *p = &snap->procs[snap->count];
        p->pid = pid;

        unsigned long utime = 0, stime = 0;
        if (read_proc_stat(pid, p->name, sizeof(p->name), &utime, &stime) < 0)
            continue;

        p->utime = utime;
        p->stime = stime;
        p->mem_kb = read_proc_mem(pid);

        /* Compute CPU % as delta if we have a previous snapshot. */
        p->cpu_usage = 0.0;
        if (prev && total_delta > 0) {
            /* Find this PID in prev snapshot. */
            for (int i = 0; i < prev->count; i++) {
                if (prev->procs[i].pid == pid) {
                    unsigned long proc_delta =
                        (utime + stime) -
                        (prev->procs[i].utime + prev->procs[i].stime);
                    p->cpu_usage =
                        100.0 * (double)proc_delta / (double)total_delta;
                    break;
                }
            }
        }

        snap->count++;
    }
    closedir(dir);
    return 0;
}

static int cmp_cpu_desc(const void *a, const void *b)
{
    double da = ((const proc_info_t *)a)->cpu_usage;
    double db = ((const proc_info_t *)b)->cpu_usage;
    if (db > da) return 1;
    if (db < da) return -1;
    return 0;
}

void proc_sort_by_cpu(proc_snapshot_t *snap)
{
    qsort(snap->procs, (size_t)snap->count, sizeof(proc_info_t), cmp_cpu_desc);
}

static int cmp_mem_desc(const void *a, const void *b)
{
    unsigned long ma = ((const proc_info_t *)a)->mem_kb;
    unsigned long mb = ((const proc_info_t *)b)->mem_kb;
    if (mb > ma) return 1;
    if (mb < ma) return -1;
    return 0;
}

void proc_sort_by_mem(proc_snapshot_t *snap)
{
    qsort(snap->procs, (size_t)snap->count, sizeof(proc_info_t), cmp_mem_desc);
}
