#include "../include/dhcp.h"


#define MAX_POOL 256
#define OFFER_TIMEOUT 30

lease_t leases[MAX_POOL];
char ip_pool[MAX_POOL][IP_LEN];
int pool_size = 0;

char subnet_mask[IP_LEN];
char gateway[IP_LEN];
int lease_time = 60;

unsigned int ip_to_int(char * ip) {
    struct in_addr addr;
    inet_aton(ip, & addr);
    return ntohl(addr.s_addr);
}

void int_to_ip(unsigned int ip, char * buffer) {
    struct in_addr addr;
    addr.s_addr = htonl(ip);
    strcpy(buffer, inet_ntoa(addr));
}

void load_config() {
    FILE * fp = fopen("config/server.conf", "r");
    if (!fp) {
        perror("Config file error");
        exit(1);
    }
    char line[128], key[32], value[32];
    char start_ip[IP_LEN], end_ip[IP_LEN];

    while (fgets(line, sizeof(line), fp)) {
        sscanf(line, "%[^=]=%s", key, value);
        if (strcmp(key, "POOL_START") == 0)
            strcpy(start_ip, value);
        else if (strcmp(key, "POOL_END") == 0)
            strcpy(end_ip, value);
        else if (strcmp(key, "SUBNET_MASK") == 0)
            strcpy(subnet_mask, value);
        else if (strcmp(key, "GATEWAY") == 0)
            strcpy(gateway, value);
        else if (strcmp(key, "LEASE_TIME") == 0)
            lease_time = atoi(value);
    }

    fclose(fp);
    unsigned int start = ip_to_int(start_ip);
    unsigned int end = ip_to_int(end_ip);

    if (end < start) {
        printf("Invalid IP range in config\n");
        exit(1);
    }

    for (unsigned int ip = start; ip <= end && pool_size < MAX_POOL; ip++) {
        int_to_ip(ip, ip_pool[pool_size]);
        pool_size++;
    }
}
void save_leases_to_file() {
    FILE *fp = fopen("leases.txt", "w");
    if (!fp) return;

    time_t now = time(NULL);

    for (int i = 0; i < pool_size; i++) {
        if (leases[i].state == ALLOCATED &&
            leases[i].expiry > now) {

            char time_str[64];
            struct tm *tm_info = localtime(&leases[i].expiry);

            strftime(time_str, sizeof(time_str),
                     "%Y-%m-%d %H:%M:%S", tm_info);

            fprintf(fp, "%s %s %s\n",
                    leases[i].client_id,
                    leases[i].ip,
                    time_str);
        }
    }

    fclose(fp);
}
void init_leases() {
    for (int i = 0; i < pool_size; i++) {
        leases[i].state = FREE;
        leases[i].expiry = 0;
        strcpy(leases[i].ip, ip_pool[i]);
        leases[i].client_id[0] = '\0';
    }
}

void cleanup_leases() {
    time_t now = time(NULL);

    for (int i = 0; i < pool_size; i++) {

        if (leases[i].state == OFFERED &&
            leases[i].expiry < now) {

            leases[i].state = FREE;
            leases[i].expiry = 0;
            leases[i].client_id[0] = '\0';
        }

        else if (leases[i].state == ALLOCATED &&
                 leases[i].expiry < now) {

            leases[i].state = FREE;
            leases[i].expiry = 0;
            leases[i].client_id[0] = '\0';
        }
    }
}
char * get_ip_from_lease(char * client_id) {
    cleanup_leases();


    for (int i = 0; i < pool_size; i++) {
        if ((leases[i].state == ALLOCATED || leases[i].state == OFFERED) &&
            strcmp(leases[i].client_id, client_id) == 0) {
            return leases[i].ip;
        }
    }


    for (int i = 0; i < pool_size; i++) {
        if (leases[i].state == FREE) {
            leases[i].state = OFFERED;
            strcpy(leases[i].client_id, client_id);
            leases[i].expiry = time(NULL) + OFFER_TIMEOUT;

            return leases[i].ip;
        }
    }

    return NULL;
}
int confirm_lease(char *client_id, char *ip) {
    cleanup_leases();
    time_t now = time(NULL);

    for (int i = 0; i < pool_size; i++) {

        if (strcmp(leases[i].ip, ip) == 0) {
            if (leases[i].state == FREE ||
                (leases[i].state == OFFERED &&
                 strcmp(leases[i].client_id, client_id) == 0) ||
                (leases[i].state == ALLOCATED &&
                 strcmp(leases[i].client_id, client_id) == 0)) {

                leases[i].state = ALLOCATED;
                strcpy(leases[i].client_id, client_id);
                leases[i].expiry = now + lease_time;

                save_leases_to_file();
                return 1;
            }

            return 0;
        }
    }

    return 0;
}

void lease_free(char *client_id) {
    for (int i = 0; i < pool_size; i++) {
        if (strcmp(leases[i].client_id, client_id) == 0) {

            leases[i].state = FREE;
            leases[i].expiry = 0;
            leases[i].client_id[0] = '\0';

            save_leases_to_file();

            return;
        }
    }
}
int renew_lease(char *client_id, char *ip) {
    time_t now = time(NULL);

    for (int i = 0; i < pool_size; i++) {

        if (strcmp(leases[i].ip, ip) == 0 &&
            strcmp(leases[i].client_id, client_id) == 0) {

            if (leases[i].state == ALLOCATED) {

                leases[i].state = ALLOCATED; 
                leases[i].expiry = now + lease_time;

                save_leases_to_file();
                return 1;
            }

            return 0;
        }
    }

    return 0;
}
char * get_subnet_mask() {
    return subnet_mask;
}

char * get_gateway() {
    return gateway;
}

int get_lease_time() {
    return lease_time;
}
