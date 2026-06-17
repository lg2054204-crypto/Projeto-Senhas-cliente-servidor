#define _WIN32_WINNT 0x0500
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORTA 8080
#define TAMANHO_BUFFER 1024

int main() {
    WSADATA wsaData;
    SOCKET cliente_socket;
    struct sockaddr_in endereco_servidor;
    char buffer[TAMANHO_BUFFER];
    char comando[TAMANHO_BUFFER];
    int bytes_recebidos;
    
    // Inicializar Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Erro ao inicializar Winsock.\n");
        return 1;
    }
    
    // Criar socket
    cliente_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cliente_socket == INVALID_SOCKET) {
        printf("Erro ao criar socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    // Configurar endereço do servidor
    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_port = htons(PORTA);
    endereco_servidor.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Conectar ao servidor
    if (connect(cliente_socket, (struct sockaddr*)&endereco_servidor, sizeof(endereco_servidor)) == SOCKET_ERROR) {
        printf("Erro ao conectar: %d\n", WSAGetLastError());
        closesocket(cliente_socket);
        WSACleanup();
        return 1;
    }
    
    printf("========================================\n");
    printf("  CLIENTE DE SENHAS - CONECTADO\n");
    printf("  Servidor: 127.0.0.1:%d\n", PORTA);
    printf("========================================\n");
    printf("\nComandos disponiveis:\n");
    printf("  PEDIR_SENHA  - Solicita uma senha\n");
    printf("  SAIR         - Desconecta\n\n");
    
    while (1) {
        printf("> ");
        fgets(comando, TAMANHO_BUFFER, stdin);
        
        // Remove quebra de linha (Windows usa \r\n)
        comando[strcspn(comando, "\r\n")] = '\0';
        
        // Enviar comando ao servidor
        send(cliente_socket, comando, strlen(comando), 0);
        
        if (strcmp(comando, "SAIR") == 0) {
            printf("Desconectando...\n");
            break;
        }
        
        // Receber resposta do servidor
        memset(buffer, 0, TAMANHO_BUFFER);
        bytes_recebidos = recv(cliente_socket, buffer, TAMANHO_BUFFER, 0);
        
        if (bytes_recebidos <= 0) {
            printf("Servidor desconectado.\n");
            break;
        }
        
        buffer[bytes_recebidos] = '\0';
        printf("Resposta: %s\n", buffer);
    }
    
    closesocket(cliente_socket);
    WSACleanup();
    return 0;
}