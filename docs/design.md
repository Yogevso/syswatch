# SysWatch — Design Document

## Architecture Overview

```
+----------------------+
| CLI Interface        |  main.c — argument parsing, mode selection, main loop
+----------+-----------+
           |
           v
+----------------------+
| Process Monitor      |  process.c — /proc/<pid>/stat + /proc/<pid>/status
+----------+-----------+
           |
           v
+----------------------+
| Network Monitor      |  network.c — /proc/net/tcp + /proc/net/udp
+----------+-----------+
           |
           v
+----------------------+
| Alert Engine         |  alerts.c — threshold rules, suspicious port detection
+----------+-----------+
           |
           v
+----------------------+
| Output Formatter     |  main.c — ANSI colors, tabular display
+----------------------+
```

## Module Design

### process.c / process.h

**Data structures:**
- `proc_info_t` — per-process info: PID, name, CPU%, memory (KB), raw tick counts
- `proc_snapshot_t` — array of `proc_info_t` + total CPU ticks for delta computation

**Key functions:**
- `proc_snapshot()` — scans `/proc`, reads each numeric directory's `stat` and `status` files. If a previous snapshot is provided, computes CPU usage as a percentage of elapsed total CPU ticks.
- `proc_sort_by_cpu()` / `proc_sort_by_mem()` — sort helpers using `qsort`.

**CPU % calculation:**
```
              (utime_curr + stime_curr) - (utime_prev + stime_prev)
CPU% = 100 × ──────────────────────────────────────────────────────
                         total_cpu_curr - total_cpu_prev
```

### network.c / network.h

**Data structures:**
- `net_conn_t` — single connection: protocol, local/remote addr+port, TCP state
- `net_snapshot_t` — array of `net_conn_t`

**Key functions:**
- `net_snapshot()` — parses `/proc/net/tcp` and `/proc/net/udp`
- `tcp_state_str()` — maps kernel state codes to human-readable strings

**Parsing approach:**
- Each line in `/proc/net/tcp` has hex-encoded IP:port pairs and a state code
- IPv4 addresses are decoded from host-byte-order hex to dotted decimal
- Ports are decoded from hex to uint16

### alerts.c / alerts.h

**Configuration:**
- `alert_config_t` — thresholds for CPU%, connection count, memory
- Defaults: CPU 80%, connections 100, memory 1 GB

**Detection rules:**
1. **High CPU** — any process exceeding `cpu_threshold`
2. **High Memory** — any process exceeding `mem_threshold_kb`
3. **Connection Spike** — total connections ≥ `conn_threshold`
4. **Suspicious Port** — local or remote port matches known backdoor list (4444, 5555, 6666, 1337, 31337, 12345)

### main.c (CLI + Display)

**Modes:** `MODE_TOP`, `MODE_NET`, `MODE_ALERTS`, `MODE_ALL`

**Main loop:**
1. Gather data (process/network snapshots based on mode)
2. Run alert evaluation if needed
3. Clear screen, print formatted output
4. Sleep for interval, repeat until SIGINT/SIGTERM

**Color scheme:**
- CPU < 40%: green | 40-80%: yellow | ≥80%: red
- ESTABLISHED: cyan | TIME_WAIT/CLOSE_WAIT: yellow
- Alerts: red label, yellow message

## Build System

Simple `Makefile` with:
- `gcc -Wall -Wextra -Werror -O2`
- Object files in `obj/`
- Single binary output: `syswatch`

## Limitations & Future Work
- IPv4 only (no IPv6 `/proc/net/tcp6` parsing yet)
- No config file support yet (thresholds are CLI flags only)
- No log-to-file mode yet
- Process count capped at 1024, connections at 2048
