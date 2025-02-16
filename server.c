#include "udp_file_transfer.h"

#define SERVER_FILES_PATH "/home/din/files_transfer/server_files/" // Define server storage path

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void handle_request(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len) {
    char buffer[BUFFER_SIZE];
    char filename[256];
    char full_path[512]; // Full file path
    FILE *file;
    Packet packet, ack;
    ssize_t n;
    uint16_t block = 0;

    while (1) {
        n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)client_addr, &client_len);
        if (n < 0) die("Failed to receive data");

        memcpy(&packet, buffer, sizeof(packet));
        uint16_t opcode = ntohs(packet.opcode);

        switch (opcode) {
            case OP_RRQ: // Read Request (Download)
                snprintf(full_path, sizeof(full_path), "%s%.200s", SERVER_FILES_PATH, packet.data);
                file = fopen(full_path, "rb");
                if (!file) {
                    fprintf(stderr, "File not found: %s\n", full_path);
                    continue;
                }
                while (1) {
                    memset(&packet, 0, sizeof(packet));
                    packet.opcode = htons(OP_DATA);
                    packet.block_number = htons(++block);
                    size_t bytes_read = fread(packet.data, 1, DATA_SIZE, file);

                    sendto(sockfd, &packet, 4 + bytes_read, 0, (struct sockaddr *)client_addr, client_len);

                    if (bytes_read < DATA_SIZE) break; // End of file reached
                    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL); // Wait for acknowledgment
                }
                fclose(file);
                break;

            case OP_WRQ: // Write Request (Upload)
                snprintf(full_path, sizeof(full_path), "%s%.200s", SERVER_FILES_PATH, packet.data);
                file = fopen(full_path, "wb");
                if (!file) {
                    fprintf(stderr, "Failed to create file: %s\n", full_path);
                    continue;
                }
                while (1) {
                    ack.opcode = htons(OP_ACK);
                    ack.block_number = htons(block);
                    sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)client_addr, client_len);

                    n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)client_addr, &client_len);
                    if (n < 0) break;
                    
                    memcpy(&packet, buffer, sizeof(packet));
                    block = ntohs(packet.block_number);

                    fwrite(packet.data, 1, n - 4, file);
                    if (n - 4 < DATA_SIZE) break;
                }
                fclose(file);
                break;

            case OP_DELETE: // Delete Request
                snprintf(full_path, sizeof(full_path), "%s%.200s", SERVER_FILES_PATH, packet.data);
                if (remove(full_path) == 0) {
                    printf("File deleted: %s\n", full_path);
                    packet.opcode = htons(OP_ACK);
                    packet.block_number = htons(0);
                    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)client_addr, client_len);
                } else {
                    perror("Error deleting file");
                    packet.opcode = htons(OP_ERROR);
                    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)client_addr, client_len);
                }
                break;

            default:
                break;
        }
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) die("Failed to create socket");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        die("Failed to bind socket");

    printf("Server listening on port %d\n", PORT);

    while (1) {
        handle_request(sockfd, &client_addr, client_len);
    }

    close(sockfd);
    return 0;
}
