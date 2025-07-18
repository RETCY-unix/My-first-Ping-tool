#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>

#define PACKET_SIZE 64
#define MAX_WAIT_TIME 5
#define PING_SLEEP_RATE 1000000

// Global variables for signal handling
int ping_loop = 1;
int total_packets = 0;
int received_packets = 0;
double total_time = 0.0;
double min_time = 999999.0;
double max_time = 0.0;

// Structure for ping packet
struct ping_pkt {
    struct icmphdr hdr;
    char msg[PACKET_SIZE - sizeof(struct icmphdr)];
};

// Calculate checksum
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    // Make 16 bit words out of every two adjacent 8 bit words and 
    // calculate the sum of all 16 bit words
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    // Add the last byte if present
    if (len == 1) {
        sum += *(unsigned char*)buf << 8;
    }

    // Fold 32-bit sum to 16 bits: add carrier to result
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    result = ~sum;
    return result;
}

// Signal handler for Ctrl+C
void intHandler(int dummy) {
    ping_loop = 0;
}

// Get current time in microseconds
long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL);
    return te.tv_sec * 1000000LL + te.tv_usec;
}

// DNS lookup function
char* dns_lookup(char* addr_host, struct sockaddr_in* addr_con) {
    struct hostent* host_entry;
    char* ip_str = malloc(INET_ADDRSTRLEN);
    
    if ((host_entry = gethostbyname(addr_host)) == NULL) {
        free(ip_str);
        return NULL;
    }
    
    strcpy(ip_str, inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0])));
    addr_con->sin_family = host_entry->h_addrtype;
    addr_con->sin_port = 0;
    addr_con->sin_addr.s_addr = *(long*)host_entry->h_addr;
    
    return ip_str;
}

// Reverse DNS lookup
char* reverse_dns_lookup(char* ip_addr) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr_in);
    char* hostname = malloc(NI_MAXHOST);
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    
    if (getnameinfo((struct sockaddr*)&addr, len, hostname, NI_MAXHOST, NULL, 0, NI_NAMEREQD)) {
        strcpy(hostname, ip_addr);
    }
    
    return hostname;
}

// Send ping packet
int send_ping(int sock_fd, struct sockaddr_in* addr_con, char* hostname, int seq_num) {
    struct ping_pkt pckt;
    struct timespec time_start, time_end;
    long long time_sent, time_received;
    int msg_count = 0;
    
    // Clear packet
    memset(&pckt, 0, sizeof(pckt));
    
    // Fill packet
    pckt.hdr.type = ICMP_ECHO;
    pckt.hdr.un.echo.id = getpid();
    pckt.hdr.un.echo.sequence = seq_num;
    
    // Fill data part
    for (int i = 0; i < sizeof(pckt.msg) - 1; i++) {
        pckt.msg[i] = i + '0';
    }
    pckt.msg[sizeof(pckt.msg) - 1] = '\0';
    
    // Calculate checksum
    pckt.hdr.checksum = 0;
    pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
    
    // Send packet
    time_sent = current_timestamp();
    if (sendto(sock_fd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr_con, sizeof(*addr_con)) <= 0) {
        printf("Packet Sending Failed!\n");
        return -1;
    }
    
    // Receive packet
    struct sockaddr_in r_addr;
    socklen_t addr_len = sizeof(r_addr);
    char recv_buffer[1024];
    
    if (recvfrom(sock_fd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr*)&r_addr, &addr_len) <= 0) {
        printf("Packet receive failed!\n");
        return -1;
    }
    
    time_received = current_timestamp();
    double rtt = (time_received - time_sent) / 1000.0;
    
    // Parse the reply
    struct iphdr* ip_hdr = (struct iphdr*)recv_buffer;
    struct icmphdr* icmp_hdr = (struct icmphdr*)(recv_buffer + (ip_hdr->ihl * 4));
    
    if (icmp_hdr->type == ICMP_ECHOREPLY && icmp_hdr->un.echo.id == getpid()) {
        printf("64 bytes from %s: icmp_seq=%d ttl=%d time=%.1f ms\n", 
               hostname, seq_num, ip_hdr->ttl, rtt);
        
        received_packets++;
        total_time += rtt;
        
        if (rtt < min_time) min_time = rtt;
        if (rtt > max_time) max_time = rtt;
        
        return 0;
    }
    
    return -1;
}

// Print ping statistics
void print_statistics(char* hostname, char* ip_addr) {
    printf("\n--- %s ping statistics ---\n", hostname);
    printf("%d packets transmitted, %d received, %.0f%% packet loss\n", 
           total_packets, received_packets, 
           ((total_packets - received_packets) / (double)total_packets) * 100.0);
    
    if (received_packets > 0) {
        printf("rtt min/avg/max = %.1f/%.1f/%.1f ms\n", 
               min_time, total_time / received_packets, max_time);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <hostname/IP>\n", argv[0]);
        return 1;
    }
    
    char* hostname = argv[1];
    char* ip_addr;
    struct sockaddr_in addr_con;
    int sock_fd;
    int seq_num = 1;
    
    // Setup signal handler
    signal(SIGINT, intHandler);
    
    // DNS lookup
    ip_addr = dns_lookup(hostname, &addr_con);
    if (ip_addr == NULL) {
        printf("ping: cannot resolve %s: Unknown host\n", hostname);
        return 1;
    }
    
    // Create socket
    sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock_fd < 0) {
        printf("Socket creation failed. Are you running as root?\n");
        printf("Try: sudo %s %s\n", argv[0], hostname);
        return 1;
    }
    
    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = MAX_WAIT_TIME;
    timeout.tv_usec = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    printf("PING %s (%s): 56 data bytes\n", hostname, ip_addr);
    
    // Main ping loop
    while (ping_loop) {
        total_packets++;
        
        if (send_ping(sock_fd, &addr_con, ip_addr, seq_num) == -1) {
            printf("Request timeout for icmp_seq %d\n", seq_num);
        }
        
        seq_num++;
        usleep(PING_SLEEP_RATE);
    }
    
    // Print statistics
    print_statistics(hostname, ip_addr);
    
    // Cleanup
    close(sock_fd);
    free(ip_addr);
    
    return 0;
}