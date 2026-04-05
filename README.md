# SysWatch

> **Part of the [Orchestrix Platform](https://github.com/Yogevso/Orchestrix-Platform)** — the system-level monitoring layer feeding metrics into System Insights API.

![C](https://img.shields.io/badge/language-C-blue)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)
![React](https://img.shields.io/badge/dashboard-React-61DAFB)
![Docker](https://img.shields.io/badge/docker-ready-2496ED)
![Status](https://img.shields.io/badge/status-Complete-success)

> A lightweight system observability tool designed for debugging real-world issues such as CPU spikes, connection leaks, and suspicious activity — using only Linux internals.

**Real-time system observability tool (CLI + dashboard)** for monitoring processes, network activity, and detecting anomalies on Linux.

> Designed to debug real issues like connection leaks (`CLOSE_WAIT`), CPU spikes, and suspicious network activity.

SysWatch aggregates system metrics from `/proc`, tracks network connections, correlates processes to their sockets, and detects anomalies — all from a single, lightweight command-line tool. The companion React dashboard provides a real-time visual interface with live WebSocket updates.

## Why This Project

Modern systems are hard to debug because information is scattered across tools like `top`, `netstat`, and logs.

SysWatch brings process monitoring, network inspection, and anomaly detection into a single lightweight tool — designed for real debugging workflows, not just observation.

## Key Highlights

- Built in **C using Linux `/proc` internals**
- Correlates **processes to network connections** via socket inode matching
- Real-time anomaly detection (CPU, memory, connection spikes, suspicious ports)
- CLI tool + **live React dashboard via WebSocket**
- Fully containerized with **Docker multi-stage builds**
- Designed for **debugging real production issues**

## Features

- **Process Monitoring** — CPU and memory usage per process via `/proc` parsing
- **Network Monitoring** — Active TCP/UDP connections from `/proc/net`
- **Process ↔ Network Correlation** — Connection count per process via socket inode matching through `/proc/<pid>/fd`
- **Anomaly Detection** — Real-time alerts for high CPU, memory spikes, connection floods, and suspicious ports (4444, 5555, 6666, 1337, 31337, 12345)
- **JSON Output** — Structured JSON mode (`--json`) for dashboard and tool integration
- **React Dashboard** — Real-time web UI with glassmorphism design, SVG icons, animated stats, sortable tables, and live WebSocket updates
- **Real-Time Updates** — Configurable refresh interval with ANSI-colored output
- **CLI-First** — No GUI overhead. Fast, scriptable, developer-friendly

## Build

```bash
make
```

Requires `gcc` and a Linux system with `/proc` filesystem.

## Usage

```bash
# Process view (default)
./syswatch --top

# Network connections
./syswatch --net

# Anomaly alerts
./syswatch --alerts

# Combined dashboard
./syswatch --all

# Custom refresh interval (1 second)
./syswatch --top -n 1

# Run once and exit (no loop)
./syswatch --all --once

# Custom thresholds
./syswatch --alerts --cpu-threshold 50 --conn-threshold 80

# Disable colors (for piping / logging)
./syswatch --top --no-color

# JSON output (for dashboards / external tools)
./syswatch --all --json --once
```

## Docker — Run & Test

```bash
# Build and run everything (CLI + React dashboard)
docker compose up --build -d

# Access the dashboard at http://localhost:5173
# WebSocket backend at ws://localhost:3001
```

To test the CLI only:
```bash
docker compose up backend --build -d
# Then in another terminal:
docker exec syswatch-backend syswatch --all --once
docker exec syswatch-backend syswatch --json --once
```

## Dashboard

The React dashboard connects to the backend via WebSocket and displays live system data:

- **Header** — SVG logo with gradient branding, live/disconnected status badge with animated pulse
- **Stats Bar** — 5 metric cards (Processes, Peak CPU, Total RSS, Connections, Alerts) with color-coded SVG icons, gradient accent lines, and hover lift effects
- **Process Table** — Sortable by CPU/MEM/CONNS with gradient CPU progress bars and glow effects
- **Network Table** — Filterable by protocol (TCP/UDP) and state (ESTABLISHED), with alternating row stripes and color-coded states
- **Alert Panel** — SVG alert icons with staggered slide-in animations, pulsing badge counter, and color-coded severity levels

The UI uses a deep dark theme with glassmorphism cards, Inter + JetBrains Mono font pairing, custom scrollbars, and smooth micro-animations throughout.

## Example Output

```
═══ SysWatch — Process Monitor ═══

PID     NAME                    CPU%        MEM  CONNS
──────────────────────────────────────────────────────────
[PROC] 1423    chrome              42.1%     320 MB     12
[PROC] 2891    python              15.3%     120 MB      3
[PROC] 512     nginx                3.2%      48 MB     87

Total processes: 187
```

```
═══ SysWatch — Alerts ═══

[ALERT] High CPU usage detected (chrome [1423]: 92.1%)
[ALERT] Too many active connections (120)
```

JSON output (`--json`):
```json
{
  "processes": [
    {"pid": 1423, "name": "chrome", "cpu": 42.1, "mem_kb": 327680, "connections": 12}
  ],
  "connections": [
    {"proto": "TCP", "local_addr": "0.0.0.0", "local_port": 8080, "remote_addr": "192.168.1.50", "remote_port": 52344, "state": "ESTABLISHED"}
  ],
  "total_processes": 187,
  "total_connections": 42,
  "alerts": [
    {"type": "high_cpu", "message": "High CPU usage detected (chrome [1423]: 92.1%)"}
  ]
}
```

## Tech Stack

| Layer | Technology |
|-------|-----------|
| Core CLI | C (gcc), static linking, `/proc` filesystem |
| Dashboard | React 18, Vite 5, CSS custom properties |
| WebSocket Bridge | Node.js, `ws` library |
| Containerization | Docker multi-stage builds, docker-compose |
| Serving | nginx:alpine (dashboard), node:20-slim (backend) |

## Project Structure

```
syswatch/
├── src/
│   ├── main.c          # CLI interface, JSON output, main loop
│   ├── process.c       # /proc parsing for processes
│   ├── network.c       # /proc/net parsing + PID correlation
│   └── alerts.c        # Anomaly detection engine
├── include/
│   ├── process.h
│   ├── network.h
│   └── alerts.h
├── dashboard/
│   ├── src/            # React frontend (Vite)
│   ├── server/         # Node.js WebSocket bridge
│   ├── Dockerfile
│   └── package.json
├── docs/
│   ├── PRD.md
│   ├── design.md
│   └── debugging_case.md
├── Dockerfile
├── docker-compose.yml
├── Makefile
├── README.md
└── LICENSE
```

## Documentation

- [Product Requirements](docs/PRD.md)
- [Design Document](docs/design.md)
- [Debugging Case Study](docs/debugging_case.md)

## License

See [LICENSE](LICENSE) for details.
