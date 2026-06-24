#include "relatorio.h"
#include <iostream>
#include <cstdio>
#include <string>
#include <unordered_map>

Estatisticas calcular_estatisticas(const std::vector<Processo>& processos, 
                                   const std::vector<std::pair<int, int>>& linha_tempo) {
    Estatisticas est = {0.0, 0.0};
    if (processos.empty()) return est;

    int n = processos.size();
    double total_resposta = 0;
    double total_espera = 0;

    for (const auto& p : processos) {
        int primeiro_t = -1;
        int ultimo_t = -1;

        for (const auto& tick : linha_tempo) {
            if (tick.first == p.id) {
                if (primeiro_t == -1) {
                    primeiro_t = tick.second;
                }
                ultimo_t = tick.second;
            }
        }

        // Se o processo nunca executou por algum motivo (e.g. tempo de execução nulo)
        if (primeiro_t == -1) {
            continue;
        }

        int tempo_resposta = primeiro_t - p.tempo_chegada;
        int tempo_termino = ultimo_t + 1;
        int tempo_retorno = tempo_termino - p.tempo_chegada;
        int tempo_espera = tempo_retorno - p.tempo_execucao;

        total_resposta += tempo_resposta;
        total_espera += tempo_espera;
    }

    est.tempo_medio_resposta = total_resposta / n;
    est.tempo_medio_espera = total_espera / n;

    return est;
}

// Retorna uma representação legível e condensada da linha do tempo da CPU
static std::string obter_linha_tempo_formatada(const std::vector<std::pair<int, int>>& linha_tempo) {
    if (linha_tempo.empty()) return "Linha do tempo vazia";

    std::string s = "|";
    int id_atual = linha_tempo[0].first;
    int duracao = 0;

    for (size_t i = 0; i < linha_tempo.size(); ++i) {
        if (linha_tempo[i].first == id_atual) {
            duracao++;
        } else {
            if (id_atual == -1) {
                s += " OCIOSO (" + std::to_string(duracao) + "s) |";
            } else {
                s += " P" + std::to_string(id_atual) + " (" + std::to_string(duracao) + "s) |";
            }
            id_atual = linha_tempo[i].first;
            duracao = 1;
        }
    }

    // Adiciona o último bloco
    if (id_atual == -1) {
        s += " OCIOSO (" + std::to_string(duracao) + "s) |";
    } else {
        s += " P" + std::to_string(id_atual) + " (" + std::to_string(duracao) + "s) |";
    }

    return s;
}

#include <sstream>

std::string gerar_relatorio_string(const std::vector<Processo>& processos, 
                                   const std::vector<std::pair<int, int>>& linha_tempo,
                                   int page_faults) {
    Estatisticas est = calcular_estatisticas(processos, linha_tempo);
    std::stringstream ss;

    ss << "\n========================================================\n";
    ss << "                 RELATORIO DE SIMULACAO                 \n";
    ss << "========================================================\n\n";

    // 1. Tabela com as métricas individuais por processo
    ss << "Metricas por Processo:\n";
    ss << "--------------------------------------------------------\n";
    ss << "ID   Chegada   Execucao   Prioridade   Espera   Resposta\n";
    ss << "--------------------------------------------------------\n";

    char buffer[256];
    for (const auto& p : processos) {
        int primeiro_t = -1;
        int ultimo_t = -1;

        for (const auto& tick : linha_tempo) {
            if (tick.first == p.id) {
                if (primeiro_t == -1) {
                    primeiro_t = tick.second;
                }
                ultimo_t = tick.second;
            }
        }

        int tempo_resposta = (primeiro_t == -1) ? 0 : (primeiro_t - p.tempo_chegada);
        int tempo_termino = (ultimo_t == -1) ? 0 : (ultimo_t + 1);
        int tempo_retorno = tempo_termino - p.tempo_chegada;
        int tempo_espera = tempo_retorno - p.tempo_execucao;

        std::snprintf(buffer, sizeof(buffer), "%-4d %-9d %-10d %-12d %-8d %-8d\n", 
                      p.id, p.tempo_chegada, p.tempo_execucao, p.prioridade, tempo_espera, tempo_resposta);
        ss << buffer;
    }
    ss << "--------------------------------------------------------\n\n";

    // 2. Linha do tempo gráfica simplificada
    ss << "Linha do Tempo de Execucao da CPU:\n";
    ss << obter_linha_tempo_formatada(linha_tempo) << "\n\n";

    // 3. Métricas globais
    ss << "Estatisticas Gerais:\n";
    ss << "--------------------------------------------------------\n";
    std::snprintf(buffer, sizeof(buffer), "Tempo Medio de Espera:   %.2f s\n", est.tempo_medio_espera);
    ss << buffer;
    std::snprintf(buffer, sizeof(buffer), "Tempo Medio de Resposta: %.2f s\n", est.tempo_medio_resposta);
    ss << buffer;
    if (page_faults >= 0) {
        std::snprintf(buffer, sizeof(buffer), "Total de Page Faults:    %d\n", page_faults);
        ss << buffer;
    }
    ss << "========================================================\n\n";

    return ss.str();
}

void exibir_relatorio(const std::vector<Processo>& processos, 
                      const std::vector<std::pair<int, int>>& linha_tempo,
                      int page_faults) {
    std::cout << gerar_relatorio_string(processos, linha_tempo, page_faults);
}
