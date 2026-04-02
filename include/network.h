#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <sys/types.h>

#define MAX_CONNECTIONS 2048
#define ADDR_STR_LEN   64

typedef enum {
    PROTO_TCP,
    PROTO_UDP
} net_proto_t;

/* TCP state codes from the kernel */
typedef enum {
    TCP_ST_ESTABLISHED = 1,
    TCP_ST_SYN_SENT,
    TCP_ST_SYN_RECV,
    TCP_ST_FIN_WAIT1,
    TCP_ST_FIN_WAIT2,
    TCP_ST_TIME_WAIT,
    TCP_ST_CLOSE,
    TCP_ST_CLOSE_WAIT,
    TCP_ST_LAST_ACK,
    TCP_ST_LISTEN,
    TCP_ST_CLOSING
} tcp_state_t;

typedef struct {
    net_proto_t proto;
    char local_addr[ADDR_STR_LEN];
    uint16_t local_port;
    char remote_addr[ADDR_STR_LEN];
    uint16_t remote_port;
    tcp_state_t state; /* only meaningful for TCP */
    unsigned long inode; /* socket inode for PID correlation */
} net_conn_t;

typedef struct {
    net_conn_t conns[MAX_CONNECTIONS];
    int count;
} net_snapshot_t;

/* Take a snapshot of network connections (TCP + UDP). */
int net_snapshot(net_snapshot_t *snap);

/* Return human-readable string for a TCP state. */
const char *tcp_state_str(tcp_state_t state);

/* Count connections belonging to a given PID (via /proc/<pid>/fd socket inodes). */
int net_count_by_pid(const net_snapshot_t *snap, pid_t pid);

#endif /* NETWORK_H */
