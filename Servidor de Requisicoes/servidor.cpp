#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "texto_em_palavras.hpp"

class Tarefa
{
public:
    int cliente_socket;        // MUDOU: no lugar de caminho_arquivo
    std::string palavra;
};

std::queue<Tarefa> fila;
std::mutex mutex_fila;
std::condition_variable cv_fila;   // MUDOU: novo, faz a thread dormir com a fila vazia

std::vector<std::string> livros = {
    "./livros/dom_casmurro.txt",
    "./livros/iracema.txt",
    "./livros/memorias_braz_cubas.txt",
    "./livros/o_cortico.txt",
    "./livros/quincas_borba.txt"
};

std::vector<std::thread> threads;

// MUDOU: o trabalhador inteiro foi reescrito (void, nunca retorna)
void trabalhador()
{
    while (true)
    {
        Tarefa tarefa_atual;

        // 1. Retirar a tarefa da fila (só essa parte fica travada)
        {
            std::unique_lock<std::mutex> trava(mutex_fila);
            cv_fila.wait(trava, [] { return !fila.empty(); });  // dorme até ter tarefa
            tarefa_atual = fila.front();
            fila.pop();
        }   // destrava aqui, antes da parte lenta

        // 2. Contar a palavra nos 5 livros (fora do lock, em paralelo)
        std::string resposta;
        for (const std::string& caminho : livros)
        {
            int qtd = contarPalavra(caminho, tarefa_atual.palavra);

            resposta += caminho;
            resposta += ": ";
            if (qtd == -1)
                resposta += "erro ao abrir o arquivo";
            else
                resposta += std::to_string(qtd);
            resposta += "\n";
        }

        // 3. Responder e fechar
        ssize_t enviado = send(tarefa_atual.cliente_socket, resposta.c_str(),
                               resposta.size(), MSG_NOSIGNAL);
        if (enviado < 0)
        {
            std::cout << "Erro ao enviar a resposta\n";
        }
        close(tarefa_atual.cliente_socket);
    }
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

    // MUDOU: permite reiniciar o servidor sem "Address already in use"
    int opt = 1;
    setsockopt(servidor_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    std::cout << "Socket criado!\n";

    // 2. Configurar endereço
    sockaddr_in endereco{};   // MUDOU: {} zera a memória
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(9090);

    // 3. Associar socket ao endereço
    if (bind(servidor_socket, (sockaddr*)&endereco, sizeof(endereco)) == -1)
    {
        std::cout << "Erro no bind\n";
        return 1;
    }

    // 4. Esperar conexões
    if (listen(servidor_socket, SOMAXCONN) == -1)   // MUDOU: 5 virou SOMAXCONN
    {
        std::cout << "Erro no listen\n";
        return 1;
    }

    std::cout << "Servidor esperando cliente...\n";

    // Pool de threads (criado uma vez, antes do loop)
    for (int i = 0; i < 3; i++)
    {
        threads.push_back(std::thread(trabalhador));
    }

    while (true)
    {
        // 5. Aceitar cliente
        int cliente_socket = accept(servidor_socket, nullptr, nullptr);

        if (cliente_socket == -1)
        {
            std::cout << "Erro no accept\n";
            continue;
        }

        std::cout << "Cliente conectado!\n";

        // 6. Receber a palavra
        char buffer[1024];
        ssize_t n = recv(cliente_socket, buffer, sizeof(buffer), 0);

        if (n < 0)
        {
            std::cout << "Erro no recebimento da palavra\n";
            close(cliente_socket);
            continue;
        }
        if (n == 0)
        {
            std::cout << "O cliente desligou a conexão\n";
            close(cliente_socket);
            continue;
        }

        // MUDOU: sem o "if (n > 0)", pois aqui n já é sempre > 0
        std::string palavra(buffer, n);

        while (!palavra.empty() && (palavra.back() == '\n' || palavra.back() == '\r'))
        {
            palavra.pop_back();
        }

        if (palavra.empty())
        {
            std::cout << "Palavra vazia, ignorando\n";
            close(cliente_socket);
            continue;
        }

        std::cout << "Palavra recebida: [" << palavra << "]\n";   // MUDOU: aspas corrigidas

        // MUDOU: UM push só (não mais 5), protegido pelo mutex
        {
            std::lock_guard<std::mutex> trava(mutex_fila);
            fila.push({cliente_socket, palavra});
        }
        cv_fila.notify_one();   // MUDOU: acorda uma thread dormindo

        // A main NÃO dá close: quem fecha é o trabalhador
    }

    return 0;
}
