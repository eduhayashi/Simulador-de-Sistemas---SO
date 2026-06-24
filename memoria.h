#ifndef MEMORIA_H
#define MEMORIA_H

#include <vector>
#include <utility>
#include "processo.h"

// Funções para simulação de paginação de memória.
// Recebem o número de frames, tamanho da página em MB, o vetor de processos
// e a linha do tempo de execução da CPU (par id_processo, tempo).
// Retornam o número total de page faults.

int simular_fifo(int num_frames, int tamanho_pagina_mb, 
                 const std::vector<Processo>& processos, 
                 const std::vector<std::pair<int, int>>& linha_tempo);

int simular_lru(int num_frames, int tamanho_pagina_mb, 
                const std::vector<Processo>& processos, 
                const std::vector<std::pair<int, int>>& linha_tempo);

int simular_otimo(int num_frames, int tamanho_pagina_mb, 
                  const std::vector<Processo>& processos, 
                  const std::vector<std::pair<int, int>>& linha_tempo);

#endif // MEMORIA_H
