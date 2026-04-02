#ifndef ALERTS_H
#define ALERTS_H

#include "process.h"
#include "network.h"

#define MAX_ALERTS 128
#define ALERT_MSG_LEN 256

/* Default thresholds */
#define DEFAULT_CPU_THRESHOLD      80.0   /* percent */
#define DEFAULT_CONN_THRESHOLD     100    /* number of connections */
#define DEFAULT_MEM_THRESHOLD_KB   (1024 * 1024) /* 1 GB */

/* Suspicious ports — common backdoor / debug ports */
#define SUSPICIOUS_PORT_COUNT 6
extern const uint16_t SUSPICIOUS_PORTS[SUSPICIOUS_PORT_COUNT];

typedef enum {
    ALERT_HIGH_CPU,
    ALERT_HIGH_MEM,
    ALERT_CONN_SPIKE,
    ALERT_SUSPICIOUS_PORT
} alert_type_t;

typedef struct {
    alert_type_t type;
    char message[ALERT_MSG_LEN];
} alert_t;

typedef struct {
    double cpu_threshold;
    int conn_threshold;
    unsigned long mem_threshold_kb;
} alert_config_t;

typedef struct {
    alert_t alerts[MAX_ALERTS];
    int count;
} alert_list_t;

/* Initialize config with defaults. */
void alert_config_init(alert_config_t *cfg);

/* Evaluate process snapshot and network snapshot against thresholds.
 * Populates |out| with triggered alerts. */
void alert_evaluate(const alert_config_t *cfg,
                    const proc_snapshot_t *procs,
                    const net_snapshot_t *net,
                    alert_list_t *out);

#endif /* ALERTS_H */
