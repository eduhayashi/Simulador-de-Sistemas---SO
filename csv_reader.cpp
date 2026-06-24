#include "csv_reader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

std::vector<Processo> ler_csv(const std::string& caminho_arquivo) {
    std::vector<Processo> processos;
    std::ifstream arquivo(caminho_arquivo);

    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir o arquivo CSV: " << caminho_arquivo << std::endl;
        return processos;
    }

    std::string linha;
    // Ignora a primeira linha (cabeçalho)
    if (!std::getline(arquivo, linha)) {
        return processos;
    }

    int proximo_id = 1;
    while (std::getline(arquivo, linha)) {
        // Remover caracteres de fim de linha do Windows (\r) se existirem
        if (!linha.empty() && linha.back() == '\r') {
            linha.pop_back();
        }

        if (linha.empty()) {
            continue;
        }

        std::stringstream ss(linha);
        std::string celula;
        std::vector<std::string> valores;

        while (std::getline(ss, celula, ',')) {
            // Remover espaços em branco extras ao redor do valor
            size_t primeiro = celula.find_first_not_of(" \t");
            if (primeiro != std::string::npos) {
                size_t ultimo = celula.find_last_not_of(" \t");
                valores.push_back(celula.substr(primeiro, (ultimo - primeiro + 1)));
            } else {
                valores.push_back("");
            }
        }

        // Suporta tanto 4 colunas (tempo_chegada, tempo_execucao, prioridade, memoria_mb)
        // quanto 5 colunas (id, tempo_chegada, tempo_execucao, prioridade, memoria_mb)
        if (valores.size() >= 4) {
            try {
                Processo p;
                if (valores.size() == 4) {
                    p.id = proximo_id++;
                    p.tempo_chegada = std::stoi(valores[0]);
                    p.tempo_execucao = std::stoi(valores[1]);
                    p.prioridade = std::stoi(valores[2]);
                    p.memoria_mb = std::stoi(valores[3]);
                } else {
                    p.id = std::stoi(valores[0]);
                    p.tempo_chegada = std::stoi(valores[1]);
                    p.tempo_execucao = std::stoi(valores[2]);
                    p.prioridade = std::stoi(valores[3]);
                    p.memoria_mb = std::stoi(valores[4]);
                }
                processos.push_back(p);
            } catch (const std::exception& e) {
                std::cerr << "Erro ao converter valores na linha: \"" << linha << "\" - " << e.what() << std::endl;
            }
        }
    }

    return processos;
}
