/*
 * main.c — SysWatch CLI entry point
 *
 * Provides the user-facing command-line interface with modes:
 *   --top       Process view (default)
 *   --net       Network connections view
 *   --alerts    Anomaly / alert view
 *   --all       Combined dashboard
 *
 * Options:
 *   -n <sec>    Refresh interval (default: 2)
 *   --no-color  Disable colored output
 *   --json      Output structured JSON (for dashboard integration)
 *   --cpu-threshold <pct>   CPU alert threshold (default: 80)
 *   --conn-threshold <num>  Connection alert threshold (default: 100)
 *   --once      Run once and exit (no real-time loop)
 *   -h, --help  Show usage
 */

#include "process.h"
#include "network.h"
#include "alerts.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/*  ANSI color helpers                                                 */
/* ------------------------------------------------------------------ */

static int use_color = 1;
static int use_json  = 0;

#define CLR_RESET   "\033[0m"
#define CLR_BOLD    "\033[1m"
#define CLR_RED     "\033[31m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_CYAN    "\033[36m"
#define CLR_MAGENTA "\033[35m"

static const char *c_reset(void)   { return use_color ? CLR_RESET   : ""; }
static const char *c_bold(void)    { return use_color ? CLR_BOLD    : ""; }
static const char *c_red(void)     { return use_color ? CLR_RED     : ""; }
static const char *c_green(void)   { return use_color ? CLR_GREEN   : ""; }
static const char *c_yellow(void)  { return use_color ? CLR_YELLOW  : ""; }
static const char *c_cyan(void)    { return use_color ? CLR_CYAN    : ""; }
static const char *c_magenta(void) { return use_color ? CLR_MAGENTA : ""; }

/* ------------------------------------------------------------------ */
/*  Display helpers                                                    */
/* ------------------------------------------------------------------ */

static void clear_screen(void)
{
    printf("\033[2J\033[H");
    fflush(stdout);
}

static void print_header(const char *title)
{
    printf("%s%s═══ SysWatch — %s ═══%s\n\n",
           c_bold(), c_cyan(), title, c_reset());
}

static void display_processes(const proc_snapshot_t *snap, int limit)
{
    print_header("Process Monitor");

    printf("%s%-7s %-20s %8s %10s %6s%s\n",
           c_bold(), "PID", "NAME", "CPU%", "MEM", "CONNS", c_reset());
    printf("──────────────────────────────────────────────────────────\n");

    int n = snap->count < limit ? snap->count : limit;
    for (int i = 0; i < n; i++) {
        const proc_info_t *p = &snap->procs[i];

        const char *cpu_clr = c_green();
        if (p->cpu_usage >= 80.0)
            cpu_clr = c_red();
        else if (p->cpu_usage >= 40.0)
            cpu_clr = c_yellow();

        /* Format memory as MB or KB */
        char mem_str[32];
        if (p->mem_kb >= 1024)
            snprintf(mem_str, sizeof(mem_str), "%lu MB", p->mem_kb / 1024);
        else
            snprintf(mem_str, sizeof(mem_str), "%lu KB", p->mem_kb);

        printf("%s[PROC]%s %-7d %-20.20s %s%7.1f%%%s %10s %6d\n",
               c_magenta(), c_reset(),
               (int)p->pid, p->name,
               cpu_clr, p->cpu_usage, c_reset(),
               mem_str, p->conn_count);
    }

    printf("\n%sTotal processes: %d%s\n", c_cyan(), snap->count, c_reset());
}

static void display_network(const net_snapshot_t *snap)
{
    print_header("Network Monitor");

    printf("%s%-5s %-22s %-22s %-13s%s\n",
           c_bold(), "PROTO", "LOCAL", "REMOTE", "STATE", c_reset());
    printf("─────────────────────────────────────────────────────────────────\n");

    for (int i = 0; i < snap->count; i++) {
        const net_conn_t *c = &snap->conns[i];

        char local[80], remote[80];
        snprintf(local, sizeof(local), "%s:%u", c->local_addr, c->local_port);
        snprintf(remote, sizeof(remote), "%s:%u",
                 c->remote_addr, c->remote_port);

        const char *proto_str = c->proto == PROTO_TCP ? "TCP" : "UDP";
        const char *state_str = c->proto == PROTO_TCP
                                    ? tcp_state_str(c->state)
                                    : "-";

        const char *state_clr = c_green();
        if (c->state == TCP_ST_ESTABLISHED)
            state_clr = c_cyan();
        else if (c->state == TCP_ST_TIME_WAIT || c->state == TCP_ST_CLOSE_WAIT)
            state_clr = c_yellow();

        printf("%s[NET]%s  %-5s %-22s → %-22s %s%-13s%s\n",
               c_green(), c_reset(),
               proto_str, local, remote,
               state_clr, state_str, c_reset());
    }

    printf("\n%sTotal connections: %d%s\n", c_cyan(), snap->count, c_reset());
}

