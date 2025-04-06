# TCP File Server-Client

This project contains a TCP server and client implemented in C using the Winsock API. The client can request text files from the server, which are stored in a designated "Files" directory. The system supports both persistent and non-persistent connections.

## Files

- `TCP_BlockingServer.c`: A blocking TCP server that handles file requests from clients.
- `TCP_Client.c`: A client program that connects to the server and requests files.
- `Files/`: Directory from which the server reads `.txt` files to send to clients.

## Features

- Menu-driven client interaction
- RTT measurement and display
- Persistent (`keep-alive`) or non-persistent connections
- Graceful error handling

## How to Run

1. **Windows only** (uses Winsock API).
2. Compile both `TCP_Client.c` and `TCP_BlockingServer.c` using a C compiler (e.g., MinGW).
3. Start the server first.
4. Run the client and follow the interactive prompts.

## Example Usage

```sh
> ./TCP_BlockingServer.exe
> ./TCP_Client.exe
