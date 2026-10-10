
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

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

struct Tarefa
{
    std::string caminho_arquivo;
    std::string palavra;
};

struct Resultado
{
    std::string caminho_arquivo;
    int quantidade;
};

std::queue<Tarefa> fila;
std::mutex mutex_fila;
std::condition_variable condicao_fila;

std::mutex mutex_resultados;
std::condition_variable condicao_resultados;

std::vector<Resultado> resultados;
int tarefas_pendentes = 0;

const std::vector<std::string> livros = {
    "./livros/dom_casmurro.txt",
    "./livros/memorias_braz_cubas.txt",
    "./livros/quincas_borba.txt",
    "./livros/o_cortico.txt",
    "./livros/iracema.txt"
};

void trabalhador()
{
    while (true)
    {
        Tarefa tarefa_atual;

        {
            std::unique_lock<std::mutex> trava(mutex_fila);

            condicao_fila.wait(trava, [] {
                return !fila.empty();
            });

            tarefa_atual = fila.front();
            fila.pop();
        }

        int quantidade = contarPalavra(
            tarefa_atual.caminho_arquivo,
            tarefa_atual.palavra
        );

        std::cout << "Tarefa concluida: "
                  << tarefa_atual.caminho_arquivo
                  << " -> " << quantidade << '\n';

        {
            std::lock_guard<std::mutex> trava(mutex_resultados);

            resultados.push_back({
                tarefa_atual.caminho_arquivo,
                quantidade
            });

            --tarefas_pendentes;

            if (tarefas_pendentes == 0)
                condicao_resultados.notify_one();
        }
    }
}

int main()
{
    int servidor_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (servidor_socket == -1)
    {
        perror("Erro ao criar socket");
        return 1;
    }

    int opcao = 1;
    setsockopt(
        servidor_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opcao,
        sizeof(opcao)
    );

    sockaddr_in endereco{};
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(8080);

    if (bind(
        servidor_socket,
        reinterpret_cast<sockaddr*>(&endereco),
        sizeof(endereco)
    ) == -1)
    {
        perror("Erro no bind");
        close(servidor_socket);
        return 1;
    }

    if (listen(servidor_socket, 10) == -1)
    {
        perror("Erro no listen");
        close(servidor_socket);
        return 1;
    }

    std::cout << "Servidor esperando cliente na porta 8080...\n";

    std::vector<std::thread> trabalhadores;

    for (int i = 0; i < 3; ++i)
        trabalhadores.emplace_back(trabalhador);

    while (true)
    {
        int cliente_socket = accept(servidor_socket, nullptr, nullptr);

        if (cliente_socket == -1)
        {
            perror("Erro no accept");
            continue;
        }

        std::cout << "\nCliente conectado!\n";

        char buffer[1024];
        std::string mensagem;
        bool mensagem_completa = false;
        bool erro_recebimento = false;

        // Recebe a palavra até encontrar '\n'.
        // Não espera o cliente fechar a conexão.
        while (!mensagem_completa)
        {
            ssize_t n = recv(
                cliente_socket,
                buffer,
                sizeof(buffer),
                0
            );

            if (n > 0)
            {
                mensagem.append(buffer, static_cast<size_t>(n));

                size_t fim = mensagem.find('\n');

                if (fim != std::string::npos)
                {
                    mensagem.erase(fim);
                    mensagem_completa = true;
                }
            }
            else if (n == 0)
            {
                break;
            }
            else
            {
                perror("Erro ao receber mensagem");
                erro_recebimento = true;
                break;
            }
        }

        // Remove '\r', caso o cliente envie "\r\n".
        if (!mensagem.empty() && mensagem.back() == '\r')
            mensagem.pop_back();

        if (erro_recebimento || !mensagem_completa || mensagem.empty())
        {
            std::cerr << "Mensagem vazia ou incompleta.\n";
            close(cliente_socket);
            continue;
        }

        std::cout << "Palavra recebida: " << mensagem << '\n';

        // Prepara as cinco tarefas.
        {
            std::lock_guard<std::mutex> trava(mutex_resultados);

            resultados.clear();
            tarefas_pendentes = static_cast<int>(livros.size());
        }

        {
            std::lock_guard<std::mutex> trava(mutex_fila);

            for (const std::string& caminho : livros)
            {
                fila.push({caminho, mensagem});
            }
        }

        condicao_fila.notify_all();

        // Aguarda os trabalhadores terminarem as buscas.
        std::string resposta;
        {
            std::unique_lock<std::mutex> trava(mutex_resultados);

            condicao_resultados.wait(trava, [] {
                return tarefas_pendentes == 0;
            });

            int total = 0;

            resposta = "Resultados para a palavra: " + mensagem + "\n\n";

            for (const Resultado& r : resultados)
            {
                resposta += r.caminho_arquivo + ": ";

                if (r.quantidade == -1)
                {
                    resposta += "erro ao abrir arquivo\n";
                }
                else
                {
                    resposta += std::to_string(r.quantidade) + "\n";
                    total += r.quantidade;
                }
            }

            resposta += "\nTotal encontrado: "
                      + std::to_string(total) + "\n";
        }

        // Envia toda a resposta, mesmo se send enviar apenas parte.
        size_t enviados = 0;

        while (enviados < resposta.size())
        {
            ssize_t n = send(
                cliente_socket,
                resposta.data() + enviados,
                resposta.size() - enviados,
                MSG_NOSIGNAL
            );

            if (n <= 0)
            {
                perror("Erro ao enviar resposta");
                break;
            }

            enviados += static_cast<size_t>(n);
        }

        std::cout << "Bytes enviados: " << enviados
                  << " de " << resposta.size() << '\n';

        // Indica que não haverá mais dados enviados.
        shutdown(cliente_socket, SHUT_WR);
        close(cliente_socket);

        std::cout << "Resposta enviada. Conexao encerrada.\n";
    }

    close(servidor_socket);

    for (auto& t : trabalhadores)
        t.join();

    return 0;
}
