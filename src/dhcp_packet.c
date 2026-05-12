#include "../include/dhcp.h"

// ================= ETH HEADER =================
struct eth_hdr {
    unsigned char dest[6];
    unsigned char src[6];
    unsigned short type;
} __attribute__((packed));

// ================= GLOBAL =================
static int sockfd;
static int ifindex;
static unsigned char src_mac[6];

// ================= CHECKSUM =================
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;

    for (; len > 1; len -= 2)
        sum += *buf++;

    if (len == 1)
        sum += *(unsigned char*)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return ~sum;
}

// ================= INIT SOCKET =================
int init_socket(char *iface) {

    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("socket failed");
        exit(1);
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);

    // get index
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        exit(1);
    }
    ifindex = ifr.ifr_ifindex;

    // get MAC
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("SIOCGIFHWADDR");
        exit(1);
    }
    memcpy(src_mac, ifr.ifr_hwaddr.sa_data, 6);

    // bind
    struct sockaddr_ll addr = {0};
    addr.sll_family   = AF_PACKET;
    addr.sll_ifindex  = ifindex;
    addr.sll_protocol = htons(ETH_P_ALL);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    return sockfd;
}

// ================= BUILD PACKET =================
int build_packet(char *buffer,
                 unsigned char *dst_mac,
                 uint32_t src_ip,
                 uint32_t dst_ip,
                 uint16_t src_port,
                 uint16_t dst_port,
                 char *payload,
                 int payload_len) {

    struct eth_hdr *eth = (struct eth_hdr *)buffer;
    struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct eth_hdr));
    struct udphdr *udp = (struct udphdr *)(buffer + sizeof(struct eth_hdr) + sizeof(struct iphdr));
    char *data = buffer + sizeof(struct eth_hdr) + sizeof(struct iphdr) + sizeof(struct udphdr);

    // ETH
    memcpy(eth->dest, dst_mac, 6);
    memcpy(eth->src, src_mac, 6);
    eth->type = htons(0x0800);

    // DATA
    memcpy(data, payload, payload_len);

    // UDP
    udp->source = htons(src_port);
    udp->dest   = htons(dst_port);
    udp->len    = htons(sizeof(struct udphdr) + payload_len);
    udp->check  = 0;

    // IP
    ip->ihl = 5;
    ip->version = 4;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + payload_len);
    ip->ttl = 64;
    ip->protocol = IPPROTO_UDP;
    ip->saddr = src_ip;
    ip->daddr = dst_ip;
    ip->check = checksum(ip, sizeof(struct iphdr));

    return sizeof(struct eth_hdr) +
           sizeof(struct iphdr) +
           sizeof(struct udphdr) +
           payload_len;
}

// ================= SEND =================
int send_packet(char *buffer, int len, unsigned char *dst_mac) {

    struct sockaddr_ll addr = {0};
    addr.sll_ifindex = ifindex;
    addr.sll_halen   = 6;

    memcpy(addr.sll_addr, dst_mac, 6);

    return sendto(sockfd, buffer, len, 0,
                  (struct sockaddr *)&addr, sizeof(addr));
}

// ================= RECEIVE =================
int recv_packet(char *buffer, unsigned char *recv_src_mac) {

    int len = recv(sockfd, buffer, 1500, 0);
    if (len <= 0) return -1;

    struct eth_hdr *eth = (struct eth_hdr *)buffer;

    // filter: only IP
    if (ntohs(eth->type) != 0x0800)
        return -1;

    // ignore self packets
    if (memcmp(eth->src, src_mac, 6) == 0)
        return -1;

    memcpy(recv_src_mac, eth->src, 6);

    struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct eth_hdr));

    if (ip->protocol != IPPROTO_UDP)
        return -1;

    struct udphdr *udp = (struct udphdr *)(buffer + sizeof(struct eth_hdr) + ip->ihl * 4);

    // only DHCP port (server side 1111 OR client side 2222)
    if (ntohs(udp->dest) != 1111 && ntohs(udp->dest) != 2222)
        return -1;

    return len;
}

// ================= SERIALIZE =================
void serialize_packet(dhcp_packet_t *pkt, char *buffer) {
    memcpy(buffer, pkt, sizeof(dhcp_packet_t));
}

void deserialize_packet(char *buffer, dhcp_packet_t *pkt) {
    memcpy(pkt, buffer, sizeof(dhcp_packet_t));
}