static void display_alerts(const alert_list_t *alerts)
{
    print_header("Alerts");

    if (alerts->count == 0) {
        printf("%s  ✓ No anomalies detected.%s\n\n", c_green(), c_reset());
        return;
    }

    for (int i = 0; i < alerts->count; i++) {
        const alert_t *a = &alerts->alerts[i];
        printf("%s[ALERT]%s %s%s%s\n",
               c_red(), c_reset(),
               c_yellow(), a->message, c_reset());
    }

    printf("\n%sTotal alerts: %d%s\n", c_red(), alerts->count, c_reset());
}

/* ------------------------------------------------------------------ */
/*  JSON output                                                        */
/* ------------------------------------------------------------------ */

/* Escape a string for JSON output. */
static void json_escape(const char *src, char *dst, size_t dst_len)
{
    size_t j = 0;
    for (size_t i = 0; src[i] && j < dst_len - 2; i++) {
        switch (src[i]) {
        case '"':  if (j + 2 < dst_len) { dst[j++] = '\\'; dst[j++] = '"'; }  break;
        case '\\': if (j + 2 < dst_len) { dst[j++] = '\\'; dst[j++] = '\\'; } break;
        case '\n': if (j + 2 < dst_len) { dst[j++] = '\\'; dst[j++] = 'n'; }  break;
        case '\t': if (j + 2 < dst_len) { dst[j++] = '\\'; dst[j++] = 't'; }  break;
        default:   dst[j++] = src[i]; break;
        }
    }
    dst[j] = '\0';
}

static void output_json(const proc_snapshot_t *procs, int proc_limit,
                        const net_snapshot_t *net,
                        const alert_list_t *alerts)
{
    printf("{\n");

    /* Processes */
    printf("  \"processes\": [");
    if (procs) {
        int n = procs->count < proc_limit ? procs->count : proc_limit;
        for (int i = 0; i < n; i++) {
            const proc_info_t *p = &procs->procs[i];
            char esc_name[512];
            json_escape(p->name, esc_name, sizeof(esc_name));
            printf("%s\n    {\"pid\":%d,\"name\":\"%s\",\"cpu\":%.1f,\"mem_kb\":%lu,\"connections\":%d}",
                   i > 0 ? "," : "",
                   (int)p->pid, esc_name, p->cpu_usage, p->mem_kb, p->conn_count);
        }
    }
    printf("\n  ],\n");

    /* Network */
    printf("  \"connections\": [");
    if (net) {
        for (int i = 0; i < net->count; i++) {
            const net_conn_t *c = &net->conns[i];
            printf("%s\n    {\"proto\":\"%s\",\"local_addr\":\"%s\",\"local_port\":%u,"
                   "\"remote_addr\":\"%s\",\"remote_port\":%u,\"state\":\"%s\"}",
                   i > 0 ? "," : "",
                   c->proto == PROTO_TCP ? "TCP" : "UDP",
                   c->local_addr, c->local_port,
                   c->remote_addr, c->remote_port,
                   c->proto == PROTO_TCP ? tcp_state_str(c->state) : "-");
        }
    }
    printf("\n  ],\n");

    /* Totals */
    printf("  \"total_processes\": %d,\n", procs ? procs->count : 0);
    printf("  \"total_connections\": %d,\n", net ? net->count : 0);

    /* Alerts */
    printf("  \"alerts\": [");
    if (alerts) {
        for (int i = 0; i < alerts->count; i++) {
            char esc_msg[512];
            json_escape(alerts->alerts[i].message, esc_msg, sizeof(esc_msg));
            const char *type_str;
            switch (alerts->alerts[i].type) {
            case ALERT_HIGH_CPU:        type_str = "high_cpu"; break;
            case ALERT_HIGH_MEM:        type_str = "high_mem"; break;
            case ALERT_CONN_SPIKE:      type_str = "conn_spike"; break;
            case ALERT_SUSPICIOUS_PORT: type_str = "suspicious_port"; break;
            default:                    type_str = "unknown"; break;
            }
            printf("%s\n    {\"type\":\"%s\",\"message\":\"%s\"}",
                   i > 0 ? "," : "", type_str, esc_msg);
        }
    }
    printf("\n  ]\n");

    printf("}\n");
    fflush(stdout);
}

/* ------------------------------------------------------------------ */
/*  Signal handling for graceful exit                                   */
/* ------------------------------------------------------------------ */

