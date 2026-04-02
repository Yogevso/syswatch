/*
 * network.c — Network connection monitoring via /proc/net
 *
 * Parses /proc/net/tcp and /proc/net/udp to enumerate active
 * connections with local/remote addresses, ports, and TCP state.
 */

#include "network.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ---------- internal helpers ---------- */

/* Convert a hex-encoded IPv4 address from /proc/net to dotted-decimal. */
static void hex_to_ip(const char *hex, char *out, size_t out_len)
{
    unsigned int addr;
    if (sscanf(hex, "%X", &addr) != 1) {
        snprintf(out, out_len, "0.0.0.0");
        return;
    }

    /* Kernel stores in network byte order on little-endian, but the hex
     * representation in /proc/net/tcp is already in host order for the
     * individual octets, so we just extract directly. */
    snprintf(out, out_len, "%u.%u.%u.%u",
             (addr)       & 0xFF,
             (addr >> 8)  & 0xFF,
             (addr >> 16) & 0xFF,
             (addr >> 24) & 0xFF);
}

static uint16_t hex_to_port(const char *hex)
{
    unsigned int port;
    if (sscanf(hex, "%X", &port) != 1)
        return 0;
    return (uint16_t)port;
}

/* Parse one /proc/net file (tcp or udp). */
static int parse_proc_net(const char *path, net_proto_t proto,
                          net_snapshot_t *snap)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        return -1;

    char line[512];
    /* Skip header line. */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) && snap->count < MAX_CONNECTIONS) {
        char local_hex[64], remote_hex[64];
        unsigned int state;
        unsigned long inode = 0;

        /* Format: sl local_address rem_address st tx_queue:rx_queue tr:tm->when retrnsmt uid timeout inode */
        /* Example: 0: 0100007F:1F90 00000000:0000 0A 00000000:00000000 00:00000000 00000000  1000 0 12345 ... */
        int matched = sscanf(line, " %*d: %63s %63s %X %*s %*s %*s %*u %*d %lu",
                             local_hex, remote_hex, &state, &inode);
        if (matched < 3)
            continue;

        net_conn_t *c = &snap->conns[snap->count];
        c->proto = proto;
        c->state = (tcp_state_t)state;
        c->inode = (matched >= 4) ? inode : 0;

        /* Split address:port */
        char *colon;

        colon = strchr(local_hex, ':');
        if (!colon) continue;
        *colon = '\0';
        hex_to_ip(local_hex, c->local_addr, sizeof(c->local_addr));
        c->local_port = hex_to_port(colon + 1);

        colon = strchr(remote_hex, ':');
        if (!colon) continue;
        *colon = '\0';
        hex_to_ip(remote_hex, c->remote_addr, sizeof(c->remote_addr));
        c->remote_port = hex_to_port(colon + 1);

        snap->count++;
    }

    fclose(fp);
    return 0;
}

/* ---------- public API ---------- */

const char *tcp_state_str(tcp_state_t state)
{
    switch (state) {
    case TCP_ST_ESTABLISHED: return "ESTABLISHED";
    case TCP_ST_SYN_SENT:    return "SYN_SENT";
    case TCP_ST_SYN_RECV:    return "SYN_RECV";
    case TCP_ST_FIN_WAIT1:   return "FIN_WAIT1";
    case TCP_ST_FIN_WAIT2:   return "FIN_WAIT2";
    case TCP_ST_TIME_WAIT:   return "TIME_WAIT";
    case TCP_ST_CLOSE:       return "CLOSE";
    case TCP_ST_CLOSE_WAIT:  return "CLOSE_WAIT";
    case TCP_ST_LAST_ACK:    return "LAST_ACK";
    case TCP_ST_LISTEN:      return "LISTEN";
    case TCP_ST_CLOSING:     return "CLOSING";
    default:                 return "UNKNOWN";
    }
}

int net_snapshot(net_snapshot_t *snap)
{
    snap->count = 0;

    /* Parse TCP connections. */
    parse_proc_net("/proc/net/tcp", PROTO_TCP, snap);

    /* Parse UDP connections. */
    parse_proc_net("/proc/net/udp", PROTO_UDP, snap);

    return 0;
}

/* Read socket inodes from /proc/<pid>/fd and count matches in snapshot. */
int net_count_by_pid(const net_snapshot_t *snap, pid_t pid)
{
    char fd_path[64];
    snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd", (int)pid);

    DIR *dir = opendir(fd_path);
    if (!dir)
        return 0;

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char link_path[320];
        char target[256];
        snprintf(link_path, sizeof(link_path), "%s/%s", fd_path, entry->d_name);

        ssize_t len = readlink(link_path, target, sizeof(target) - 1);
        if (len <= 0)
            continue;
        target[len] = '\0';

        /* Socket FDs look like "socket:[12345]" */
        unsigned long inode;
        if (sscanf(target, "socket:[%lu]", &inode) != 1)
            continue;

        /* Match against connections in snapshot. */
        for (int i = 0; i < snap->count; i++) {
            if (snap->conns[i].inode == inode) {
                count++;
                break;
            }
        }
    }
    closedir(dir);
    return count;
}
