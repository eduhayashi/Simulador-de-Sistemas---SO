# 🖥️ Simulador de Sistemas Operacionais

Simulador de **Gerenciamento de Processos e Memória Virtual** desenvolvido em C++ com interface gráfica (GTK3), como projeto da disciplina de Sistemas Operacionais — Engenharia de Computação, 2026.

---

## 📋 Sobre o Projeto

O simulador permite carregar uma lista de processos via arquivo CSV e visualizar, de forma interativa, o comportamento de diferentes algoritmos de escalonamento e políticas de substituição de páginas.

---

## ⚙️ Funcionalidades

### Escalonamento de Processos
- **Round-Robin (RR)** — com quantum configurável
- **SJF Preemptivo** — Shortest Job First com preempção
- **Prioridade Preemptiva** — escalonamento por prioridade com preempção

### Gerenciamento de Memória Virtual
- Simulação de paginação com frames configuráveis
- **FIFO** — First In, First Out
- **LRU** — Least Recently Used
- **Ótimo** — substituição ideal (teórica)

### Relatório de Saída
- Linha do tempo de execução dos processos
- Tempo Médio de Resposta
- Tempo Médio de Espera
- Total de Page Faults

---

## 🗂️ Formato do Arquivo de Entrada (CSV)

```
tempo_chegada,tempo_execucao,prioridade,memoria_mb
0,5,2,128
2,3,1,64
4,8,3,256
```

---

## 🖼️ Interface Gráfica

- Seleção do arquivo CSV de entrada
- Configuração da memória física e virtual (em MB)
- Escolha do algoritmo de escalonamento e da política de substituição
- Exibição do relatório ao final da simulação

---

## 🛠️ Tecnologias Utilizadas

- **Linguagem:** C++
- **Interface Gráfica:** GTK3
- **Build:** Makefile

---

## 🚀 Como Compilar e Executar

### Pré-requisitos

```bash
sudo apt install build-essential libgtk-3-dev
```

### Compilar

```bash
make
```

### Executar

```bash
./simulador
```

---

## 📁 Estrutura do Projeto

```
.
├── main.cpp
├── processo.h
├── csv_reader.h / csv_reader.cpp
├── escalonador.h / escalonador.cpp
├── memoria.h / memoria.cpp
├── relatorio.h / relatorio.cpp
├── interface.h / interface.cpp
├── Makefile
└── README.md
```

---

## 👥 Integrantes

| Nome                      | RA       |
|---------------------------|----------|
| Eduardo Akutagawa Hayashi | a2725991 |
| Leonardo Simões Laraya    | a2703688 |

---

## 📅 Informações da Disciplina

- **Curso:** Engenharia de Computação
- **Semestre:** 2026
- **Entrega:** 21/06/2026
- **Apresentação:** 22 e 24/06/2026
