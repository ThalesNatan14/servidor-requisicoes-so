# Uso: make          -> compila cliente e servidor
#      make clean    -> apaga os executaveis
# Precisa de g++ (no WSL/Ubuntu: sudo apt install build-essential)

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread

all: cliente servidor

cliente: cliente.cpp
	$(CXX) $(CXXFLAGS) cliente.cpp -o cliente

# O servidor chama contarPalavra() e normalizar_palavra(), que estao
# DEFINIDAS em texto_em_palavras.cpp. Por isso esse arquivo precisa ser
# compilado junto (senao da "undefined reference" na hora de linkar).
servidor: servidor.cpp texto_em_palavras.cpp texto_em_palavras.hpp
	$(CXX) $(CXXFLAGS) servidor.cpp texto_em_palavras.cpp -o servidor

clean:
	rm -f cliente servidor