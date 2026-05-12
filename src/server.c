#include "../include/dhcp.h"
#include <ifaddrs.h>

#define SERVER_PORT 1111
#define BUFFER_SIZE 1024

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
int main() {
    char buffer[BUFFER_SIZE];
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
	
	list_interfaces();
	char interface[20];
	printf("Enter interface : ");
	scanf("%s",interface);
    sockfd = init_socket(interface, SERVER_PORT);
	
    log_event("INFO", "DHCP Server started");
    load_config();
    init_leases();
    while (1) {
        dhcp_packet_t pkt, response;
        memset( & response, 0, sizeof(response));
        recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr * ) & client_addr, & addr_len);

        deserialize_packet(buffer, & pkt);
        if (pkt.msg_type == DHCP_DISCOVER) {

            char * ip = get_ip_from_lease(pkt.client_id);
            if (!ip) {
                log_event("WARN", "IP pool exhausted");
                response.msg_type = DHCP_NAK;
                response.lease_time = get_lease_time();
                strcpy(response.client_id, pkt.client_id);
                serialize_packet( & response, buffer);
                sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0, (struct sockaddr * ) & client_addr, addr_len);
                log_event("INFO", "NAK Sent");
                continue;
            }

            response.msg_type = DHCP_OFFER;
            strcpy(response.client_id, pkt.client_id);
            strcpy(response.assigned_ip, ip);
            strcpy(response.subnet_mask, get_subnet_mask());
            strcpy(response.gateway, get_gateway());
            response.lease_time = get_lease_time();

            serialize_packet( & response, buffer);

            sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0, (struct sockaddr * ) & client_addr, addr_len);

            char logbuf[128];
            sprintf(logbuf, "OFFER → %s : %s", pkt.client_id, ip);
            log_event("INFO", logbuf);
        } else if (pkt.msg_type == DHCP_RENEW) {
            int ok = renew_lease(pkt.client_id, pkt.requested_ip);
	    cleanup_leases();
            memset( & response, 0, sizeof(response));
            if (ok) {
                response.msg_type = DHCP_ACK;
                strcpy(response.client_id, pkt.client_id);
                strcpy(response.assigned_ip, pkt.requested_ip);
                strcpy(response.subnet_mask, get_subnet_mask());
                strcpy(response.gateway, get_gateway());
                response.lease_time = get_lease_time();

                serialize_packet( & response, buffer);

                sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0,
                    (struct sockaddr * ) & client_addr, addr_len);
               
                char logbuf[128];
                sprintf(logbuf, "RENEW-ACK → %s : %s",
                    pkt.client_id, pkt.requested_ip);
                log_event("INFO", logbuf);
            } else {
                response.msg_type = DHCP_NAK;

                serialize_packet( & response, buffer);
                sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0,
                    (struct sockaddr * ) & client_addr, addr_len);

                log_event("WARN", "RENEW denied");
            }
        } else if (pkt.msg_type == DHCP_REQUEST) {

              int status = confirm_lease(pkt.client_id, pkt.requested_ip);
              memset(&response, 0, sizeof(response));

               if (!status) {
        	log_event("ERROR", "Lease confirmation failed");
		response.msg_type = DHCP_NAK;
        	strcpy(response.client_id, pkt.client_id);
        	serialize_packet(&response, buffer);
        	sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0,
               (struct sockaddr *)&client_addr, addr_len);
       		 continue;
    		}

    		response.msg_type = DHCP_ACK;
   		strcpy(response.client_id, pkt.client_id);
    		strcpy(response.assigned_ip, pkt.requested_ip);
    		strcpy(response.subnet_mask, get_subnet_mask());
    		strcpy(response.gateway, get_gateway());
    		response.lease_time = get_lease_time();
    		serialize_packet(&response, buffer);
    		sendto(sockfd, buffer, sizeof(dhcp_packet_t), 0,
           	(struct sockaddr *)&client_addr, addr_len);
    		char logbuf[128];
    		sprintf(logbuf, "ACK → %s : %s", pkt.client_id, pkt.requested_ip);
    		log_event("INFO", logbuf);
	} else if (pkt.msg_type == DHCP_RELEASE) {
            log_event("INFO", "RELEASE received");
            lease_free(pkt.client_id);
            log_event("INFO", "IP freed");
        }
    }
}
