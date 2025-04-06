#include <winsock.h>

#include <stdio.h>

#include <string.h>

#include <time.h>

#include <stdlib.h>

#include <stdbool.h>

#include <assert.h>

const int TIME_PORT = 27015;

// Get filename from user
char * getFileName() {
    char * fileName = (char * ) malloc(255);
    char invalidChar;
    if (!fileName) {
        perror("Error allocating memory");
        exit(1);
    }

    // Get input and check proper input
    while (1) { // Loop until valid input is received
        printf("\nEnter file name: ");
        if (scanf("%254s", fileName) != 1) {
            fprintf(stderr, "Error: Failed to read file name.\n");
            continue; // Retry
        }

        // Check if input is empty
        if (strlen(fileName) == 0) {
            fprintf(stderr, "Error: File name cannot be empty.\n");
            continue; // Retry
        }

        // Check for invalid characters in the file name
        int isValid = 1; // Flag to track validity
        for (size_t i = 0; fileName[i] != '\0'; i++) {
            if (fileName[i] == '/' || fileName[i] == '\\' || fileName[i] == ':' ||
                fileName[i] == '*' || fileName[i] == '?' || fileName[i] == '"' ||
                fileName[i] == '<' || fileName[i] == '>' || fileName[i] == '|') {
                fprintf(stderr, "Error: File name contains invalid characters.\n");
                isValid = 0; // Invalid input
                break;
            }
        }

        if (isValid) {
            break; // Valid file name entered
        }
    }


  return fileName;
}

// Check for errors in send/receive functions
bool checkForAnError(int bytesResult, char * ErrorAt, SOCKET socket) {
  if (SOCKET_ERROR == bytesResult) {
    printf("Time Client: Error at %s(): %d\n", ErrorAt, WSAGetLastError());
    closesocket(socket);
    WSACleanup();
    return true;
  }
  return false;
}

