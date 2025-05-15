#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <mysql.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024
using namespace std;

MYSQL* connect_db() {
    MYSQL* conn = mysql_init(nullptr);

    if (!conn) {
        cerr << "Erro ao iniciar conexão com mysql. \n";
        return nullptr;
    }

    if (!mysql_real_connect(conn, "localhost", "root", "eltinho123", "bd_http", 3306, nullptr, 0)){
        cerr << "Erro ao conectar: " << mysql_error(conn) << "\n";
        return nullptr;
    }

    return conn;
}

void inserir_nome(const string& nome) {
    MYSQL* conn = connect_db();

    string query = "INSERT INTO nomes (nome) VALUES ('" + nome + "')";

        if (mysql_query(conn, query.c_str())) {
            cerr << "Erro ao inserir";
        }

        mysql_close(conn);
}

void handle_client(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    //Receber dados:
    int bytesRecebidos = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
    if (bytesRecebidos <= 0) {
        closesocket(clientSocket);
        return;
    }

    string request(buffer);
    cout << "Requisição recebida: \n" << request << endl;

    // Verifica se é POST /inserir
    if (request.find("POST /inserir") == 0) {
        size_t bodyStart = request.find("\r\n\r\n");
        if (bodyStart != string::npos) {
            string body = request.substr(bodyStart + 4);  
            cout << "Corpo recebido: " << body << endl;
            inserir_nome(body);
            string ok = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
            send(clientSocket, ok.c_str(), ok.size(), 0);
        }
        else {
            cerr << "Não foi possível localizar o corpo da requisição.\n";
        }
    }
    else {
        // Resposta padrão
        string response = "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "Hello world";
        send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
    }

    closesocket(clientSocket);
}

int main()
{
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr {}, clientAddr{};
    int clientSize = sizeof(clientAddr);

    //Inicializar WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Erro ao inicializar WinSock.\n";
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == INVALID_SOCKET) {
        cerr << "Erro ao criar o socket. \n";
        WSACleanup();
        return 1;
    }

    // Configurar endereço

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    //Associar socket à porta
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Erro no bind.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    //Começar a escutar conexões
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Erro no listen.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Servidor HTTP escutando na porta: " << PORT << "...\n";

    while (true) {
        clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Erro no accept.\n";
            continue;
        }

        handle_client(clientSocket); // (versão sequencial simples)
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

