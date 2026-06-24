#ifndef ESCALONADOR_H
#define ESCALONADOR_H

#include <vector>
#include <utility>
#include "processo.h"

// Retorna a linha do tempo como um vector de pairs (id_processo, tempo_atual).
// Se o ID for -1, significa que a CPU estava ociosa naquele instante de tempo.

std::vector<std::pair<int, int>> escalonamento_round_robin(std::vector<Processo> processos, int quantum);

std::vector<std::pair<int, int>> escalonamento_sjf_preemptivo(std::vector<Processo> processos);

std::vector<std::pair<int, int>> escalonamento_prioridade_preemptivo(std::vector<Processo> processos);

#endif // ESCALONADOR_H
