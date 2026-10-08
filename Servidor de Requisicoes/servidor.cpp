#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "texto_em_palavras.hpp"

class Tarefa
{

public:
    std::string caminho_arquivo;
    std::string palavra;
};

//cria a fila que vai conter o caminho e a palavra do cliente
std::queue<Tarefa> fila;
//é o controlador, que não vai deixar por exemplo duas thread mexer na fila ao mesmo tempo
std::mutex mutex_fila;

void trabalhador()
{
    mutex_fila.lock();

    Tarefa tarefa_atual = fila.front();
    fila.pop();

    mutex_fila.unlock();

    int resultado = contarPalavra(tarefa_atual.caminho_arquivo,tarefa_atual.palavra);

    std::cout << "Arquivo: " << tarefa_atual.caminho_arquivo << "\n";
    std::cout << "Palavra: " << tarefa_atual.palavra << "\n";
    std::cout << "Quantidade encontrada: " << resultado << "\n";
}

int main()
{
    // 1. Criar socket
    int servidor_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (servidor_socket == -1)
    {
        std::cout << "Erro ao criar socket\n";
        return 1;
    }

    std::cout << "Socket criado!\n";
/*

    // 2. Configurar endereço
    sockaddr_in endereco;

    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(8080);


    // 3. Associar socket ao endereço
    if (bind(servidor_socket,(sockaddr*)&endereco,sizeof(endereco)) == -1)
    {
        std::cout << "Erro no bind\n";
        return 1;
    }


    // 4. Esperar conexões
    if (listen(servidor_socket, 5) == -1)
    {
        std::cout << "Erro no listen\n";
        return 1;
    }

    std::cout << "Servidor esperando cliente...\n";


    // 5. Aceitar cliente
    int cliente_socket = accept(
        servidor_socket,
        nullptr,
        nullptr
    );

    if (cliente_socket == -1)
    {
        std::cout << "Erro no accept\n";
        return 1;
    }

    std::cout << "Cliente conectado!\n";
*/


    // Ainda falta recv(), send() e close()


    Tarefa tarefa1;
    tarefa1.caminho_arquivo = "./livros/dom_casmurro.txt";
    tarefa1.palavra = "casa";

    mutex_fila.lock();

    fila.push(tarefa1);

    mutex_fila.unlock();

    std::thread t1(trabalhador);

    t1.join();

    return 0;
}
