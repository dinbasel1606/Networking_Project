#include "udp_file_transfer.h"

#define CLIENT_FILES_PATH "/home/din/files_transfer/client_files/" // Define client storage path

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void send_request(int sockfd, struct sockaddr_in *server_addr, const char *operation, const char *filename) {
    Packet packet, response;
    socklen_t server_len = sizeof(*server_addr);
    char buffer[BUFFER_SIZE];
    char full_path[512];  // Full path to the file
    FILE *file;
    uint16_t block = 0;

    // Construct full file path
    snprintf(full_path, sizeof(full_path), "%s%.200s", CLIENT_FILES_PATH, filename);

    if (strcmp(operation, "upload") == 0) {
        // Upload request
        packet.opcode = htons(OP_WRQ);
        strncpy(packet.data, filename, sizeof(packet.data) - 1);
        sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)server_addr, server_len);

        file = fopen(full_path, "rb");
        if (!file) die("Failed to open file");

        while (1) {
            memset(&packet, 0, sizeof(packet));
            packet.opcode = htons(OP_DATA);
            packet.block_number = htons(++block);
            size_t bytes_read = fread(packet.data, 1, DATA_SIZE, file);
            
            sendto(sockfd, &packet, 4 + bytes_read, 0, (struct sockaddr *)server_addr, server_len);
            recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);

            if (bytes_read < DATA_SIZE) break;  // End of file
        }
        fclose(file);

    } else if (strcmp(operation, "download") == 0) {
        // Download request
        packet.opcode = htons(OP_RRQ);
        strncpy(packet.data, filename, sizeof(packet.data) - 1);
        sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)server_addr, server_len);

        file = fopen(full_path, "wb");
        if (!file) die("Failed to create file");

        while (1) {
            ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
            if (n < 0) die("Error receiving data");

            memcpy(&response, buffer, sizeof(response));
            size_t bytes_received = n - 4; // Remove header size

            fwrite(response.data, 1, bytes_received, file);

            // Send acknowledgment
            memset(&packet, 0, sizeof(packet));
            packet.opcode = htons(OP_ACK);
            packet.block_number = response.block_number;
            sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)server_addr, server_len);

            if (bytes_received < DATA_SIZE) break; // End of file
        }
        fclose(file);

    } else if (strcmp(operation, "delete") == 0) {
        // Delete request
        packet.opcode = htons(OP_DELETE);
        strncpy(packet.data, filename, sizeof(packet.data) - 1);
        sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)server_addr, server_len);

        // Wait for acknowledgment
        recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        memcpy(&response, buffer, sizeof(response));

        if (ntohs(response.opcode) == OP_ACK) {
            printf("File deleted successfully.\n");
        } else {
            printf("File deletion failed.\n");
        }
    } else {
        fprintf(stderr, "Unknown operation: %s\n", operation);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <server_ip> <operation> <filename>\n", argv[0]);
        return 1;
    }

    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) die("Failed to create socket");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    send_request(sockfd, &server_addr, argv[2], argv[3]);

    close(sockfd);
    return 0;
}
