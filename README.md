# C++ Socket Chat

## Video Demo
https://github.com/user-attachments/assets/ec9fb406-a52c-4f62-b411-20f695d503a0

## Project Overview
This is a learning project for socket programming in C++.

References used:
- *Beej’s Guide to Network Programming*
- https://www.learncpp.com/

The project is a simple chat application where users can:
- connect to a server,
- create chats,
- join existing chats,
- and exchange messages in real time.

## Server Architectures
I implemented two server approaches:

1. **Thread-pool server** (`servert`)
   - Uses a thread pool to process client tasks concurrently.

2. **`poll()` event-driven server** (`serverp`)
   - Uses `poll()` to handle multiple clients in a single event loop.
   - Better suited for scalable I/O handling.
   - Supports broadcasting updates (for example, chat list updates and message updates).

## Client Architecture
The client is multithreaded:
- one thread reads user input,
- one thread listens for server updates.

This allows the client to keep chat state updated in real time while the user is typing.

## Protocol (Current Format)
Communication uses a simple custom TCP protocol framed by a 4-byte length header (handled by `Utils::sendAll` / `Utils::receiveAll`).

### Client Requests
- List chats:
  - `GET /chats`
- Get one chat (messages):
  - `GET /chat/<chat_name>`
- Create chat:
  - `POST /chat/<chat_name>`
- Send message:
  - `POST /message/<chat_name>/username:<username>{<message>}`

### Server Responses
Common response patterns:
- Chat list:
  - `/chats/` followed by one chat name per line
- Single chat content:
  - `/chat/<chat_name>` followed by one message per line
- Errors / status:
  - `Chat not found`
  - `Chat already exists`
  - `Invalid request`
  - `OK`

### Notes
- The protocol is intentionally lightweight for learning purposes.
- Request parsing currently expects valid formats (especially for `POST /message`).

## Build and Run
### 1) Build
```bash
make
```

### 2) Run one server (choose one)
Thread-pool server:
```bash
./servert
```

`poll()` server:
```bash
./serverp
```

### 3) Run clients (in one or more other terminals)
```bash
./client
```

You can run multiple client instances to simulate multiple connected users.
