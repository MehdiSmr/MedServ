#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>

#include "utils/utils.h"

#define PORT 8989

const int detectConsoleLines() {
    winsize w{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) {
        return static_cast<int>(w.ws_row / 2);
    }
    return 20;
}

namespace Settings {
    const int g_consolelines(detectConsoleLines());
}

struct AppState {
    std::vector<std::string> chats;
    std::vector<std::string> messages;
    std::string currentChat;
    std::string status;
    bool inChat{false};
};

struct UiSnapshot {
    std::vector<std::string> chats;
    std::vector<std::string> messages;
    std::string currentChat;
    std::string status;
    bool inChat{false};
};

static bool startsWith(const std::string& s, const std::string& p) {
    return s.rfind(p, 0) == 0;
}

static std::vector<std::string> splitNonEmptyLines(const std::string& text) {
    std::vector<std::string> lines;
    std::stringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

static UiSnapshot makeSnapshot(const AppState& state) {
    return UiSnapshot{state.chats, state.messages, state.currentChat, state.status, state.inChat};
}

static void renderUI(const std::string& username, const UiSnapshot& snap, std::mutex& io_mtx) {
    std::lock_guard<std::mutex> ioLock(io_mtx);

    // Full-page illusion (ANSI clear + home)
    std::cout << "\033[2J\033[H";

    std::cout << "User: " << username << "\n";
    if (snap.inChat) {
        std::cout << "Mode: Chat (" << snap.currentChat << ")\n";
    } else {
        std::cout << "Mode: Lobby\n";
    }
    std::cout << "----------------------------------------\n";

    if (snap.inChat) {
        if (snap.messages.empty()) {
            std::cout << "(no messages)\n";
        } else {
            for (const auto& msg : snap.messages) {
                std::cout << msg << "\n";
            }
        }
        std::cout << "----------------------------------------\n";
        std::cout << "Commands: /leave, /chats, /help, /quit\n";
        std::cout << "(plain text sends message)\n";
    } else {
        if (snap.chats.empty()) {
            std::cout << "No chats available\n";
        } else {
            std::cout << "Available chats:\n";
            for (const auto& c : snap.chats) {
                std::cout << "- " << c << "\n";
            }
        }
        std::cout << "----------------------------------------\n";
        std::cout << "Commands: /chats, /new <chat>, /join <chat>, /help, /quit\n";
    }

    if (!snap.status.empty()) {
        std::cout << "\n[" << snap.status << "]\n";
    }

    std::cout << "> " << std::flush;
}

static bool sendRequest(int client_fd, const std::string& request) {
    return Utils::sendAll(client_fd, request) == 0;
}

static void applyServerResponse(AppState& state, const std::string& response) {
    const auto lines = splitNonEmptyLines(response);
    if (lines.empty()) {
        // server may send just "\n" for empty chat list
        if (!state.inChat) {
            state.chats.clear();
            state.status = "No chats available";
        }
        return;
    }

    auto chatsIt = std::find(lines.begin(), lines.end(), "/chats/");
    if (chatsIt != lines.end()) {
        state.chats.assign(chatsIt + 1, lines.end());
        state.status = state.chats.empty() ? "No chats available" : "Chats updated";
        return;
    }

    auto chatIt = std::find_if(lines.begin(), lines.end(), [](const std::string& l) { return startsWith(l, "/chat/"); });
    if (chatIt != lines.end()) {
        const std::string chatName = chatIt->substr(6);
        std::vector<std::string> payload(chatIt + 1, lines.end());

        if (payload.size() == 1 && payload[0] == "Chat not found") {
            state.inChat = false;
            state.currentChat.clear();
            state.messages.clear();
            state.status = "Chat not found";
            return;
        }

        state.inChat = true;
        state.currentChat = chatName;
        state.messages = std::move(payload);
        state.status = "Opened chat: " + chatName;
        return;
    }

    if (state.inChat) {
        // POST /message responses from your servers are raw chat contents (no /chat/ marker)
        state.messages = lines;
        state.status = "Chat updated";
    } else {
        state.status = lines.front();
    }
}

static void readerLoop(int client_fd,
                       std::atomic<bool>& running,
                       std::mutex& state_mtx,
                       AppState& state,
                       const std::string& username,
                       std::mutex& io_mtx) {
    char buffer[1024];

    while (running.load()) {
        std::memset(buffer, 0, sizeof(buffer));
        if (Utils::receiveAll(client_fd, buffer, sizeof(buffer)) < 0) {
            {
                std::lock_guard<std::mutex> lock(state_mtx);
                state.status = "Disconnected from server";
            }
            UiSnapshot snap;
            {
                std::lock_guard<std::mutex> lock(state_mtx);
                snap = makeSnapshot(state);
            }
            renderUI(username, snap, io_mtx);
            running.store(false);
            break;
        }

        {
            std::lock_guard<std::mutex> lock(state_mtx);
            applyServerResponse(state, std::string(buffer));
        }

        UiSnapshot snap;
        {
            std::lock_guard<std::mutex> lock(state_mtx);
            snap = makeSnapshot(state);
        }
        renderUI(username, snap, io_mtx);
    }
}

int main() {
    int client_fd;
    sockaddr_in serverAddr{};

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error\n";
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address / address not supported\n";
        close(client_fd);
        return -1;
    }

    if (connect(client_fd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed\n";
        close(client_fd);
        return -1;
    }

    std::string username;
    for (int i = 0; i < Settings::g_consolelines; ++i) {
        std::cout << '\n';
    }
    std::cout << "Enter username: ";
    std::cin >> username;

    std::atomic<bool> running{true};
    std::mutex state_mtx;
    std::mutex io_mtx;
    AppState state;

    std::thread reader(readerLoop,
                       client_fd,
                       std::ref(running),
                       std::ref(state_mtx),
                       std::ref(state),
                       std::cref(username),
                       std::ref(io_mtx));

    // initial chat fetch
    if (!sendRequest(client_fd, "GET /chats")) {
        running.store(false);
    }

    while (running.load()) {
        std::string input;
        if (!std::getline(std::cin >> std::ws, input)) {
            running.store(false);
            break;
        }

        if (input == "/quit") {
            running.store(false);
            break;
        }

        if (input == "/help") {
            UiSnapshot snap;
            {
                std::lock_guard<std::mutex> lock(state_mtx);
                state.status = "Help shown";
                snap = makeSnapshot(state);
            }
            renderUI(username, snap, io_mtx);
            continue;
        }

        if (input == "/chats") {
            if (!sendRequest(client_fd, "GET /chats")) {
                running.store(false);
                break;
            }
            continue;
        }

        if (startsWith(input, "/new ")) {
            const std::string chatName = input.substr(5);
            if (chatName.empty()) {
                UiSnapshot snap;
                {
                    std::lock_guard<std::mutex> lock(state_mtx);
                    state.status = "Chat name cannot be empty";
                    snap = makeSnapshot(state);
                }
                renderUI(username, snap, io_mtx);
                continue;
            }

            if (!sendRequest(client_fd, "POST /chat/" + chatName)) {
                running.store(false);
                break;
            }
            // refresh lobby list right after create
            if (!sendRequest(client_fd, "GET /chats")) {
                running.store(false);
                break;
            }
            continue;
        }

        if (startsWith(input, "/join ")) {
            const std::string chatName = input.substr(6);
            if (chatName.empty()) {
                UiSnapshot snap;
                {
                    std::lock_guard<std::mutex> lock(state_mtx);
                    state.status = "Chat name cannot be empty";
                    snap = makeSnapshot(state);
                }
                renderUI(username, snap, io_mtx);
                continue;
            }

            if (!sendRequest(client_fd, "GET /chat/" + chatName)) {
                running.store(false);
                break;
            }
            continue;
        }

        if (input == "/leave") {
            UiSnapshot snap;
            {
                std::lock_guard<std::mutex> lock(state_mtx);
                state.inChat = false;
                state.currentChat.clear();
                state.messages.clear();
                state.status = "Left chat";
                snap = makeSnapshot(state);
            }
            renderUI(username, snap, io_mtx);
            continue;
        }

        std::string chatName;
        {
            std::lock_guard<std::mutex> lock(state_mtx);
            if (!state.inChat || state.currentChat.empty()) {
                UiSnapshot snap = makeSnapshot(state);
                snap.status = "Not in chat. Use /join <chat>";
                renderUI(username, snap, io_mtx);
                continue;
            }
            chatName = state.currentChat;
        }

        const std::string req = "POST /message/" + chatName + "/username:" + username + "{" + input + "}";
        if (!sendRequest(client_fd, req)) {
            running.store(false);
            break;
        }
    }

    running.store(false);
    shutdown(client_fd, SHUT_RDWR);

    if (reader.joinable()) {
        reader.join();
    }

    close(client_fd);
    return 0;
}
