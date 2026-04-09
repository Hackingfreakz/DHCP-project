#include "../include/dhcp.h"
#include <time.h>


#define SERVER_PORT 1111
#define BUFFER_SIZE 1024




int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: %s <client_id>\n", argv[0]);
        return 1;
    }

    int sockfd;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    char buffer[BUFFER_SIZE];
    dhcp_packet_t pkt, response;

   
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    
    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
                   &broadcast, sizeof(broadcast)) < 0) {
        perror("Broadcast enable failed");
        exit(1);
    }

    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    log_event("INFO", "Client started");

    discover:
    memset(&pkt, 0, sizeof(pkt));
    pkt.msg_type = DHCP_DISCOVER;
    strcpy(pkt.client_id, argv[1]);

    serialize_packet(&pkt, buffer);
    
    
    
    sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));

    log_event("INFO", "DISCOVER sent");

   
    struct sockaddr_in from_addr;
    socklen_t addr_len = sizeof(from_addr);

    if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0,(struct sockaddr*)&from_addr, &addr_len) < 0) {
        perror("Failed to receive OFFER");
        exit(1);
    }

    deserialize_packet(buffer, &response);
    if(response.msg_type == DHCP_NAK ){
        printf("IP Pool exhausted");
        log_event("INFO","NAK received");
        sleep(response.lease_time);
        goto discover;
    }
    else if(response.msg_type != DHCP_OFFER){
        log_event("ERROR", "Invalid OFFER received");
        exit(1);
    }
    
    char offered_ip[16];
    strcpy(offered_ip, response.assigned_ip);

    char logbuf[128];
    sprintf(logbuf, "OFFER received: %s", offered_ip);
    log_event("INFO", logbuf);

    //sleep(20);
    memset(&pkt, 0, sizeof(pkt));
    pkt.msg_type = DHCP_REQUEST;
    strcpy(pkt.client_id, argv[1]);
    strcpy(pkt.requested_ip, offered_ip);

    serialize_packet(&pkt, buffer);

    sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0,
           (struct sockaddr*)&from_addr, addr_len);
    //printf("DEBUG requested_ip: %s\n", offered_ip);
    log_event("INFO", "REQUEST sent");


    if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                 (struct sockaddr*)&from_addr, &addr_len) < 0) {
        perror("Failed to receive ACK");
        exit(1);
    }

    deserialize_packet(buffer, &response);

    if (response.msg_type != DHCP_ACK) {
        log_event("ERROR", "Invalid ACK received");
        exit(1);
    }
    char time_buff[64];

    time_t now=time(NULL)+response.lease_time;
    struct tm *t = localtime(&now);
    strftime(time_buff,sizeof(time_buff), "%H:%M:%S",t);
    sprintf(logbuf, "ACK received: IP=%s Lease=%d Expiry time=%s client_id=%s",
            response.assigned_ip, response.lease_time,time_buff,response.client_id);
    log_event("INFO", logbuf);


    printf("\n IP Assigned: %s\n", response.assigned_ip);
    printf(" Subnet Mask: %s\n",response.subnet_mask);
    printf(" Default Gateway: %s\n",response.gateway);
    printf(" Lease Time: %d seconds\n\n", response.lease_time);

    close(sockfd);
    return 0;
}
