#include "../include/dhcp.h"
#include <ifaddrs.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#define SERVER_PORT 1111
#define BUFFER_SIZE 1024

int sockfd_global;
struct sockaddr_in server_addr_global;
socklen_t addr_len_global;
char interface[20];
char client_id_global[MAX_CLIENT_ID];
char assigned_ip_global[IP_LEN];


void handle_exit(int sig) {
    printf("\n[CLIENT] Caught Ctrl+C → Sending RELEASE to server\n");

    dhcp_packet_t pkt;
    char buffer[BUFFER_SIZE];

    memset(&pkt, 0, sizeof(pkt));
    pkt.msg_type = DHCP_RELEASE;
    strcpy(pkt.client_id, client_id_global);
    strcpy(pkt.requested_ip, assigned_ip_global);

    serialize_packet(&pkt, buffer);

    sendto(sockfd_global, buffer, sizeof(pkt), 0,
           (struct sockaddr *)&server_addr_global, addr_len_global);

    printf("[CLIENT] Release packet sent. Exiting...\n");
    close(sockfd_global);
    exit(0);
}

void list_interfaces(){
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    printf("Available network interfaces:\n");
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr != NULL && ifa->ifa_addr->sa_family == AF_PACKET) {
            printf("- %s\n", ifa->ifa_name);
        }
    }
    freeifaddrs(ifaddr);

    return;
}

int main(int argc, char *argv[]) {
    
    list_interfaces();
    srand(time(NULL));
    sprintf(client_id_global,"%x",rand()%12342341);
	
	printf("Enter a interface name: ");
    scanf("%s",interface);
    //printf("%s",client_id_global);
	
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    dhcp_packet_t req, res;

    struct sockaddr_in from_addr;
    socklen_t addr_len = sizeof(from_addr);

    int wait_time = 2;  

    log_event("INFO", "Client started");

discover:

    sockfd =  init_socket(interface, SERVER_PORT);
  
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    sockfd_global = sockfd;
    server_addr_global = server_addr;
    addr_len_global = sizeof(server_addr);

    signal(SIGINT, handle_exit);

    memset(&pkt, 0, sizeof(pkt));
    pkt.msg_type = DHCP_DISCOVER;
    strcpy(pkt.client_id, client_id_global);

    buffer = buildPacket(&pkt);

    sendto(sockfd, buffer, sizeof(pkt), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));

    log_event("INFO", "DISCOVER sent");

    if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                 (struct sockaddr *)&from_addr, &addr_len) < 0) {
        perror("Failed to receive OFFER");
        close(sockfd);
        sleep(wait_time);
        goto discover;
    }

    server_addr = from_addr;
    server_addr_global = from_addr;
    addr_len_global = sizeof(from_addr);

    parsePacket(&response);

    if (response.msg_type == DHCP_NAK) {
        printf("IP Pool exhausted, retrying in %d sec...\n", wait_time);

        sleep(wait_time);
        if (wait_time < 20) wait_time *= 2;

        close(sockfd);
        goto discover;
    }

    if (response.msg_type != DHCP_OFFER) {
        log_event("ERROR", "Invalid OFFER received");
        close(sockfd);
        exit(1);
    }

    char offered_ip[IP_LEN];
    strcpy(offered_ip, response.assigned_ip);

    char logbuf[128];
    sprintf(logbuf, "OFFER received: %s", offered_ip);
    log_event("INFO", logbuf);

    memset(&pkt, 0, sizeof(pkt));
    pkt.msg_type = DHCP_REQUEST;
    strcpy(pkt.client_id, argv[1]);
    strcpy(pkt.requested_ip, offered_ip);

    serialize_packet(&pkt, buffer);

    sendto(sockfd, buffer, sizeof(pkt), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));

    log_event("INFO", "REQUEST sent");

    if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                 (struct sockaddr *)&from_addr, &addr_len) < 0) {
        perror("Failed to receive ACK");
        close(sockfd);
        goto discover;
    }

    deserialize_packet(buffer, &response);

    if (response.msg_type == DHCP_ACK) {

        sprintf(logbuf, "ACK received: IP=%s Lease=%d",
                response.assigned_ip, response.lease_time);
        log_event("INFO", logbuf);

        strcpy(assigned_ip_global, response.assigned_ip);

    } else if (response.msg_type == DHCP_NAK) {

        log_event("WARN", "Request denied, retrying...");
        sleep(wait_time);

        if (wait_time < 20) wait_time *= 2;

        close(sockfd);
        goto discover;

    } else {
        log_event("ERROR", "Unexpected response");
        close(sockfd);
        exit(1);
    }

    wait_time = 2;

    printf("\n IP Assigned: %s\n", response.assigned_ip);
    printf(" Subnet Mask: %s\n", response.subnet_mask);
    printf(" Default Gateway: %s\n", response.gateway);
    printf(" Lease Time: %d seconds\n\n", response.lease_time);


    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    time_t start = time(NULL);

    while (1) {

        sleep(1);

        if (time(NULL) - start >= response.lease_time / 2) {

            int retries = 3;
            int success = 0;

        while (retries--) {

    log_event("INFO", "Sending RENEW request");

    memset(&pkt, 0, sizeof(pkt));
    pkt.msg_type = DHCP_RENEW;
    strcpy(pkt.client_id, argv[1]);
    strcpy(pkt.requested_ip, offered_ip);

    serialize_packet(&pkt, buffer);

    sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));

    memset(buffer, 0, BUFFER_SIZE);
    memset(&response, 0, sizeof(response));

    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                     (struct sockaddr *)&from_addr, &addr_len);

    if (n == sizeof(dhcp_packet_t)) {

        deserialize_packet(buffer, &response);

        if (response.msg_type == DHCP_ACK) {
            printf("Lease renewed! New lease time: %d\n",
                   response.lease_time);
            start = time(NULL);
            success = 1;
            break;
        } else {
            printf("Received non-ACK packet\n");
        }

    } else {
        printf("No valid response (n=%d)\n", n);
    }

    printf("Retrying renew...\n");
}

            if (!success) {
                printf("Lease renewal failed. Releasing IP...\n");

                memset(&pkt, 0, sizeof(pkt));
                pkt.msg_type = DHCP_RELEASE;
                strcpy(pkt.client_id, argv[1]);
                strcpy(pkt.requested_ip, offered_ip);

                serialize_packet(&pkt, buffer);

                sendto(sockfd, buffer, sizeof(pkt), 0,
                       (struct sockaddr *)&server_addr, sizeof(server_addr));

                close(sockfd);

                sleep(2);
		break;
               
            }
        }
    }
}
