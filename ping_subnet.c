#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

int ping_ip(const char *ip, FILE *fp) {
    char command[256];
    int success_count = 0;

    // Format the ping command to send one packet
    snprintf(command, sizeof(command), "ping -c 1 -W 0.5 %s > /dev/null 2>&1", ip);

    // Execute the ping command and check the result
    if (system(command) == 0) { // system() returns 0 if successful
        success_count++;
        fprintf(fp, "%s\n", ip);
        fflush(fp);
        // If the first ping is successful send two more pings
        for (int i = 0; i < 2; i++) {
            if (system(command) == 0) {
                success_count++;
            }
        }
    } else {
        // If the first ping fails
        if (system(command) == 0) {
            success_count++;
            fprintf(fp, "%s\n", ip);
            fflush(fp);
        }
    }

    return success_count;
}

volatile sig_atomic_t keep_running = 1;

void handle_signal(int sig) {
    keep_running = 0;
    printf("\nStopping, please wait...\n");
}

int main() {
    FILE *fp = fopen("responders.txt", "w");
    if (!fp) {
            perror("Failed to open file");
            return 1;
    }
    
    signal(SIGINT, handle_signal);

    char cidr_input[100];
    char *ip_address;
    char *subnet_mask;
    struct in_addr ip_addr, net_addr, broad_addr;
    int mask;
    
    printf("Enter IP/CIDR (e.g. 192.168.1.0/24): ");
    scanf("%s", cidr_input);

    ip_address = strtok(cidr_input, "/");
    subnet_mask = strtok(NULL, "/");

    // Convert IP and mask to binary format
    inet_pton(AF_INET, ip_address, &ip_addr);
    mask = atoi(subnet_mask);

    unsigned long total_ips = (1UL << (32 - mask)) - 2;
    printf ("IPs to Check: %lu\n", total_ips);
    // Calculate the network address
    unsigned long mask_binary = ~((1 << (32 - mask)) - 1);
    net_addr.s_addr = ip_addr.s_addr & htonl(mask_binary);

    // Calculate the broadcast address
    broad_addr.s_addr = net_addr.s_addr | htonl(~mask_binary);
    
    time_t start_time, current_time;
    time(&start_time);

    char net_str[INET_ADDRSTRLEN];
    char broad_str[INET_ADDRSTRLEN];
    
    inet_ntop(AF_INET, &net_addr, net_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &broad_addr, broad_str, INET_ADDRSTRLEN);

    printf("Starting IP: %s\nEnding IP: %s\n", net_str, broad_str);  

    int successful_pings = 0;
    int unsuccessful_pings = 0;

    for (unsigned long addr = ntohl(net_addr.s_addr) + 1; addr < ntohl(broad_addr.s_addr) && keep_running; addr++) {
        struct in_addr current_addr;
        current_addr.s_addr = htonl(addr);
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &current_addr, ip_str, INET_ADDRSTRLEN);
        
        time(&current_time);
        int elapsed_time = (int)difftime(current_time, start_time);

        printf("\rRuntime: %d seconds  Current IP: %s    ", elapsed_time, ip_str);
        fflush(stdout);

        int result = ping_ip(ip_str, fp);
        if (result > 0) {
            successful_pings++;
        } else {
            unsuccessful_pings++;
        }
    }

    fclose(fp);
    
    printf("\nFinished\nResponders: %d\nNonresponders: %d\n", successful_pings, unsuccessful_pings);

    return 0;
}