static volatile sig_atomic_t running = 1;

static void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

/* ------------------------------------------------------------------ */
/*  Usage                                                              */
/* ------------------------------------------------------------------ */

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [MODE] [OPTIONS]\n"
        "\n"
        "Modes:\n"
        "  --top          Process view (default)\n"
        "  --net          Network connections view\n"
        "  --alerts       Anomaly-focused view\n"
        "  --all          Combined dashboard\n"
        "\n"
        "Options:\n"
        "  -n <seconds>           Refresh interval (default: 2)\n"
        "  --once                 Run once and exit\n"
        "  --json                 Output structured JSON\n"
        "  --no-color             Disable colored output\n"
        "  --cpu-threshold <pct>  CPU alert threshold (default: 80)\n"
        "  --conn-threshold <num> Connection alert threshold (default: 100)\n"
        "  -h, --help             Show this help\n",
        prog);
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

typedef enum {
    MODE_TOP,
    MODE_NET,
    MODE_ALERTS,
    MODE_ALL
} run_mode_t;

int main(int argc, char *argv[])
{
    run_mode_t mode = MODE_TOP;
    int interval    = 2;
    int once        = 0;

    alert_config_t alert_cfg;
    alert_config_init(&alert_cfg);

    /* ---- Parse arguments ---- */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--top") == 0) {
            mode = MODE_TOP;
        } else if (strcmp(argv[i], "--net") == 0) {
            mode = MODE_NET;
        } else if (strcmp(argv[i], "--alerts") == 0) {
            mode = MODE_ALERTS;
        } else if (strcmp(argv[i], "--all") == 0) {
            mode = MODE_ALL;
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            interval = atoi(argv[++i]);
            if (interval < 1) interval = 1;
        } else if (strcmp(argv[i], "--once") == 0) {
            once = 1;
        } else if (strcmp(argv[i], "--no-color") == 0) {
            use_color = 0;
        } else if (strcmp(argv[i], "--json") == 0) {
            use_json = 1;
            use_color = 0;
        } else if (strcmp(argv[i], "--cpu-threshold") == 0 && i + 1 < argc) {
            alert_cfg.cpu_threshold = atof(argv[++i]);
        } else if (strcmp(argv[i], "--conn-threshold") == 0 && i + 1 < argc) {
            alert_cfg.conn_threshold = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 ||
                   strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    /* ---- Install signal handlers ---- */
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    /* ---- Main loop ---- */
    proc_snapshot_t snap_prev, snap_curr;
    int has_prev = 0;

    while (running) {
        proc_snapshot_t *procs = NULL;
        net_snapshot_t net;
        alert_list_t alerts;
        int have_net = 0;

        /* Gather data based on mode (JSON always gathers everything). */
        if (use_json || mode == MODE_TOP || mode == MODE_ALL || mode == MODE_ALERTS) {
            if (proc_snapshot(&snap_curr, has_prev ? &snap_prev : NULL) == 0) {
                proc_sort_by_cpu(&snap_curr);
                procs = &snap_curr;
            }
        }

        if (use_json || mode == MODE_NET || mode == MODE_ALL || mode == MODE_ALERTS) {
            net_snapshot(&net);
            have_net = 1;
        }

        /* Process ↔ Network correlation: count connections per PID. */
        if (procs && have_net) {
            for (int i = 0; i < procs->count; i++)
                procs->procs[i].conn_count = net_count_by_pid(&net, procs->procs[i].pid);
        }

        if (use_json || mode == MODE_ALERTS || mode == MODE_ALL) {
            alert_evaluate(&alert_cfg, procs, &net, &alerts);
        }

        /* JSON output mode. */
        if (use_json) {
            output_json(procs, 50,
                        have_net ? &net : NULL,
                        &alerts);
        } else {
        /* Display. */
        clear_screen();

        switch (mode) {
        case MODE_TOP:
            if (procs)
                display_processes(procs, 25);
            break;
        case MODE_NET:
            display_network(&net);
            break;
        case MODE_ALERTS:
            display_alerts(&alerts);
            break;
        case MODE_ALL:
            if (procs)
                display_processes(procs, 15);
            printf("\n");
            display_network(&net);
            printf("\n");
            display_alerts(&alerts);
            break;
        }
        } /* end else (non-JSON) */

        /* Rotate snapshots for CPU delta calculation. */
        if (procs) {
            memcpy(&snap_prev, &snap_curr, sizeof(proc_snapshot_t));
            has_prev = 1;
        }

        if (once)
            break;

        sleep((unsigned)interval);
    }

    if (!use_json)
        printf("\n%sSysWatch terminated.%s\n", c_cyan(), c_reset());
    return 0;
}
