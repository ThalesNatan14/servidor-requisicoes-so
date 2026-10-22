# Servidor de Requisições — Sistemas Operacionais

**Integrantes:**
- Gustavo Madeira Meneghelo
- Thales Natan de Oliveira

## Sobre o projeto

Este projeto consiste em um servidor de requisições desenvolvido em C++ para a disciplina de Sistemas Operacionais.

O sistema permite que um cliente envie uma palavra ao servidor, que pesquisa essa palavra em cinco livros no formato `.txt` e retorna a quantidade de ocorrências encontradas em cada arquivo.

O servidor utiliza comunicação TCP por sockets, uma fila de tarefas compartilhada, mutex, variável de condição e um conjunto de três threads trabalhadoras (*thread pool*) para processar as requisições.

## Tecnologias e conceitos utilizados

- **C++:** implementação do cliente, servidor e processamento dos textos.
- **Sockets TCP:** comunicação entre cliente e servidor.
- **Threads:** processamento das tarefas em paralelo.
- **Mutex:** proteção do acesso à fila compartilhada.
- **Variável de condição:** sincronização das threads trabalhadoras.
- **Fila de tarefas:** organização das requisições pendentes.

## Como executar

### 1. Compilação

Abra um terminal na pasta do projeto.

Compile o servidor:


g++ -pthread servidor.cpp texto_em_palavras.cpp -o servidor


Compile o cliente:


g++ -pthread cliente.cpp -o cliente


### 2. Iniciar o servidor

Em um terminal, execute:


./servidor


O servidor ficará aguardando conexões dos clientes. No código atual, a comunicação utiliza a porta `9090`.

### 3. Modo interativo

Abra outro terminal e execute:


./cliente


Digite a palavra que deseja pesquisar. O cliente enviará a requisição ao servidor e exibirá a quantidade de ocorrências encontradas em cada um dos cinco livros.

Esse modo é indicado para realizar pesquisas individuais e demonstrar o funcionamento do sistema.

### 4. Modo benchmark

Para testar o comportamento do servidor sob carga, execute o cliente com o comando:


./cliente bench 5 10 casa


Os argumentos são:

- `bench`: ativa o modo benchmark.
- `5`: quantidade de clientes simulados.
- `10`: quantidade de consultas realizadas por cada cliente.
- `casa`: palavra pesquisada em todas as consultas.

Nesse exemplo, serão realizadas até 50 consultas. Ao final, o programa apresenta estatísticas do teste, incluindo o tempo total, a vazão, a latência média, a latência máxima e a quantidade de consultas bem-sucedidas ou com erro.

É possível alterar os parâmetros. Por exemplo:


./cliente bench 10 10 casa


Esse comando simula dez clientes, com dez consultas por cliente, totalizando até 100 consultas.

Para testar dois benchmarks simultaneamente, execute um comando em cada terminal de cliente, mantendo o servidor em execução. Isso aumenta a carga sobre o sistema.

**Observação:** o benchmark simula vários clientes por meio de threads no programa cliente. O processamento no servidor é realizado por três threads trabalhadoras.

## Considerações finais

O projeto demonstra conceitos de comunicação entre processos, sockets TCP, concorrência, sincronização e gerenciamento de tarefas com threads em C++. Além do funcionamento normal das requisições, o modo benchmark permite observar o comportamento do sistema quando submetido a diferentes cargas de trabalho.
