#include "escalonador.h"
#include <algorithm>
#include <queue>
#include <iostream>

std::vector<std::pair<int, int>> escalonamento_round_robin(std::vector<Processo> processos, int quantum) {
    std::vector<std::pair<int, int>> linha_tempo;
    if (processos.empty()) return linha_tempo;
    if (quantum <= 0) quantum = 1; // Garante quantum válido

    std::vector<int> fila_prontos;
    std::vector<int> tempo_restante(processos.size());
    std::vector<bool> no_sistema(processos.size(), false);
    std::vector<bool> finalizado(processos.size(), false);

    for (size_t i = 0; i < processos.size(); ++i) {
        tempo_restante[i] = processos[i].tempo_execucao;
    }

    int t = 0;
    int processos_restantes = processos.size();
    int index_atual = -1;
    int quantum_restante = 0;

    while (processos_restantes > 0) {
        // 1. Adicionar processos que chegaram neste instante t
        for (size_t i = 0; i < processos.size(); ++i) {
            if (!no_sistema[i] && !finalizado[i] && processos[i].tempo_chegada <= t) {
                fila_prontos.push_back(i);
                no_sistema[i] = true;
            }
        }

        // 2. Se o processo atual terminou ou esgotou o quantum, decidir o próximo
        if (index_atual != -1) {
            if (tempo_restante[index_atual] == 0) {
                finalizado[index_atual] = true;
                processos_restantes--;
                index_atual = -1;
            } else if (quantum_restante == 0) {
                fila_prontos.push_back(index_atual);
                index_atual = -1;
            }
        }

        // 3. Escolher novo processo se nenhum estiver rodando
        if (index_atual == -1 && !fila_prontos.empty()) {
            index_atual = fila_prontos.front();
            fila_prontos.erase(fila_prontos.begin());
            quantum_restante = std::min(quantum, tempo_restante[index_atual]);
        }

        // 4. Executar se houver processo atual
        if (index_atual != -1) {
            linha_tempo.push_back({processos[index_atual].id, t});
            tempo_restante[index_atual]--;
            quantum_restante--;
        } else {
            // CPU ociosa
            linha_tempo.push_back({-1, t});
        }

        t++;
    }

    return linha_tempo;
}

std::vector<std::pair<int, int>> escalonamento_sjf_preemptivo(std::vector<Processo> processos) {
    std::vector<std::pair<int, int>> linha_tempo;
    if (processos.empty()) return linha_tempo;

    std::vector<int> tempo_restante(processos.size());
    for (size_t i = 0; i < processos.size(); ++i) {
        tempo_restante[i] = processos[i].tempo_execucao;
    }

    int t = 0;
    int processos_restantes = processos.size();

    while (processos_restantes > 0) {
        int melhor_indice = -1;
        int menor_tempo_restante = 1e9;

        for (size_t i = 0; i < processos.size(); ++i) {
            if (processos[i].tempo_chegada <= t && tempo_restante[i] > 0) {
                if (tempo_restante[i] < menor_tempo_restante) {
                    menor_tempo_restante = tempo_restante[i];
                    melhor_indice = i;
                } else if (tempo_restante[i] == menor_tempo_restante) {
                    // Critérios de desempate:
                    // 1. Quem chegou primeiro
                    // 2. Menor ID do processo
                    if (melhor_indice == -1 || processos[i].tempo_chegada < processos[melhor_indice].tempo_chegada) {
                        melhor_indice = i;
                    } else if (processos[i].tempo_chegada == processos[melhor_indice].tempo_chegada) {
                        if (processos[i].id < processos[melhor_indice].id) {
                            melhor_indice = i;
                        }
                    }
                }
            }
        }

        if (melhor_indice != -1) {
            linha_tempo.push_back({processos[melhor_indice].id, t});
            tempo_restante[melhor_indice]--;
            if (tempo_restante[melhor_indice] == 0) {
                processos_restantes--;
            }
        } else {
            // CPU ociosa
            linha_tempo.push_back({-1, t});
        }
        t++;
    }

    return linha_tempo;
}

std::vector<std::pair<int, int>> escalonamento_prioridade_preemptivo(std::vector<Processo> processos) {
    std::vector<std::pair<int, int>> linha_tempo;
    if (processos.empty()) return linha_tempo;

    std::vector<int> tempo_restante(processos.size());
    for (size_t i = 0; i < processos.size(); ++i) {
        tempo_restante[i] = processos[i].tempo_execucao;
    }

    int t = 0;
    int processos_restantes = processos.size();

    while (processos_restantes > 0) {
        int melhor_indice = -1;
        int melhor_prioridade = 1e9; // Assume que menor valor numérico = maior prioridade acadêmica

        for (size_t i = 0; i < processos.size(); ++i) {
            if (processos[i].tempo_chegada <= t && tempo_restante[i] > 0) {
                if (processos[i].prioridade < melhor_prioridade) {
                    melhor_prioridade = processos[i].prioridade;
                    melhor_indice = i;
                } else if (processos[i].prioridade == melhor_prioridade) {
                    // Critérios de desempate:
                    // 1. Quem chegou primeiro
                    // 2. Menor ID do processo
                    if (melhor_indice == -1 || processos[i].tempo_chegada < processos[melhor_indice].tempo_chegada) {
                        melhor_indice = i;
                    } else if (processos[i].tempo_chegada == processos[melhor_indice].tempo_chegada) {
                        if (processos[i].id < processos[melhor_indice].id) {
                            melhor_indice = i;
                        }
                    }
                }
            }
        }

        if (melhor_indice != -1) {
            linha_tempo.push_back({processos[melhor_indice].id, t});
            tempo_restante[melhor_indice]--;
            if (tempo_restante[melhor_indice] == 0) {
                processos_restantes--;
            }
        } else {
            // CPU ociosa
            linha_tempo.push_back({-1, t});
        }
        t++;
    }

    return linha_tempo;
}
