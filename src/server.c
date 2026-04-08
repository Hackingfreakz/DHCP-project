#include "../include/dhcp.h"

#define SERVER_PORT 1111
#define BUFFER_SIZE 1024

int sockfd;
struct sockaddr_in server_addr, client_addr;

 
//void log_event(const char *level, const char *msg);

int main() {
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
	    perror("socket failed");
	    exit(1);
    }
    int broadcast_enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,&broadcast_enable, sizeof(broadcast_enable));
    memset(&server_addr, 0, sizeof(server_addr));   

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
	    perror("bind failed");  
	    exit(1);
	}
log_event("INFO", "DHCP Server started");
	load_config();
	init_leases();
    while (1) {
        dhcp_packet_t pkt, response;
        recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                 (struct sockaddr*)&client_addr, &addr_len);

        deserialize_packet(buffer, &pkt);
        if (pkt.msg_type == DHCP_DISCOVER) {

            char *ip = get_ip_from_lease(pkt.client_id);

            if (!ip) {
                log_event("WARN", "IP pool exhausted");
                continue;
            }

            memset(&response, 0, sizeof(response));

            response.msg_type = DHCP_OFFER;
            strcpy(response.client_id, pkt.client_id);
            strcpy(response.assigned_ip, ip);

            strcpy(response.subnet_mask, get_subnet_mask());
            strcpy(response.gateway, get_gateway());
            response.lease_time = get_lease_time();

            serialize_packet(&response, buffer);

            sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0,
                   (struct sockaddr*)&client_addr, addr_len);

            char logbuf[128];
            sprintf(logbuf, "OFFER → %s : %s", pkt.client_id, ip);
            log_event("INFO", logbuf);
        }

      
        else if (pkt.msg_type == DHCP_REQUEST) {

            
            int status = confirm_lease(pkt.client_id, pkt.requested_ip);

            if (!status) {
                log_event("ERROR", "Lease confirmation failed");
                continue;
            }

            memset(&response, 0, sizeof(response));
            response.msg_type = DHCP_ACK;
            strcpy(response.client_id, pkt.client_id);
            strcpy(response.assigned_ip, pkt.requested_ip);

            strcpy(response.subnet_mask, get_subnet_mask());
            strcpy(response.gateway, get_gateway());
            response.lease_time = get_lease_time();

            serialize_packet(&response, buffer);

            sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0,
                   (struct sockaddr*)&client_addr, addr_len);

            char logbuf[128];
            sprintf(logbuf, "ACK → %s : %s", pkt.client_id, pkt.requested_ip);
            log_event("INFO", logbuf);
        }
    }

    close(sockfd);
    return 0;
}
