# SysWatch — Debugging Case Study

## Scenario: Diagnosing a Runaway Process

### Problem
A backend service becomes unresponsive. The engineering team suspects a resource leak but doesn't know which process is responsible.

### Using SysWatch

#### Step 1 — Identify the CPU hog
```bash
$ ./syswatch --top -n 1
```
Output:
```
═══ SysWatch — Process Monitor ═══

PID     NAME                    CPU%        MEM
─────────────────────────────────────────────────
[PROC] 8421    my-backend-worker     87.3%     412 MB
[PROC] 1023    postgres              12.1%     256 MB
[PROC] 512     nginx                  3.2%      48 MB
```
The worker process is consuming 87% CPU — well above normal.

#### Step 2 — Check for connection leaks
```bash
$ ./syswatch --net
```
Output:
```
═══ SysWatch — Network Monitor ═══

PROTO LOCAL                  REMOTE                 STATE
─────────────────────────────────────────────────────────────────
[NET]  TCP   0.0.0.0:8080       192.168.1.50:52344     ESTABLISHED
[NET]  TCP   0.0.0.0:8080       192.168.1.50:52345     ESTABLISHED
[NET]  TCP   0.0.0.0:8080       192.168.1.50:52346     CLOSE_WAIT
...
(142 connections shown)
```
142 connections, many in `CLOSE_WAIT` — the worker is not closing connections properly.

#### Step 3 — Confirm with alerts
```bash
$ ./syswatch --alerts --cpu-threshold 50 --conn-threshold 80
```
Output:
```
═══ SysWatch — Alerts ═══

[ALERT] High CPU usage detected (my-backend-worker [8421]: 87.3%)
[ALERT] Too many active connections (142)
```

### Diagnosis
The backend worker has a connection leak: it opens TCP connections to downstream services but doesn't close them on timeout, causing `CLOSE_WAIT` accumulation and CPU spin in the event loop.

### Resolution
- Fix the connection cleanup logic in the worker
- Add timeout handling for stale connections
- Use `syswatch --all -n 1` to verify the fix in real time

---

## Scenario: Detecting a Suspicious Service

### Problem
A server is exhibiting unusual outbound traffic.

### Using SysWatch
```bash
$ ./syswatch --alerts
```
Output:
```
═══ SysWatch — Alerts ═══

[ALERT] Suspicious local port 4444 open (TCP)
[ALERT] Connection to suspicious remote port 31337 (192.168.1.10:45678 -> 10.0.0.99:31337)
```

Port 4444 (common Metasploit listener) and outbound traffic to port 31337 (Back Orifice) are flagged. This warrants immediate investigation — potential compromise.

### Next Steps
- Identify the process bound to port 4444 (`--top` to correlate PID)
- Isolate the machine from the network
- Perform forensic analysis
