#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int BUFFER_SIZE = 4096;
const int PORT = 8080;
const string ROOT_DIR = "./www";    // Directory from which to serve static files

string handle_request(const string& request) {
    // Use string stream to split the request line by whitespace
    istringstream request_stream(request);
    string method, path, version;
    request_stream >> method >> path >> version;

    if (method != "GET") {
        ostringstream response;
        response << "HTTP/1.1 405 Method Not Allowed" << endl << endl;
        return response.str();
    }

    if (path == "/") {
        path = "/index.html";
    }

    string file_path = ROOT_DIR + path;
    ifstream file(file_path);

    // If file doesn't exist, return 404 Not Found
    if (!file.is_open()) {
        ostringstream response;
        response << "HTTP/1.1 404 Not Found" << endl << endl;
        response << "Page not found";
        return response.str();
    }

    // Read entire file content into a string
    stringstream file_content;
    file_content << file.rdbuf();
    string body = file_content.str();
    file.close();

    ostringstream response;
    response << "HTTP/1.1 200 OK" << endl
        << "Content-Type: text/html" << endl
        << "Content-Length: " << body.size() << endl << endl
        << body;
    return response.str();
}

// Handler for a single client in its own thread
void client_handler(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    // Block until data is received
    int val_read = recv(client_socket, buffer, sizeof(buffer), 0);
    if (val_read > 0) {
        string request(buffer, val_read);
        string response = handle_request(request);
        send(client_socket, response.c_str(), (int)response.size(), 0);
    }
    closesocket(client_socket);
}

int main() {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        cerr << "WSAStartup failed!" << endl;
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        cerr << "Socket creation failed!" << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the specified IP and port
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Bind failed!" << endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Start listening for incoming connections
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed!" << endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    cout << "Server is running on http://localhost:" << PORT << endl;

    // Main loop: accept and handle incoming connections
    while (true) {
        sockaddr_in client_addr{};
        int client_addr_len = sizeof(client_addr);
        // Accept blocks until a new client connects
        SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET) {
            cerr << "Accept failed: " << WSAGetLastError() << endl;
            continue;
        }

        // Launch a new thread to handle the client independently
        thread(client_handler, client_socket).detach();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
