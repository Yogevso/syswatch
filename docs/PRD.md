# SysWatch — Product Requirements Document

> **SysWatch** is a lightweight system observability tool designed for debugging real-world issues such as CPU spikes, connection leaks, and suspicious activity — using only Linux internals.

## Project Name
**SysWatch** — System Observability CLI

## Objective
Build a real-time system observability tool that enables:
- Monitoring of system processes and resource usage
- Inspection of network activity and connections
- Detection of abnormal system behavior
- Debugging of performance and runtime issues

## Target Users
- System engineers
- DevOps / SRE engineers
- Backend developers
- QA / validation engineers

## Problem Statement
Debugging system behavior is difficult because:
- Information is fragmented across many tools (`top`, `netstat`, `ss`, logs, etc.)
- Anomalies are not obvious in real time
- Existing tools are either too basic or too complex

Engineers need a **lightweight, unified CLI tool** to monitor and debug system behavior quickly.

## Solution
A CLI-based observability tool that:
- Aggregates system metrics (CPU, memory, processes)
- Tracks network activity
- Detects anomalies in real time
- Provides simple, readable output

## Core Features

### 1. Process Monitoring (`--top`)
- Lists running processes from `/proc`
- Shows CPU usage per process (computed as delta between snapshots)
- Shows memory usage (VmRSS) per process
- Sorted by CPU usage descending

### 2. Network Monitoring (`--net`)
- Parses `/proc/net/tcp` and `/proc/net/udp`
- Displays local → remote address:port pairs
- Shows TCP connection state

### 3. Alert / Anomaly Detection (`--alerts`)
- High CPU usage detection (configurable threshold, default 80%)
- High memory usage detection (default 1 GB)
- Connection spike detection (configurable threshold, default 100)
- Suspicious port detection (known backdoor ports)

### 4. CLI Modes
```
./syswatch --top       # process view (default)
./syswatch --net       # network view
./syswatch --alerts    # anomaly-focused view
./syswatch --all       # combined dashboard
```

### 5. Real-Time Updates
- Refresh every N seconds (configurable via `-n`)
- Live monitoring with SIGINT/SIGTERM graceful shutdown
- `--once` flag for single-shot execution

### 6. Options
- `--no-color` — disable ANSI colored output
- `--json` — output structured JSON for dashboard/tool integration
- `--cpu-threshold <pct>` — set CPU alert threshold
- `--conn-threshold <num>` — set connection count alert threshold

### 7. JSON Output Mode (`--json`)
- Outputs structured JSON per update cycle
- Enables integration with dashboards, logging pipelines, or external tools
- Includes processes, connections, and alerts in a single JSON object

Example:
```json
{
  "processes": [
    {"pid": 8421, "name": "worker", "cpu": 87.3, "mem_kb": 421888, "connections": 142}
  ],
  "connections": [
    {"proto": "TCP", "local_addr": "0.0.0.0", "local_port": 8080, "remote_addr": "192.168.1.50", "remote_port": 52344, "state": "ESTABLISHED"}
  ],
  "total_processes": 187,
  "total_connections": 142,
  "alerts": [
    {"type": "high_cpu", "message": "High CPU usage detected (worker [8421]: 87.3%)"}
  ]
}
```

### 8. Process ↔ Network Correlation
- Each process shows its active connection count
- Correlation performed via `/proc/<pid>/fd` socket inode matching against `/proc/net/tcp` entries
- Enables quick identification of processes with connection leaks

## Success Criteria
- Process data displayed correctly from `/proc`
- Network connections parsed and shown correctly
- Alerts trigger under defined conditions
- CLI runs smoothly with real-time refresh
- JSON output is valid and parseable by external tools
- Process-network correlation correctly counts connections per PID

## Why It Matters

This project demonstrates low-level Linux system understanding, real-time monitoring, and debugging workflows used in production environments. It reflects how engineers diagnose performance issues, resource leaks, and abnormal system behavior.

SysWatch is not a toy monitoring tool — it is a debugging instrument built on the same `/proc` filesystem internals that power tools like `top`, `ss`, and `lsof`. The process↔network correlation feature goes beyond what most standard tools offer out of the box, enabling engineers to instantly identify which process owns a connection leak.
