#ifndef UDP_FILE_TRANSFER_H // If UDP_FILE_TRANSFER_H is NOT defined
#define UDP_FILE_TRANSFER_H


#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <stdint.h>     
#include <arpa/inet.h>  // Network byte order conversion functions
#include <unistd.h>     // POSIX API (e.g., close, sleep)

#define PORT 6969

// Define the buffer size for data transmission
//  - 512 bytes for data payload
//  - 4 bytes for TFTP-like headers (Opcode + Block Number)
#define BUFFER_SIZE 516 

// Define the maximum data size per packet
#define DATA_SIZE 512

// Define timeout duration in seconds for retransmissions
#define TIMEOUT 3 

#define OP_DATA 3

// Define operation codes for different types of file transfer requests
#define OP_RRQ 1    // Read Request (Client requests a file from the server)
#define OP_WRQ 2    // Write Request (Client sends a file to the server)
#define OP_ACK 3    // Acknowledgment (Server acknowledges receipt of a packet)
#define OP_ERROR 4  // Error (Server or client signals an error condition)
#define OP_DELETE 5 // Delete Request (Client requests deletion of a file)

// Structure definition for a data packet used in file transfers
typedef struct {
    uint16_t opcode;       // Operation code (OP_RRQ, OP_WRQ, OP_ACK)
    uint16_t block_number; // Block number (used for sequencing packets)
    char data[DATA_SIZE];  // Data payload (actual file contents)
} Packet;

// Function prototype for handling critical errors
// This function prints an error message and terminates the program
void die(const char *msg);

#endif
