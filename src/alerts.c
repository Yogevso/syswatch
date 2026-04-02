/*
 * alerts.c — Anomaly detection engine
 *
 * Evaluates process and network snapshots against configurable
 * thresholds and produces a list of triggered alerts.
 */

#include "alerts.h"

#include <stdio.h>
#include <string.h>

/* Suspicious ports: commonly associated with backdoors or debug services. */
const uint16_t SUSPICIOUS_PORTS[SUSPICIOUS_PORT_COUNT] = {
    4444,  /* Metasploit default */
    5555,  /* Android debug bridge */
    6666,  /* IRC backdoor */
    1337,  /* "elite" port */
    31337, /* Back Orifice */
    12345  /* NetBus */
};

/* ---------- internal helpers ---------- */

static void push_alert(alert_list_t *list, alert_type_t type, const char *msg)
{
    if (list->count >= MAX_ALERTS)
        return;

    alert_t *a = &list->alerts[list->count];
    a->type = type;
    snprintf(a->message, ALERT_MSG_LEN, "%s", msg);
    list->count++;
}

static int is_suspicious_port(uint16_t port)
{
    for (int i = 0; i < SUSPICIOUS_PORT_COUNT; i++) {
        if (SUSPICIOUS_PORTS[i] == port)
            return 1;
    }
    return 0;
}

/* ---------- public API ---------- */

void alert_config_init(alert_config_t *cfg)
{
    cfg->cpu_threshold    = DEFAULT_CPU_THRESHOLD;
    cfg->conn_threshold   = DEFAULT_CONN_THRESHOLD;
    cfg->mem_threshold_kb = DEFAULT_MEM_THRESHOLD_KB;
}

void alert_evaluate(const alert_config_t *cfg,
                    const proc_snapshot_t *procs,
                    const net_snapshot_t  *net,
                    alert_list_t *out)
{
    out->count = 0;

    /* --- Process alerts --- */
    if (procs) {
        for (int i = 0; i < procs->count; i++) {
            const proc_info_t *p = &procs->procs[i];

            if (p->cpu_usage >= cfg->cpu_threshold) {
                char buf[ALERT_MSG_LEN];
                snprintf(buf, sizeof(buf),
                         "High CPU usage detected (%s [%d]: %.1f%%)",
                         p->name, (int)p->pid, p->cpu_usage);
                push_alert(out, ALERT_HIGH_CPU, buf);
            }

            if (p->mem_kb >= cfg->mem_threshold_kb) {
                char buf[ALERT_MSG_LEN];
                snprintf(buf, sizeof(buf),
                         "High memory usage detected (%s [%d]: %lu MB)",
                         p->name, (int)p->pid, p->mem_kb / 1024);
                push_alert(out, ALERT_HIGH_MEM, buf);
            }
        }
    }

    /* --- Network alerts --- */
    if (net) {
        if (net->count >= cfg->conn_threshold) {
            char buf[ALERT_MSG_LEN];
            snprintf(buf, sizeof(buf),
                     "Too many active connections (%d)", net->count);
            push_alert(out, ALERT_CONN_SPIKE, buf);
        }

        for (int i = 0; i < net->count; i++) {
            const net_conn_t *c = &net->conns[i];

            if (is_suspicious_port(c->local_port)) {
                char buf[ALERT_MSG_LEN];
                snprintf(buf, sizeof(buf),
                         "Suspicious local port %u open (%s)",
                         c->local_port,
                         c->proto == PROTO_TCP ? "TCP" : "UDP");
                push_alert(out, ALERT_SUSPICIOUS_PORT, buf);
            }

            if (is_suspicious_port(c->remote_port)) {
                char buf[ALERT_MSG_LEN];
                snprintf(buf, sizeof(buf),
                         "Connection to suspicious remote port %u (%s:%u -> %s:%u)",
                         c->remote_port,
                         c->local_addr, c->local_port,
                         c->remote_addr, c->remote_port);
                push_alert(out, ALERT_SUSPICIOUS_PORT, buf);
            }
        }
    }
}
