#include <winsock2.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

const int TIME_PORT = 27015;

// Reads file from files folder
char* readFile(char* filename) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "Files\\%s.txt", filename);


    FILE* f = fopen(filepath, "rt");
    if (!f) {
        perror("Error opening file");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        perror("Error allocating memory");
        fclose(f);
        return NULL;
    }
    fread(buffer, 1, length, f);
    buffer[length] = '\0';
    fclose(f);
    return buffer;
}


///////////////////////////////////////////////
void main() {

    //printf("v6\n");

    WSADATA wsaData;
    SOCKET listenSocket;
    LARGE_INTEGER frequency, start, end;
    double interval;
    QueryPerformanceFrequency(&frequency);
    int packetCounter = 0;

    // Initialize Winsock
    if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        printf("Time Server: Error at WSAStartup()\n");
        return;
    }

    // Create a socket for listening
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == listenSocket) {
        printf("Time Server: Error at socket(): %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    // Server address structure
    struct sockaddr_in serverService;
    memset(&serverService, 0, sizeof(serverService));
    serverService.sin_family = AF_INET;
    serverService.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections on any IP address
    serverService.sin_port = htons(TIME_PORT);

    // Bind the socket
    if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *)&serverService, sizeof(serverService))) {
        printf("Time Server: Error at bind(): %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    // Listen for incoming connections
    if (SOCKET_ERROR == listen(listenSocket, 5)) {
        printf("Time Server: Error at listen(): %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    printf("Time Server: Waiting for client connections.\n");

    // Main server loop to accept and handle client connections
    while (1) {
        struct sockaddr_in from;
        int fromLen = sizeof(from);

        // Accept a client connection
        SOCKET msgSocket = accept(listenSocket, (struct sockaddr *)&from, &fromLen);
        if (INVALID_SOCKET == msgSocket) {
            printf("Time Server: Error at accept(): %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return;
        }

        printf("\nTime Server: Client connected.\n");

        // Handle client requests in a loop (for persistent connections)
        bool keepConnection = true;
        while (keepConnection) {
            printf("\n");
            char recvBuff[255] = {0};

            // Receive data from client
            int bytesRecv = recv(msgSocket, recvBuff, sizeof(recvBuff) - 1, 0);
            if (bytesRecv == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAECONNRESET) {
                    printf("Time Server: Connection reset by peer.\n");
                } else {
                    printf("Time Server: recv() failed with error code %d\n", err);
                }
                closesocket(msgSocket);
                break;
            }

            if (bytesRecv == 0) {
                printf("Time Server: Connection closed by client.\n");
                closesocket(msgSocket);
                break;
            }

            recvBuff[bytesRecv] = '\0'; // Null-terminate the received data

            printf("Time Server: Received data: %s\n", recvBuff);
            QueryPerformanceCounter(&start); // start timer for handling
            packetCounter++;

            // Parse the received buffer
            char persistenceFlag = recvBuff[0]; // 't' or 'f'
            char* fileName = recvBuff + 2;      // Pointer to the filename
            int packetNumber = (int)(recvBuff[1]); // Number of packets sent + 33

            printf("Packet Number: %d, Total packets recived: %d Persistence Flag: %c, Filename: %s\n",
                   packetNumber - 33, packetCounter, persistenceFlag, fileName);

            // Check if fileName is empty
            if (strlen(fileName) == 0) {
                // Send an error message if no filename was selected
                const char* errorMsg = "invalid option, please select 1-4";
                int bytesSent = send(msgSocket, errorMsg, (int)strlen(errorMsg), 0);
                if (bytesSent == SOCKET_ERROR) {
                    printf("Time Server: send() failed with error code %d\n", WSAGetLastError());
                    closesocket(msgSocket);
                    break;
                }
            }
            else {
                // Read the requested file
                char* fileContent = readFile(fileName);
                if (fileContent == NULL) {
                    // If file not found or error, send an error message
                    const char* errorMsg = "Error reading file or file not found.";
                    int bytesSent = send(msgSocket, errorMsg, (int)strlen(errorMsg), 0);
                    if (bytesSent == SOCKET_ERROR) {
                        printf("Time Server: send() failed with error code %d\n", WSAGetLastError());
                        closesocket(msgSocket);
                        break;
                    }
                } else {
                    // Send the file content to the client
                    int bytesSent = send(msgSocket, fileContent, (int)strlen(fileContent), 0);
                    if (bytesSent == SOCKET_ERROR) {
                        printf("Time Server: send() failed with error code %d\n", WSAGetLastError());
                        free(fileContent);
                        closesocket(msgSocket);
                        break;
                    }
                    free(fileContent);
                }
            }

            // Determine whether to keep the connection open
            if (persistenceFlag == 't') {
                // Persistent connection, keep the connection open
                printf("Time Server: Keeping persistent connection open.\n");
            }
            else if (persistenceFlag == 'f') {
                // Non-persistent connection, close the connection
                printf("Time Server: Closing non-persistent connection.\n");
                closesocket(msgSocket);
                keepConnection = false;
            }
            else {
                // Invalid persistence flag
                printf("Time Server: Invalid persistence flag received. Closing connection.\n");
                closesocket(msgSocket);
                keepConnection = false;
            }

            QueryPerformanceCounter(&end);
            interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
            printf("\nRequest process time: %lf ms\n", interval);

        } // End of client handling loop
    } // End of server main loop

    closesocket(listenSocket);
    WSACleanup();
}
