#ifdef _WIN32
#error "Compile dentro do WSL/Linux (no Windows puro os sockets sao diferentes: Winsock)."
#endif

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <functional>
#include <cerrno>
#include <cstdio>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

const char* SERVIDOR_IP = "127.0.0.1";
const int SERVIDOR_PORTA = 8080;

bool consultar(const std::string& palavra, std::string& resultado)
{
    resultado.clear();

    int cliente_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (cliente_socket < 0)
    {
        perror("Erro ao criar socket");
        return false;
    }

    // Tempo limite para receber a resposta.
    timeval limite{};
    limite.tv_sec = 30;
    limite.tv_usec = 0;

    if (setsockopt(
            cliente_socket,
            SOL_SOCKET,
            SO_RCVTIMEO,
            &limite,
            sizeof(limite)) < 0)
    {
        perror("Erro ao configurar timeout");
        close(cliente_socket);
        return false;
    }

    sockaddr_in endereco{};
    endereco.sin_family = AF_INET;
    endereco.sin_port = htons(SERVIDOR_PORTA);

    if (inet_pton(AF_INET, SERVIDOR_IP, &endereco.sin_addr) <= 0)
    {
        std::cerr << "Endereco IP invalido.\n";
        close(cliente_socket);
        return false;
    }

    if (connect(
            cliente_socket,
            reinterpret_cast<sockaddr*>(&endereco),
            sizeof(endereco)) < 0)
    {
        perror("Erro ao conectar ao servidor");
        close(cliente_socket);
        return false;
    }

    // Envia a palavra terminada em '\n'.
    std::string mensagem = palavra + "\n";
    size_t enviados = 0;

    while (enviados < mensagem.size())
    {
        ssize_t n = send(
            cliente_socket,
            mensagem.data() + enviados,
            mensagem.size() - enviados,
            MSG_NOSIGNAL
        );

        if (n < 0 && errno == EINTR)
            continue;

        if (n <= 0)
        {
            perror("Erro ao enviar palavra");
            close(cliente_socket);
            return false;
        }

        enviados += static_cast<size_t>(n);
    }

    // Recebe a resposta até o servidor fechar o envio.
    char buffer[1024];

    while (true)
    {
        ssize_t n = recv(
            cliente_socket,
            buffer,
            sizeof(buffer),
            0
        );

        if (n < 0 && errno == EINTR)
            continue;

        if (n < 0)
        {
            perror("Erro ao receber resposta");
            close(cliente_socket);
            return false;
        }

        if (n == 0)
            break;

        // Cada bloco é acrescentado exatamente uma vez.
        resultado.append(buffer, static_cast<size_t>(n));
    }

    close(cliente_socket);

    return !resultado.empty();
}

void modo_interativo()
{
    std::string palavra;
    std::string resultado;

    while (true)
    {
        std::cout << "Digite a palavra (ou 'sair'): " << std::flush;

        if (!std::getline(std::cin, palavra) || palavra == "sair")
            break;

        if (palavra.empty())
            continue;

        if (consultar(palavra, resultado))
        {
            std::cout << "\n" << resultado << "\n";
        }
        else
        {
            std::cerr
                << "Erro: nao foi possivel obter a resposta do servidor "
                << "(" << SERVIDOR_IP << ":" << SERVIDOR_PORTA << ").\n";
        }
    }
}

struct Estatisticas
{
    std::mutex mtx;
    long long ok = 0;
    long long erro = 0;
    double soma_ms = 0.0;
    double max_ms = 0.0;

    void registrar(bool sucesso, double ms)
    {
        std::lock_guard<std::mutex> trava(mtx);

        if (sucesso)
            ok++;
        else
            erro++;

        soma_ms += ms;

        if (ms > max_ms)
            max_ms = ms;
    }
};

void trabalho_cliente(
    const std::string& palavra,
    int consultas,
    Estatisticas& stats)
{
    std::string resultado;

    for (int i = 0; i < consultas; i++)
    {
        auto inicio = std::chrono::steady_clock::now();

        bool sucesso = consultar(palavra, resultado);

        auto fim = std::chrono::steady_clock::now();

        double duracao = std::chrono::duration<double, std::milli>(
            fim - inicio
        ).count();

        stats.registrar(sucesso, duracao);
    }
}

void modo_benchmark(
    int n_clientes,
    int consultas,
    const std::string& palavra)
{
    Estatisticas stats;
    std::vector<std::thread> threads;

    auto inicio = std::chrono::steady_clock::now();

    for (int i = 0; i < n_clientes; i++)
    {
        threads.emplace_back(
            trabalho_cliente,
            std::cref(palavra),
            consultas,
            std::ref(stats)
        );
    }

    for (auto& t : threads)
        t.join();

    auto fim = std::chrono::steady_clock::now();

    double total_s = std::chrono::duration<double>(
        fim - inicio
    ).count();

    long long total = stats.ok + stats.erro;

    std::cout << std::fixed << std::setprecision(3)
              << "\n===== RESULTADO DO BENCHMARK =====\n"
              << "Palavra buscada      : " << palavra << "\n"
              << "Clientes simultaneos : " << n_clientes << "\n"
              << "Consultas por cliente: " << consultas << "\n"
              << "Total de consultas   : " << total
              << " (ok: " << stats.ok
              << ", erro: " << stats.erro << ")\n"
              << "Tempo total          : " << total_s << " s\n"
              << "Vazao                : "
              << (total_s > 0 ? total / total_s : 0)
              << " consultas/s\n"
              << "Latencia media       : "
              << (total ? stats.soma_ms / total : 0)
              << " ms\n"
              << "Latencia maxima      : " << stats.max_ms << " ms\n";
}

int main(int argc, char* argv[])
{
    if (argc >= 5 && std::string(argv[1]) == "bench")
    {
        int n = 0;
        int m = 0;

        try
        {
            n = std::stoi(argv[2]);
            m = std::stoi(argv[3]);
        }
        catch (...)
        {
            std::cerr << "N e M precisam ser numeros inteiros.\n";
            return 1;
        }

        if (n <= 0 || m <= 0)
        {
            std::cerr << "N e M precisam ser maiores que 0.\n";
            return 1;
        }

        modo_benchmark(n, m, argv[4]);
    }
    else if (argc == 1)
    {
        modo_interativo();
    }
    else
    {
        std::cerr
            << "Uso:\n  " << argv[0]
            << "\n  " << argv[0]
            << " bench <N_clientes> <consultas_por_cliente> <palavra>\n";

        return 1;
    }

    return 0;
}
