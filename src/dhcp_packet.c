#include "../include/dhcp.h"

void serialize_packet(dhcp_packet_t *pkt, char *buffer) {
    memcpy(buffer, pkt, sizeof(dhcp_packet_t));
}

void deserialize_packet(char *buffer, dhcp_packet_t *pkt) {
    memcpy(pkt, buffer, sizeof(dhcp_packet_t));
}

