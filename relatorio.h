#ifndef RELATORIO_H
#define RELATORIO_H

#include <vector>
#include <string>
#include <utility>
#include "processo.h"

struct Estatisticas {
    double tempo_medio_resposta;
    double tempo_medio_espera;
};

// Calcula o tempo médio de resposta e tempo médio de espera
Estatisticas calcular_estatisticas(const std::vector<Processo>& processos, 
                                   const std::vector<std::pair<int, int>>& linha_tempo);

// Exibe no terminal o relatório formatado
void exibir_relatorio(const std::vector<Processo>& processos, 
                      const std::vector<std::pair<int, int>>& linha_tempo,
                      int page_faults);

// Gera a string do relatório formatado para ser exibida no terminal ou na GUI
std::string gerar_relatorio_string(const std::vector<Processo>& processos, 
                                   const std::vector<std::pair<int, int>>& linha_tempo,
                                   int page_faults);

#endif // RELATORIO_H
