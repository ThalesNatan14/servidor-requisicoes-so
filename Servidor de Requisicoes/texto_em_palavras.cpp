#include "texto_em_palavras.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <algorithm>

std::string normalizar_palavra(std::string palavra)
{
    // Transformar todos os caracteres em minúsculo
    for (char& caracter : palavra)
    {
        caracter = std::tolower(caracter);
    }

    // Remover pontuação
    palavra.erase(
        std::remove_if(palavra.begin(), palavra.end(), ::ispunct),
        palavra.end()
    );

    return palavra;
}

int contarPalavra(const std::string caminho_arquivo, const std::string palavra)
{
    std::ifstream arquivo(caminho_arquivo);

    if (!arquivo.is_open())
    {
        std::cout<< "O arquivo, do caminho "<< caminho_arquivo<< " não abriu corretamente\n";

        return -1;
    }

    int contador_palavra = 0;
    std::string palavra_do_texto;

    // Normalizar a palavra procurada apenas uma vez
    std::string palavra_normalizada = normalizar_palavra(palavra);

    // Ler o arquivo palavra por palavra
    while (arquivo >> palavra_do_texto)
    {
        palavra_do_texto = normalizar_palavra(palavra_do_texto);

        // Comparar a palavra procurada com a palavra do texto
        if (palavra_normalizada == palavra_do_texto)
        {
            contador_palavra++;
        }
    }

    return contador_palavra;
}