// Create and connect a socket
SOCKET createSocket() {
  SOCKET connSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (INVALID_SOCKET == connSocket) {
    printf("Time Client: Error at socket(): %d\n", WSAGetLastError());
    WSACleanup();
    exit(1);
  }

  struct sockaddr_in server;
  memset( & server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_port = htons(TIME_PORT);

  if (SOCKET_ERROR == connect(connSocket, (SOCKADDR * ) & server, sizeof(server))) {
    printf("Time Client: Error at connect(): %d\n", WSAGetLastError());
    closesocket(connSocket);
    WSACleanup();
    exit(1);
  }

  printf("\nConnection established successfully.\n");
  return connSocket;
}

// Choose connection type: Persistent or Non-Persistent
bool choosePersistence() {
    char connectionType;
    printf("Select connection type:\n");
    printf("1: Persistent\n");
    printf("2: Non-Persistent\n");
    printf("Your choice: ");

    while (1) {
        // Check if input is successfully read
        if (scanf(" %c", &connectionType) != 1) {
            fprintf(stderr, "Error reading input. Please try again.\n");
            // Clear input buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        } else {
            // Validate input
            if (connectionType == '1') {
                return true;
            } else if (connectionType == '2') {
                return false;
            } else {
                printf("Invalid choice. Please enter '1' or '2'.\n");
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
            }
        }
    }
}

// Read a file's content into a string
char * readFile(char * filename) {
  FILE * f = fopen(filename, "rt");
  if (!f) {
    perror("Error opening file");
    exit(1);
  }
  fseek(f, 0, SEEK_END);
  long length = ftell(f);
  fseek(f, 0, SEEK_SET);
  char * buffer = (char * ) malloc(length + 1);
  if (!buffer) {
    perror("Error allocating memory");
    fclose(f);
    exit(1);
  }
  fread(buffer, 1, length, f);
  buffer[length] = '\0';
  fclose(f);
  return buffer;
}

// Request handling menu
int requestHandling() {
    int option;
    while (1) {
        printf("\nPlease choose an option:\n");
        printf("1: Read from a file\n");
        printf("2: Read from a file N times\n");
        printf("3: Show RTT to all packages\n");
        printf("4: Exit\n");
        printf("Your choice: ");

        // Check if input is successfully read
        if (scanf(" %d", &option) != 1) {
            fprintf(stderr, "Error reading input. Please try again.\n");
            // Clear input buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);

        } else {
            // Validate input
            if (option >= 1 && option <= 4) {
                if (option == 4) {
                    printf("Goodbye!\n");
                }
                return option;
            } else {
                printf("Invalid choice. Please choose an option between 1 and 4.\n");
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
            }
        }
    }
}

//Print array up to n
void printArray(double * arr, int n) {
    int i = 0;
    double sum = 0;
    printf("You have sent %d packets\n\n", n);
    printf("Index    RTT (ms)\n");
    printf("-----    ---------\n");
    for (i; i < n; i++) {
        printf("%5d    %lf ms\n", i, arr[i]);
        sum = sum + arr[i];
    }
    double avg = sum / n;
    printf("\nAverage RTT per packet: %lf ms\n", avg);
}


///////////////////////////////////////////////
void main() {
  // Initialize Winsock

  //printf("v6\n");

  WSADATA wsaData;
  if (NO_ERROR != WSAStartup(MAKEWORD(2, 0), & wsaData)) {
    printf("Time Client: Error at WSAStartup()\n");
    return;
  }

  int bytesSent = 0, bytesRecv = 0, option = -1, numberSends = -1;
  bool isSocketOpen = false, isPersistent;
  double packetsRTT[31] = {0};
  int lastPacketSentNumber = 0;
  char * sendBuff = NULL;
  char * fileName = NULL;
  LARGE_INTEGER frequency, start, end;
  char recvBuff[255] = {0};

  // Query frequency for timing
  QueryPerformanceFrequency( & frequency);

  SOCKET connSocket;

  isPersistent = choosePersistence();

  while (option != 4) {

    option = requestHandling();

    if (option == 1) {
        numberSends = 1;
        fileName = getFileName();

    } else if (option == 2) {
        while (1) {
        printf("\nchoose N: ");

        // Check if input is successfully read
        if (scanf("%d", &numberSends) != 1) {
            fprintf(stderr, "Error reading input. Please try again.\n");
            // Clear input buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        } else {
            // Validate input
            if (numberSends >= 0 && numberSends <= 30) {
                break;
            } else {
                printf("Invalid choice. Please choose a number between 0 and 30.\n");
        }
    }
}
fileName = getFileName();

    } else if (option == 3) {
      printArray(packetsRTT, lastPacketSentNumber);
      numberSends = 0;

    } else if (option == 4){
        if (fileName){
        free(fileName);
        exit(1);
      }

    } else {
      numberSends = 1;
      fileName = "";
    }

    // Prepare send buffer
    if (fileName) {
      sendBuff = (char * ) malloc(strlen(fileName) + 2);
      if (!sendBuff) {
        perror("Error allocating memory");
        free(fileName);
        closesocket(connSocket);
        WSACleanup();
        exit(1);
      }
    }

    // Send and receive loop
    while (numberSends > 0) {

    //OPEN SOCKET
    if (isSocketOpen == false) {
      connSocket = createSocket();
      isSocketOpen = true;
    }

      sprintf(sendBuff, "%c%c%s", isPersistent ? 't' : 'f', (char)(lastPacketSentNumber + 33), fileName);
      numberSends--;

      memset(recvBuff, 0, sizeof(recvBuff));
      bytesRecv = -1;

      bytesSent = send(connSocket, sendBuff, (int) strlen(sendBuff), 0);
      if (checkForAnError(bytesSent, "send", connSocket)) {
        free(fileName);
        exit(1);
      }

      double interval = 0.0;
      QueryPerformanceCounter( & start);

      while (bytesRecv == -1 && interval < 0.2) {
        QueryPerformanceCounter( & end);
        interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
        bytesRecv = recv(connSocket, recvBuff, sizeof(recvBuff) - 1, 0);
        if (bytesRecv == SOCKET_ERROR) {
          printf("Time Client: Error at recv(): %d\n", WSAGetLastError());
          free(fileName);
          closesocket(connSocket);
          WSACleanup();
          exit(1);
        } else if (bytesRecv == 0) {
          printf("Connection closed by server.\n");
          closesocket(connSocket);
          isSocketOpen = false;
          break;
        }
      }
      QueryPerformanceCounter( & end);
      interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
      packetsRTT[lastPacketSentNumber++] = interval;

      printf("\nReceived from server packet %d: \n%s\n",lastPacketSentNumber , recvBuff);
      if (!isPersistent && isSocketOpen) {
          closesocket(connSocket);
          isSocketOpen = false;
      }
    }


    if (sendBuff) {
      free(sendBuff);
      sendBuff = NULL;
    }

    if (fileName != NULL && fileName[0] != '\0') {
        free(fileName);
        fileName = NULL;

    }
    memset(recvBuff, 0, sizeof(recvBuff));


      if (!isPersistent && isSocketOpen) {
          closesocket(connSocket);
          isSocketOpen = false;


      if (NO_ERROR != WSAStartup(MAKEWORD(2, 0), & wsaData)) {
        printf("Time Client: Error at WSAStartup()\n");
        exit(1);
      }
    }
  }

  closesocket(connSocket);
  WSACleanup();
}
