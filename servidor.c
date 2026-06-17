#define _WIN32_WINNT 0x0500
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib") // Linka a biblioteca Winsock

#define PORTA 8080
#define MAX_CLIENTES 10
#define TAMANHO_BUFFER 1024

// Estrutura para passar dados para a thread
typedef struct {
    SOCKET socket_cliente;
    int id_cliente;
} ThreadData;

// Contador de senhas (compartilhado entre threads)
int contador_senha = 0;
HANDLE mutex_senha; // Mutex do Windows

// Função para gerar próxima senha
void gerar_senha(char *senha) {
    WaitForSingleObject(mutex_senha, INFINITE);
    contador_senha++;
    int num = contador_senha;
    ReleaseMutex(mutex_senha);
    
    // Formato: A001, A002, A003...
    sprintf(senha, "A%03d", num);
}

// Função executada por cada thread (atende um cliente)
DWORD WINAPI atender_cliente(LPVOID parametro) {
    ThreadData *dados = (ThreadData*)parametro;
    SOCKET sock = dados->socket_cliente;
    int id = dados->id_cliente;
    free(dados);
    
    char buffer[TAMANHO_BUFFER];
    char senha[10];
    int bytes_recebidos;
    
    printf("[Servidor] Cliente %d conectado!\n", id);
    
    while (1) {
        memset(buffer, 0, TAMANHO_BUFFER);
        bytes_recebidos = recv(sock, buffer, TAMANHO_BUFFER, 0);
        
        if (bytes_recebidos <= 0) {
            printf("[Servidor] Cliente %d desconectou.\n", id);
            break;
        }
        
        buffer[bytes_recebidos] = '\0';
        
        // Remove quebra de linha (Windows usa \r\n)
        buffer[strcspn(buffer, "\r\n")] = '\0';
        printf("[Servidor] Recebido de %d: %s\n", id, buffer);
        
        if (strcmp(buffer, "PEDIR_SENHA") == 0) {
            gerar_senha(senha);
            send(sock, senha, strlen(senha), 0);
            printf("[Servidor] Enviado para %d: %s\n", id, senha);
        } else if (strcmp(buffer, "SAIR") == 0) {
            printf("[Servidor] Cliente %d pediu para sair.\n", id);
            break;
        } else {
            char *msg = "COMANDO_INVALIDO";
            send(sock, msg, strlen(msg), 0);
        }
    }
    
    closesocket(sock);
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET servidor_socket, cliente_socket;
    struct sockaddr_in endereco_servidor, endereco_cliente;
    int tamanho_endereco = sizeof(endereco_cliente);
    int id_cliente = 0;
    HANDLE thread;
    
    // Inicializar Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Erro ao inicializar Winsock.\n");
        return 1;
    }
    
    // Criar mutex para controle da senha
    mutex_senha = CreateMutex(NULL, FALSE, NULL);
    if (mutex_senha == NULL) {
        printf("Erro ao criar mutex.\n");
        WSACleanup();
        return 1;
    }
    
    // Criar socket
    servidor_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor_socket == INVALID_SOCKET) {
        printf("Erro ao criar socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    // Configurar endereço do servidor
    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_addr.s_addr = INADDR_ANY;
    endereco_servidor.sin_port = htons(PORTA);
    
    // Bind (associar socket à porta)
    if (bind(servidor_socket, (struct sockaddr*)&endereco_servidor, sizeof(endereco_servidor)) == SOCKET_ERROR) {
        printf("Erro no bind: %d\n", WSAGetLastError());
        closesocket(servidor_socket);
        WSACleanup();
        return 1;
    }
    
    // Listen (aguardar conexões)
    if (listen(servidor_socket, MAX_CLIENTES) == SOCKET_ERROR) {
        printf("Erro no listen: %d\n", WSAGetLastError());
        closesocket(servidor_socket);
        WSACleanup();
        return 1;
    }
    
    printf("========================================\n");
    printf("  SERVIDOR DE SENHAS - INICIADO\n");
    printf("  Porta: %d\n", PORTA);
    printf("  Aguardando clientes...\n");
    printf("========================================\n\n");
    
    while (1) {
        // Aceitar nova conexão
        cliente_socket = accept(servidor_socket, (struct sockaddr*)&endereco_cliente, &tamanho_endereco);
        if (cliente_socket == INVALID_SOCKET) {
            printf("Erro no accept: %d\n", WSAGetLastError());
            continue;
        }
        
        id_cliente++;
        
        // Preparar dados para a thread
        ThreadData *dados = (ThreadData*)malloc(sizeof(ThreadData));
        dados->socket_cliente = cliente_socket;
        dados->id_cliente = id_cliente;
        
        // Criar thread para atender o cliente
        thread = CreateThread(NULL, 0, atender_cliente, dados, 0, NULL);
        if (thread == NULL) {
            printf("Erro ao criar thread.\n");
            free(dados);
            closesocket(cliente_socket);
        } else {
            CloseHandle(thread); // Libera o handle da thread
        }
    }
    
    closesocket(servidor_socket);
    CloseHandle(mutex_senha);
    WSACleanup();
    return 0;
}