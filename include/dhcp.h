#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>

#define MAX_CLIENT_ID 32
#define IP_LEN 16

typedef enum {
    DHCP_DISCOVER = 1,
        DHCP_OFFER,
        DHCP_REQUEST,
        DHCP_ACK,
        DHCP_NAK,
        DHCP_RENEW,
        DHCP_RELEASE
}
dhcp_msg_type_t;

typedef struct {
    dhcp_msg_type_t msg_type;
    char client_id[MAX_CLIENT_ID];
    char requested_ip[IP_LEN];
    char assigned_ip[IP_LEN];
    char subnet_mask[IP_LEN];
    char gateway[IP_LEN];
    int lease_time;
}
dhcp_packet_t;

typedef enum {
    FREE = 0,
        OFFERED,
        ALLOCATED
}
state;

typedef struct {
    char ip[IP_LEN];
    char client_id[MAX_CLIENT_ID];
    time_t expiry;
    state state;
}
lease_t;

void serialize_packet(dhcp_packet_t * pkt, char * buffer);
void deserialize_packet(char * buffer, dhcp_packet_t * pkt);
void log_event(const char * level, const char * msg);
char * get_subnet_mask();
char * get_gateway();
int get_lease_time();
void load_config();
void lease_free(char * ip);
int renew_lease(char * client_id, char * ip);
void init_leases();
char * get_ip_from_lease(char * client_id);
int confirm_lease(char * client_id, char * ip);
