#include "../include/dhcp.h"

#include <stdio.h>

#include <signal.h>

#define SERVER_PORT 1111
#define BUFFER_SIZE 1024

int sockfd_global;
struct sockaddr_in server_addr_global;
socklen_t addr_len_global;
char client_id_global[MAX_CLIENT_ID];
char assigned_ip_global[IP_LEN];
void handle_exit(int sig) {
    printf("\n[CLIENT] Caught Ctrl+C → Sending RELEASE to server\n");

    dhcp_packet_t pkt;
    char buffer[BUFFER_SIZE];

    memset( & pkt, 0, sizeof(pkt));
    pkt.msg_type = DHCP_RELEASE;
    strcpy(pkt.client_id, client_id_global);
    strcpy(pkt.requested_ip, assigned_ip_global);

    serialize_packet( & pkt, buffer);

    sendto(sockfd_global, buffer, sizeof(pkt), 0,
        (struct sockaddr * ) & server_addr_global, addr_len_global);

    printf("[CLIENT] Release packet sent. Exiting...\n");
    close(sockfd_global);
    exit(0);
}
int main(int argc, char * argv[]) {
    if (argc < 2) {
        printf("Usage: %s <client_id>\n", argv[0]);
        return 1;
    }
    int sockfd;
    struct sockaddr_in server_addr;
    memset( & server_addr, 0, sizeof(server_addr));
    char buffer[BUFFER_SIZE];
    dhcp_packet_t pkt, response;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, & broadcast, sizeof(broadcast)) < 0) {
        perror("Broadcast enable failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    log_event("INFO", "Client started");
    sockfd_global = sockfd;
    server_addr_global = server_addr;
    addr_len_global = sizeof(server_addr);
    strcpy(client_id_global, argv[1]);
    signal(SIGINT, handle_exit);
    discover:
        memset( & pkt, 0, sizeof(pkt));
    pkt.msg_type = DHCP_DISCOVER;
    strcpy(pkt.client_id, argv[1]);

    serialize_packet( & pkt, buffer);

    sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0,
        (struct sockaddr * ) & server_addr, sizeof(server_addr));

    log_event("INFO", "DISCOVER sent");

    struct sockaddr_in from_addr;
    socklen_t addr_len = sizeof(from_addr);

    if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr * ) & from_addr, & addr_len) < 0) {
        perror("Failed to receive OFFER");
        exit(1);
    }

    server_addr = from_addr;
    server_addr_global = from_addr;
    addr_len_global = sizeof(from_addr);

    deserialize_packet(buffer, & response);
    if (response.msg_type == DHCP_NAK) {
        printf("IP Pool exhausted");
        log_event("INFO", "NAK received");
        sleep(response.lease_time);
        goto discover;
    } else if (response.msg_type != DHCP_OFFER) {
        log_event("ERROR", "Invalid OFFER received");
        exit(1);
    }

    char offered_ip[16];
    strcpy(offered_ip, response.assigned_ip);

    char logbuf[128];
    sprintf(logbuf, "OFFER received: %s", offered_ip);
    log_event("INFO", logbuf);

    memset( & pkt, 0, sizeof(pkt));
    pkt.msg_type = DHCP_REQUEST;
    strcpy(pkt.client_id, argv[1]);
    strcpy(pkt.requested_ip, offered_ip);

    serialize_packet( & pkt, buffer);

    sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0, (struct sockaddr * ) & server_addr, sizeof(server_addr));
    printf("DEBUG requested_ip: %s\n", offered_ip);
    log_event("INFO", "REQUEST sent");

    if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
            (struct sockaddr * ) & from_addr, & addr_len) < 0) {
        perror("Failed to receive ACK");
        exit(1);
    }

    deserialize_packet(buffer, & response);

    if (response.msg_type != DHCP_ACK) {
        log_event("ERROR", "Invalid ACK received");
        exit(1);
    }

    sprintf(logbuf, "ACK received: IP=%s Lease=%d",
        response.assigned_ip, response.lease_time);
    log_event("INFO", logbuf);

    printf("\n IP Assigned: %s\n", response.assigned_ip);
    printf(" Subnet Mask: %s\n", response.subnet_mask);
    printf(" Default Gateway: %s\n", response.gateway);
    printf(" Lease Time: %d seconds\n\n", response.lease_time);

    time_t start = time(NULL);
    while (1) {
        sleep(1);
        if (time(NULL) - start >= response.lease_time / 2) {
            log_event("INFO", "Sending RENEW request");
            memset( & pkt, 0, sizeof(pkt));
            pkt.msg_type = DHCP_RENEW;
            strcpy(pkt.client_id, argv[1]);
            strcpy(pkt.requested_ip, offered_ip);

            serialize_packet( & pkt, buffer);
            sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0, (struct sockaddr * ) & server_addr, sizeof(server_addr));
            if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr * ) & from_addr, & addr_len) > 0) {

                deserialize_packet(buffer, & response);

                if (response.msg_type == DHCP_ACK) {
                    printf("Lease renewed! New lease time: %d\n", response.lease_time);
                    start = time(NULL);
                } else {
                    printf("Lease renewal failed.\n");
                }
            }
        }
    }
}
